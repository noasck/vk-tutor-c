#include "vkinit.h"

static inline int
u_strcmp ( const char * str1, const char * str2 )
{
    while ( *str1 && *str1 == *str2 ) str1++, str2++;
    return ( int ) ( ( uint8_t ) *str1 - ( uint8_t ) *str2 );
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback ( VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT              messageType,
                const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
                void *                                       pUserData )
{

    fprintf ( stdout, "vk_debug >>> %s\n", pCallbackData->pMessage );
    return VK_FALSE;
}

VkResult
BasedVKInit ( Engine * engine )
{
    VkResult rcode = VK_INCOMPLETE;

    /* Debug logger instantiated */
    VkDebugUtilsMessengerCreateInfoEXT debugMsgrCreateInfo = {};
    debugMsgrCreateInfo.sType =
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugMsgrCreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugMsgrCreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugMsgrCreateInfo.pfnUserCallback = debugCallback;

    /* VK info for VK instance */
    VkApplicationInfo appInfo       = {};
    appInfo.sType                   = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName        = "Hello Triangle";
    appInfo.applicationVersion      = VK_MAKE_VERSION ( 1, 0, 0 );
    appInfo.pEngineName             = "Chiteks Engine";
    appInfo.engineVersion           = VK_MAKE_VERSION ( 0, 1, 0 );
    appInfo.apiVersion              = VK_API_VERSION_1_1;
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo     = &appInfo;
    if ( engine->validationLayerCount )
        createInfo.pNext =
            ( VkDebugUtilsMessengerCreateInfoEXT * ) &debugMsgrCreateInfo;
    else { createInfo.pNext = NULL; }

    /* Fetch VK+SDL Extensions */
    uint32_t extensionCount = 0;
    if ( ! SDL_Vulkan_GetInstanceExtensions ( NULL, &extensionCount, NULL ) )
    {

        _DEBUG_P ( "error: getting extensions count: %s\n", SDL_GetError () );
        goto r_base;
    }
    extensionCount += ( engine->validationLayerCount ? 1 : 0 );

    const char ** vkExtensions =
        ( const char ** ) malloc ( extensionCount * sizeof ( const char * ) );
    if ( ! vkExtensions )
    {
        _DEBUG_P ( "error: malloc vkExtensions of size: %zu\n",
                   extensionCount * sizeof ( const char * ) );
        goto r_base;
    }
    /* defer: free(vkExtensions) */
    if ( ! SDL_Vulkan_GetInstanceExtensions (
             NULL, &extensionCount, vkExtensions ) )
    {

        _DEBUG_P ( "error: getting extensions: %s\n", SDL_GetError () );
        goto defer_free_extensions;
    }

    if ( engine->validationLayerCount )
        vkExtensions[ extensionCount - 1 ] =
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    createInfo.enabledExtensionCount   = extensionCount;
    createInfo.ppEnabledExtensionNames = vkExtensions;

    /* Validate validation valid validator layers...*/

    uint32_t layerCount;
    VkResult opResult;
    if ( ( opResult =
               vkEnumerateInstanceLayerProperties ( &layerCount, NULL ) ) )
    {

        _DEBUG_P ( "error: getting available layers count: %d\n", opResult );
        goto defer_free_extensions;
    }

    VkLayerProperties * availableLayers = ( VkLayerProperties * ) malloc (
        layerCount * sizeof ( VkLayerProperties ) );
    if ( ! availableLayers )
    {
        _DEBUG_P ( "error: malloc availableLayers of size: %zu\n",
                   layerCount * sizeof ( VkLayerProperties ) );
        goto defer_free_extensions;
    }

    /* defer: free(availableLayers) */

    if ( ( opResult = vkEnumerateInstanceLayerProperties (
               &layerCount, availableLayers ) ) )
    {
        _DEBUG_P ( "error: getting available layers: %d\n", opResult );
        goto defer_free_availableLayers;
    }

    for ( size_t i = 0; i < engine->validationLayerCount; i++ )
    {
        uint8_t      layerFound = 0;
        const char * layerName  = engine->validationLayers[ i ];
        for ( uint32_t j = 0; j < layerCount; j++ )
        {
            if ( u_strcmp ( layerName, availableLayers[ j ].layerName ) == 0 )
            {
                layerFound = 1;
                break;
            }
        }
        if ( ! layerFound )
        {
            _DEBUG_P ( "error: validation layer not available: %zu\n", i );
            goto defer_free_availableLayers;
        }
    }

    _DEBUG_P ( "All validation layers are supported.\n" );

    if ( engine->validationLayerCount )
    {
        createInfo.enabledLayerCount   = engine->validationLayerCount;
        createInfo.ppEnabledLayerNames = engine->validationLayers;
    }
    else { createInfo.enabledLayerCount = 0; }

    /* Vk instance creation */
    if ( ( opResult = vkCreateInstance (
               &createInfo, NULL, &( engine->vkInstance ) ) ) )
    {
        _DEBUG_P ( "error: creating VK instance: %d\n", opResult );
        goto defer_free_availableLayers;
    }

    /* defer: vkDestroyInstance */

    /* Debug Messenger Creation */
    if ( engine->validationLayerCount )
    {
        PFN_vkCreateDebugUtilsMessengerEXT func =
            ( PFN_vkCreateDebugUtilsMessengerEXT ) vkGetInstanceProcAddr (
                engine->vkInstance, "vkCreateDebugUtilsMessengerEXT" );
        if ( ! func )
        {
            _DEBUG_P ( "error: retrieving DebugUtilsEXT func: %d\n",
                       opResult );
            goto defer_destroy_instance;
        }
        if ( ( opResult = func ( engine->vkInstance,
                                 &debugMsgrCreateInfo,
                                 NULL,
                                 &( engine->debugMessenger ) ) ) )
        {
            _DEBUG_P ( "error: creating DebugUtilsMessenger: %d\n",
                       opResult );
            goto defer_destroy_instance;
        }
        /* defer: on fail: destroyDebugUtilsMessenger */
    }

    /* Physical Device Selection */

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices ( engine->vkInstance, &deviceCount, NULL );
    if ( deviceCount == 0 )
    {
        _DEBUG_P ( "error: no physical devices found" );
        goto defer_destroy_degub_msgr;
    }

    VkPhysicalDevice * devices =
        malloc ( deviceCount * sizeof ( VkPhysicalDevice ) );

    if ( ! devices )
    {
        _DEBUG_P ( "error: malloc devices of size : %zu\n",
                   deviceCount * sizeof ( VkPhysicalDevice ) );
        goto defer_destroy_degub_msgr;
    }

    /* defer: free(devices) */

    if ( ( opResult = vkEnumeratePhysicalDevices (
               engine->vkInstance, &deviceCount, devices ) ) )
    {
        _DEBUG_P ( "error: getting Physical Devices : %d\n", opResult );
        goto defer_free_devices;
    }

    /* Iterate through devices and select suitable */

    VkPhysicalDeviceProperties deviceProperties = {};
    VkPhysicalDeviceFeatures   deviceFeatures   = {};

    for ( uint32_t i = 0; i < deviceCount; i++ )
    {

        vkGetPhysicalDeviceProperties ( devices[ i ], &deviceProperties );
        vkGetPhysicalDeviceFeatures ( devices[ i ], &deviceFeatures );
        _DEBUG_P ( "Device found: %s of type %d\n",
                   deviceProperties.deviceName,
                   deviceProperties.deviceType );
        if ( (
#ifdef LLVM_MESA
                 deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU
#else
                 deviceProperties.deviceType ==
                     VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ||
                 deviceProperties.deviceType ==
                     VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
#endif
                 ) &&
             deviceFeatures.geometryShader )
        {
            engine->physicalDevice = devices[ i ];
            break;
        }
    }

    if ( ! engine->physicalDevice )
    {
        _DEBUG_P ( "error: getting Physical Devices : no suitable found" );
        goto defer_free_devices;
    }

    /* Get Queue Families for current device */
    uint32_t queueFamilyCount = 0;

    vkGetPhysicalDeviceQueueFamilyProperties (
        engine->physicalDevice, &queueFamilyCount, NULL );

    if ( ! queueFamilyCount )
    {
        _DEBUG_P ( "error: no queues available on the device" );
        goto defer_free_devices;
    }

    VkQueueFamilyProperties * queues = ( VkQueueFamilyProperties * ) malloc (
        queueFamilyCount * sizeof ( VkQueueFamilyProperties ) );
    if ( ! queues )
    {
        _DEBUG_P ( "error: malloc queues of size : %zu\n",
                   queueFamilyCount * sizeof ( VkQueueFamilyProperties ) );
        goto defer_destroy_degub_msgr;
    }
    /* defer: ON FAIL: free(queues) */

    vkGetPhysicalDeviceQueueFamilyProperties (
        engine->physicalDevice, &queueFamilyCount, queues );

    /* Save queues to engine instance */
    QueueFamilies * queuesFamilies =
        ( QueueFamilies * ) malloc ( sizeof ( QueueFamilies ) );

    if ( ! queuesFamilies )
    {
        _DEBUG_P ( "error: malloc queueFamilies struct of size : %zu\n",
                   sizeof ( QueueFamilies ) );
        goto defer_free_queues;
    }
    /* defer: ON FAIL: free(queuesFamilies) */

    queuesFamilies->count  = queueFamilyCount;
    queuesFamilies->queues = queues;
    engine->queueFamilies  = queuesFamilies;

    /* Find suitable graphics queue */
    uint32_t graphicsFamilyIdx = -1;
    for ( uint32_t i = 0; i < engine->queueFamilies->count; i++ )
    {

        if ( engine->queueFamilies->queues[ i ].queueFlags &
             VK_QUEUE_GRAPHICS_BIT )
        {
            graphicsFamilyIdx = i;
            break;
        }
    }
    if ( graphicsFamilyIdx == -1 )
    {
        _DEBUG_P ( "error: suitable queue Family not fount\n" );
        goto defer_free_queueFamilies;
    }

    /* Create Logical Device */
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsFamilyIdx;
    queueCreateInfo.queueCount       = engine->queueFamilies->count;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo logDeviceInfo   = {};
    logDeviceInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    logDeviceInfo.pQueueCreateInfos    = &queueCreateInfo;
    logDeviceInfo.queueCreateInfoCount = 1;
    logDeviceInfo.pEnabledFeatures     = &deviceFeatures;
    logDeviceInfo.enabledExtensionCount = 0;

    if ( engine->validationLayerCount )
    {
        logDeviceInfo.enabledLayerCount   = engine->validationLayerCount;
        logDeviceInfo.ppEnabledLayerNames = engine->validationLayers;
    }

    if ( ( opResult = vkCreateDevice ( engine->physicalDevice,
                                       &logDeviceInfo,
                                       NULL,
                                       &( engine->device ) ) ) )
    {
        _DEBUG_P ( "error: creating Logical Device : %d\n", opResult );
        goto defer_free_queueFamilies;
    }

    /* Get Vk Queue */

    vkGetDeviceQueue (
        engine->device, graphicsFamilyIdx, 0, &( engine->graphicsQueue ) );

    rcode = VK_SUCCESS;

defer_free_queueFamilies:
    if ( rcode ) free ( queuesFamilies );
defer_free_queues:
    if ( rcode ) free ( queues );
defer_free_devices:
    free ( devices );
defer_destroy_degub_msgr:
    if ( rcode && engine->validationLayerCount )
    {
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            ( PFN_vkDestroyDebugUtilsMessengerEXT ) vkGetInstanceProcAddr (
                engine->vkInstance, "vkDestroyDebugUtilsMessengerEXT" );
        if ( func != NULL )
        {
            func ( engine->vkInstance, engine->debugMessenger, NULL );
        }
    }
defer_destroy_instance:
    if ( rcode ) vkDestroyInstance ( engine->vkInstance, NULL );
defer_free_availableLayers:
    free ( availableLayers );
defer_free_extensions:
    free ( vkExtensions );
r_base:
    return rcode;
}

VkResult
BasedVKCleanup ( Engine * engine )
{
    if ( engine->debugMessenger && engine->validationLayerCount )
    {
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            ( PFN_vkDestroyDebugUtilsMessengerEXT ) vkGetInstanceProcAddr (
                engine->vkInstance, "vkDestroyDebugUtilsMessengerEXT" );
        if ( func != NULL )
        {
            func ( engine->vkInstance, engine->debugMessenger, NULL );
        }
    }
    free ( engine->queueFamilies->queues );
    free ( engine->queueFamilies );
    vkDestroyDevice ( engine->device, NULL );
    vkDestroyInstance ( engine->vkInstance, NULL );
    return 0;
}
