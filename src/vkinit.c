#include "vkinit.h"

/* Load GLSL -> compiled SPIR-V Shaders embedded with xxd */
#include "resources/shaders/Triangle_frag.h"
#include "resources/shaders/Triangle_vert.h"

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

static inline uint32_t
u_clamp ( uint32_t value, uint32_t min, uint32_t max )
{
    return value < min ? min : ( value > max ? max : value );
}

static SwapChainConfig
chooseBestSwapChainConfig ( const SwapChainSupportDetails * details,
                            uint32_t                        window_width,
                            uint32_t                        window_height )
{
    SwapChainConfig config;

    config.surfaceFormat = details->formats[ 0 ];
    for ( size_t i = 0; i < details->formatCount; i++ )
    {
        if ( details->formats[ i ].format == VK_FORMAT_B8G8R8A8_SRGB &&
             details->formats[ i ].colorSpace ==
                 VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
        {
            config.surfaceFormat = details->formats[ i ];
            break;
        }
    }

    config.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for ( size_t i = 0; i < details->modeCount; i++ )
    {
        if ( details->modes[ i ] == VK_PRESENT_MODE_MAILBOX_KHR )
        {
            config.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    if ( details->capabilities.currentExtent.width != UINT32_MAX )
    {
        config.extent = details->capabilities.currentExtent;
    }
    else
    {

        config.extent.width =
            u_clamp ( window_width,
                      details->capabilities.minImageExtent.width,
                      details->capabilities.maxImageExtent.width );
        config.extent.height =
            u_clamp ( window_height,
                      details->capabilities.minImageExtent.height,
                      details->capabilities.maxImageExtent.height );
    }

    config.imageCount = details->capabilities.minImageCount + 1;

    if ( details->capabilities.maxImageCount > 0 &&
         config.imageCount > details->capabilities.maxImageCount )
        config.imageCount = details->capabilities.maxImageCount;

    return config;
}

int
querySwapChainSupport ( Engine * engine )
{
    VkResult                  result  = -1;
    SwapChainSupportDetails * details = &engine->swapChainDetails;

    VkSurfaceCapabilitiesKHR capabilities;
    if ( vkGetPhysicalDeviceSurfaceCapabilitiesKHR (
             engine->physicalDevice, engine->surface, &capabilities ) !=
         VK_SUCCESS )
    {
        _DEBUG_P ( "error: failed to query surface capabilities\n" );
        return result;
    }
    details->capabilities = capabilities;

    uint32_t formatCount = 0;
    if ( vkGetPhysicalDeviceSurfaceFormatsKHR (
             engine->physicalDevice, engine->surface, &formatCount, NULL ) !=
         VK_SUCCESS )
    {
        _DEBUG_P ( "error: failed to get surface format count\n" );
        return result;
    }

    if ( formatCount > 0 )
    {
        details->formats = ( VkSurfaceFormatKHR * ) malloc (
            formatCount * sizeof ( VkSurfaceFormatKHR ) );
        if ( ! details->formats )
        {
            _DEBUG_P ( "error: malloc failed for formats\n" );
            return result;
        }

        if ( vkGetPhysicalDeviceSurfaceFormatsKHR ( engine->physicalDevice,
                                                    engine->surface,
                                                    &formatCount,
                                                    details->formats ) !=
             VK_SUCCESS )
        {
            _DEBUG_P ( "error: failed to get surface formats\n" );
            goto defer_free_formats;
        }
    }
    details->formatCount = formatCount;

    uint32_t modeCount = 0;
    if ( vkGetPhysicalDeviceSurfacePresentModesKHR (
             engine->physicalDevice, engine->surface, &modeCount, NULL ) !=
         VK_SUCCESS )
    {
        _DEBUG_P ( "error: failed to get present mode count\n" );
        goto defer_free_formats;
    }

    if ( modeCount > 0 )
    {
        details->modes = ( VkPresentModeKHR * ) malloc (
            modeCount * sizeof ( VkPresentModeKHR ) );
        if ( ! details->modes )
        {
            _DEBUG_P ( "error: malloc failed for present modes\n" );
            goto defer_free_formats;
        }

        if ( vkGetPhysicalDeviceSurfacePresentModesKHR (
                 engine->physicalDevice,
                 engine->surface,
                 &modeCount,
                 details->modes ) != VK_SUCCESS )
        {
            _DEBUG_P ( "error: failed to get present modes\n" );
            goto defer_free_modes;
        }
    }
    details->modeCount = modeCount;

    result = VK_SUCCESS;

defer_free_modes:
    if ( result ) free ( details->modes );

defer_free_formats:
    if ( result ) free ( details->formats );

    return result;
}

int32_t
findGraphicsQueueFamily ( Engine * engine )
{

    VkBool32 presentSupport = false;
    for ( uint32_t i = 0; i < engine->queueFamilies->count; i++ )
    {
        if ( vkGetPhysicalDeviceSurfaceSupportKHR ( //
                 engine->physicalDevice,
                 i,
                 engine->surface,
                 &presentSupport ) )
            continue;
        if ( ( engine->queueFamilies->queues[ i ].queueFlags &
               VK_QUEUE_GRAPHICS_BIT ) &&
             presentSupport )
            return i;
    }
    return -1;
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
    appInfo.apiVersion              = VK_API_VERSION_1_3;
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo     = &appInfo;
    if ( engine->validationLayers.size )
        createInfo.pNext =
            ( VkDebugUtilsMessengerCreateInfoEXT * ) &debugMsgrCreateInfo;
    else { createInfo.pNext = NULL; }

    /* Fetch VK+GLFW Extensions */
    uint32_t extensionCount = 0;

    const char ** vkExtensions =
        glfwGetRequiredInstanceExtensions ( &extensionCount );
    if ( ! vkExtensions )
    {
        _DEBUG_P ( "error: getting extensions\n" );
        goto r_base;
    }

    const char ** vkExtensionsFull = ( const char ** ) malloc (
        ( extensionCount + engine->customInstanceExt.size ) *
        sizeof ( const char * ) );
    if ( ! vkExtensionsFull )
    {
        _DEBUG_P ( "error: malloc vkExtensions of size: %zu\n",
                   ( extensionCount + engine->customInstanceExt.size ) *
                       sizeof ( const char * ) );
        goto r_base;
    }
    /* defer: free(vkExtensionsFull) */

    for ( size_t i = 0; i < extensionCount; i++ )
        vkExtensionsFull[ i ] = vkExtensions[ i ];
    for ( size_t i = 0; i < engine->customInstanceExt.size; i++ )
        vkExtensionsFull[ extensionCount + i ] =
            engine->customInstanceExt.data[ i ];

    for ( uint32_t i = 0; i < extensionCount + engine->customInstanceExt.size;
          i++ )
    {
        _DEBUG_P ( "Ext: %s\n", vkExtensionsFull[ i ] );
    }

    createInfo.enabledExtensionCount =
        extensionCount + engine->customInstanceExt.size;
    createInfo.ppEnabledExtensionNames = vkExtensionsFull;

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

    for ( size_t i = 0; i < engine->validationLayers.size; i++ )
    {
        uint8_t      layerFound = 0;
        const char * layerName  = engine->validationLayers.data[ i ];
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

    if ( engine->validationLayers.size )
    {
        createInfo.enabledLayerCount   = engine->validationLayers.size;
        createInfo.ppEnabledLayerNames = engine->validationLayers.data;
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
    if ( engine->validationLayers.size )
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

    /* Create VK+GLFW Surface */
    if ( glfwCreateWindowSurface ( engine->vkInstance,
                                   engine->window,
                                   NULL,
                                   &( engine->surface ) ) != VK_SUCCESS )
    {
        _DEBUG_P ( "error: failed to create VK surface\n" );
        goto defer_destroy_degub_msgr;
    }
    /* defer: vkDestroySurface */

    /* Physical Device Selection */

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices ( engine->vkInstance, &deviceCount, NULL );
    if ( deviceCount == 0 )
    {
        _DEBUG_P ( "error: no physical devices found" );
        goto defer_destroySurface;
    }

    VkPhysicalDevice * devices =
        malloc ( deviceCount * sizeof ( VkPhysicalDevice ) );

    if ( ! devices )
    {
        _DEBUG_P ( "error: malloc devices of size : %zu\n",
                   deviceCount * sizeof ( VkPhysicalDevice ) );
        goto defer_destroySurface;
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
        _DEBUG_P ( "Device found: %s of type %d and %d\n",
                   deviceProperties.deviceName,
                   deviceProperties.deviceType,
                   deviceFeatures.geometryShader );
        // TODO: check extensions compability and support
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
        _DEBUG_P ( "error: getting Physical Devices : no suitable found\n" );
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
    uint32_t graphicsFamilyIdx = findGraphicsQueueFamily ( engine );
    if ( graphicsFamilyIdx == -1 )
    {
        _DEBUG_P ( "error: suitable queue Family not fount\n" );
        goto defer_free_queueFamilies;
    }
    engine->graphicsQueueIdx = graphicsFamilyIdx;
    engine->presentQueueIdx  = graphicsFamilyIdx;

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
    logDeviceInfo.enabledExtensionCount   = engine->customDeviceExt.size;
    logDeviceInfo.ppEnabledExtensionNames = engine->customDeviceExt.data;

    if ( engine->validationLayers.size )
    {
        logDeviceInfo.enabledLayerCount   = engine->validationLayers.size;
        logDeviceInfo.ppEnabledLayerNames = engine->validationLayers.data;
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
    vkGetDeviceQueue (
        engine->device, graphicsFamilyIdx, 0, &( engine->presentQueue ) );

    querySwapChainSupport ( engine );

    rcode = VK_SUCCESS;

defer_free_swapChainSupportDetails:
    if ( rcode ) free ( engine->swapChainDetails.formats );
    if ( rcode ) free ( engine->swapChainDetails.modes );
defer_free_queueFamilies:
    if ( rcode ) free ( queuesFamilies );
defer_free_queues:
    if ( rcode ) free ( queues );
defer_free_devices:
    free ( devices );
defer_destroySurface:
    if ( rcode )
        vkDestroySurfaceKHR ( engine->vkInstance, engine->surface, NULL );
defer_destroy_degub_msgr:
    if ( rcode && engine->validationLayers.size )
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
    free ( vkExtensionsFull );
r_base:
    return rcode;
}

VkResult
CringedSwapChain ( Engine * engine )
{
    VkResult opResult, rcode = VK_INCOMPLETE;

    int win_width, win_height;
    glfwGetFramebufferSize ( engine->window, &win_width, &win_height );

    engine->swapChainConfig = chooseBestSwapChainConfig (
        &( engine->swapChainDetails ), win_width, win_height );

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType         = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface       = engine->surface;
    createInfo.minImageCount = engine->swapChainConfig.imageCount;
    createInfo.imageFormat   = engine->swapChainConfig.surfaceFormat.format;
    createInfo.imageColorSpace =
        engine->swapChainConfig.surfaceFormat.colorSpace;
    createInfo.imageExtent      = engine->swapChainConfig.extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform =
        engine->swapChainDetails.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode    = engine->swapChainConfig.presentMode;
    createInfo.oldSwapchain   = VK_NULL_HANDLE;

    createInfo.clipped = VK_TRUE;

    uint32_t queueFamilyIndices[] = { engine->graphicsQueueIdx,
                                      engine->presentQueueIdx };
    if ( engine->graphicsQueueIdx != engine->presentQueueIdx )
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount =
            sizeof ( queueFamilyIndices ) / sizeof ( uint32_t );
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices   = NULL;
    }

    if ( ( opResult = vkCreateSwapchainKHR ( //
               engine->device,
               &createInfo,
               NULL,
               &( engine->swapChain ) ) ) != VK_SUCCESS )
    {
        _DEBUG_P ( "error: swapChain creation failed: %d", opResult );
        goto r_base;
    }
    /* defer: vkDestroy(swapChain) */

    uint32_t imageCount = 0;
    if ( ( opResult = vkGetSwapchainImagesKHR (
               engine->device, engine->swapChain, &imageCount, NULL ) ) !=
             VK_SUCCESS ||
         ( ! imageCount ) )
    {
        _DEBUG_P ( "error: retrieving VkImages count: %d and count %d",
                   opResult,
                   imageCount );
        goto defer_destroySwapChain;
    }
    engine->swapChainImages =
        ( VkImage * ) malloc ( imageCount * sizeof ( VkImage ) );
    if ( ! engine->swapChainImages )
    {
        _DEBUG_P ( "error: malloc Vkimages of size: %zu",
                   imageCount * sizeof ( VkImage ) );
        goto defer_destroySwapChain;
    }
    /* defer: ON FAIL: free engine->swapchainimages */

    if ( ( opResult = vkGetSwapchainImagesKHR ( engine->device,
                                                engine->swapChain,
                                                &imageCount,
                                                engine->swapChainImages ) ) !=
         VK_SUCCESS )
    {
        _DEBUG_P ( "error: retrieving VkImages: %d and count %d",
                   opResult,
                   imageCount );
        goto defer_free_images;
    }
    engine->swapChainImagesCount = imageCount;

    engine->swapChainImageViews = ( VkImageView * ) malloc (
        engine->swapChainImagesCount * sizeof ( VkImageView ) );
    if ( ! engine->swapChainImages )
    {
        _DEBUG_P ( "error: malloc VkImageViews of size: %zu",
                   engine->swapChainImagesCount * sizeof ( VkImageView ) );
        goto defer_free_images;
    }
    /* defer: ON FAIL: free engine->swapchainimageVIEWS */

    VkImageViewCreateInfo swIView_createinfo = {};
    swIView_createinfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    swIView_createinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    swIView_createinfo.format = engine->swapChainConfig.surfaceFormat.format;
    swIView_createinfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    swIView_createinfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    swIView_createinfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    swIView_createinfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    swIView_createinfo.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_COLOR_BIT;
    swIView_createinfo.subresourceRange.baseMipLevel   = 0;
    swIView_createinfo.subresourceRange.levelCount     = 1;
    swIView_createinfo.subresourceRange.baseArrayLayer = 0;
    swIView_createinfo.subresourceRange.layerCount     = 1;

    for ( uint32_t i = 0; i < engine->swapChainImagesCount; i++ )
    {

        swIView_createinfo.image = engine->swapChainImages[ i ];
        if ( ( opResult = vkCreateImageView (
                              engine->device,
                              &swIView_createinfo,
                              NULL,
                              &( engine->swapChainImageViews[ i ] ) ) !=
                          VK_SUCCESS ) )
        {
            _DEBUG_P ( "error: creating vkImageView for Image idx %d: %d",
                       i,
                       opResult );
            goto defer_free_image_views;
        }
    }

    rcode = VK_SUCCESS;
defer_free_image_views:
    if ( rcode ) free ( engine->swapChainImageViews );
defer_free_images:
    if ( rcode ) free ( engine->swapChainImages );
defer_destroySwapChain:
    if ( rcode )
        vkDestroySwapchainKHR ( engine->device, engine->swapChain, NULL );
r_base:
    return rcode;
}

/* =================================
 * G R A P H I C S   P I P E L I N E
 * =================================
 *        Vertex/Index buffer
 *                 │
 *                 │
 *                 ▼
 *       ┌────────────────────┐
 *       ││      Input       ││
 *       ││    assembler     ││
 *       └────────────────────┘
 *                 │
 *                 │
 *                 ▼
 *        ┌──────────────────┐
 *        │   Vertex Shader  │
 *        └────────┬─────────┘
 *                 │ Screen
 *                 │ space
 *                 ▼
 *        ┌──────────────────┐
 *        │   Tessellation   │
 *        └────────┬─────────┘
 *                 │ Subdivide
 *                 │ mesh
 *                 ▼
 *        x────────x─────────x
 *        │  Geometry Shader │
 *        x────────x─────────x
 *                 │ Culling
 *                 │ Divide
 *                 ▼
 *       ┌────────────────────┐
 *       ││  Rasterization   ││
 *       └─────────┬──────────┘
 *                 │ Fragments
 *                 │
 *                 ▼
 *        ┌──────────────────┐
 *        │  Fragment Shader │
 *        └────────┬─────────┘
 *                 │ Textures
 *                 │ Norms
 *                 ▼ Light
 *       ┌────────────────────┐
 *       ││  Color Blending  ││
 *       └─────────┬──────────┘
 *                 │ Blend
 *                 │
 *                 ▼
 *            Framebuffer
 */

VkResult
BasedGraphicsPipeline ( Engine * engine )
{
    VkResult opResult, rcode = VK_INCOMPLETE;

    engine->triVert = cringedCreateShader ( //
        build_shaders_Triangle_vert_spv,
        build_shaders_Triangle_vert_spv_len );
    if ( ! engine->triVert )
    {
        _DEBUG_P ( "error: failed to load shader Triangle_vert.spv\n" );
        goto r_base;
    }

    engine->triFrag = cringedCreateShader ( //
        build_shaders_Triangle_frag_spv,
        build_shaders_Triangle_frag_spv_len );
    if ( ! engine->triFrag )
    {
        _DEBUG_P ( "error: failed to load shader Triangle_frag.spv\n" );
        goto defer_cleanup;
    }

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    /* Create Vertex Shader Module & Stage */
    engine->triVert->s_module =
        ( VkShaderModule * ) malloc ( sizeof ( VkShaderModule ) );
    if ( ! engine->triVert->s_module )
    {
        _DEBUG_P ( "error: malloc triVert shadermodule of size: %zu\n",
                   sizeof ( VkShaderModule ) );
        goto defer_cleanup;
    }
    createInfo.codeSize = engine->triVert->size;
    createInfo.pCode    = ( uint32_t * ) engine->triVert->data;
    if ( ( opResult = vkCreateShaderModule ( //
               engine->device,
               &createInfo,
               NULL,
               engine->triVert->s_module ) ) != VK_SUCCESS )
    {
        free ( engine->triVert->s_module );
        engine->triVert->s_module = NULL;
        _DEBUG_P ( "error: creating triVert shadermodule: %d\n", opResult );
        goto defer_cleanup;
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = *( engine->triVert->s_module );
    vertShaderStageInfo.pName  = "main";

    /* Create Fragment Shader Module & Stage */
    engine->triFrag->s_module =
        ( VkShaderModule * ) malloc ( sizeof ( VkShaderModule ) );
    if ( ! engine->triFrag->s_module )
    {
        _DEBUG_P ( "error: malloc triFrag shadermodule of size: %zu\n",
                   sizeof ( VkShaderModule ) );
        goto defer_cleanup;
    }
    createInfo.codeSize = engine->triFrag->size;
    createInfo.pCode    = ( uint32_t * ) engine->triFrag->data;
    if ( ( opResult = vkCreateShaderModule ( //
               engine->device,
               &createInfo,
               NULL,
               engine->triFrag->s_module ) ) != VK_SUCCESS )
    {
        free ( engine->triFrag->s_module );
        engine->triFrag->s_module = NULL;
        _DEBUG_P ( "error: creating triFrag shadermodule: %d\n", opResult );
        goto defer_cleanup;
    }
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    vertShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    vertShaderStageInfo.module = *( engine->triFrag->s_module );
    vertShaderStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo,
                                                       fragShaderStageInfo };

    /* Vertex Shader input
     * TODO: no input for now */
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 0;
    vertexInputInfo.pVertexBindingDescriptions      = NULL;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions    = NULL;

    /* Input Assembly */
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    /* Viewport & Scissors Setup */
    VkOffset2D scissorsOffset = { 0, 0 };

    VkViewport viewport = {};
    viewport.x          = 0.0f;
    viewport.y          = 0.0f;
    viewport.width      = ( float ) engine->swapChainConfig.extent.width;
    viewport.height     = ( float ) engine->swapChainConfig.extent.height;
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;
    VkRect2D scissor    = {};
    scissor.offset      = scissorsOffset;
    scissor.extent      = engine->swapChainConfig.extent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;

    /* Rasterizer */

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_TRUE; /* clamp instead of discarding */
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp          = 0.0f;
    rasterizer.depthBiasSlopeFactor    = 0.0f;

    /* Antialiasing: Multisampling */
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable   = VK_FALSE;
    multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading      = 1.0f;
    multisampling.pSampleMask           = NULL;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable      = VK_FALSE;

    /* Color Blending: alpha blending enabled */
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable         = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable       = VK_FALSE;
    colorBlending.logicOp             = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount     = 1;
    colorBlending.pAttachments        = &colorBlendAttachment;
    colorBlending.blendConstants[ 0 ] = 0.0f;
    colorBlending.blendConstants[ 1 ] = 0.0f;
    colorBlending.blendConstants[ 2 ] = 0.0f;
    colorBlending.blendConstants[ 3 ] = 0.0f;

    /* Pipeline Layout */
    VkPipelineLayout pipelineLayout;
    engine->pipelineLayout = malloc ( sizeof ( VkPipelineLayout ) );
    if ( ! engine->pipelineLayout )
    {
        _DEBUG_P ( "error: malloc pipeline Layout of size: %zu\n",
                   sizeof ( VkPipelineLayout ) );
        goto defer_cleanup;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = 0;
    pipelineLayoutInfo.pSetLayouts            = NULL;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges    = NULL;

    if ( ( opResult = vkCreatePipelineLayout ( //
               engine->device,
               &pipelineLayoutInfo,
               NULL,
               engine->pipelineLayout ) ) != VK_SUCCESS )
    {
        free ( engine->pipelineLayout );
        engine->pipelineLayout = NULL;
        _DEBUG_P ( "error: creating PipelineLayout: %d\n", opResult );
        goto defer_cleanup;
    }
    /* defer: on fail: destroy pipeline layout */

    rcode = VK_SUCCESS;

defer_cleanup:
    if ( rcode ) BasedGraphicsPipelineCleanup ( engine );
r_base:
    return rcode;
}

VkResult
BasedGraphicsPipelineCleanup ( Engine * engine )
{
    if ( engine->pipelineLayout )
    {
        vkDestroyPipelineLayout (
            engine->device, *( engine->pipelineLayout ), NULL );
        free ( engine->pipelineLayout );
    }
    if ( engine->triFrag->s_module )
    {
        vkDestroyShaderModule (
            engine->device, *( engine->triFrag->s_module ), NULL );
        free ( engine->triFrag->s_module );
    }
    if ( engine->triVert->s_module )
    {
        vkDestroyShaderModule (
            engine->device, *( engine->triVert->s_module ), NULL );
        free ( engine->triVert->s_module );
    }
    if ( engine->triFrag ) cringedDestroyShader ( engine->triFrag );
    if ( engine->triVert ) cringedDestroyShader ( engine->triVert );
    return VK_SUCCESS;
}

VkResult
BasedSwapChainCleanup ( Engine * engine )
{

    free ( engine->swapChainImages );
    vkDestroySwapchainKHR ( engine->device, engine->swapChain, NULL );

    for ( size_t i = 0; i < engine->swapChainImagesCount; i++ )
        vkDestroyImageView (
            engine->device, engine->swapChainImageViews[ i ], NULL );
    return VK_SUCCESS;
}

VkResult
BasedVKCleanup ( Engine * engine )
{
    if ( engine->debugMessenger && engine->validationLayers.size )
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
    free ( engine->swapChainDetails.formats );
    free ( engine->swapChainDetails.modes );

    vkDestroySurfaceKHR ( engine->vkInstance, engine->surface, NULL );
    vkDestroyDevice ( engine->device, NULL );
    vkDestroyInstance ( engine->vkInstance, NULL );
    return VK_SUCCESS;
}
