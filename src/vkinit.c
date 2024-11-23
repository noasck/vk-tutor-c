#include "vkinit.h"

/* Load GLSL -> compiled SPIR-V Shaders embedded with xxd */
#include "resources/shaders/Triangle_frag.h"
#include "resources/shaders/Triangle_vert.h"

#ifndef NDEBUG
#define free( ptr )                                                     \
    do {                                                                \
        if ( ptr )                                                      \
        {                                                               \
            printf ( "try free( :%s )\n", #ptr );                       \
            free ( ptr );                                               \
            ptr = NULL;                                                 \
        }                                                               \
        else { printf ( "ERROR: NULL POINTER free ( :%s )\n", #ptr ); } \
    } while ( 0 )
#endif

#define U_ALLOC( var, type, len )                                  \
    do {                                                           \
        var = ( type * ) malloc ( ( len ) * sizeof ( type ) );     \
        if ( ! var )                                               \
        {                                                          \
            _DEBUG_P ( "error: ualloc of %s of size %zu",          \
                       #var,                                       \
                       ( size_t ) ( ( len ) * sizeof ( type ) ) ); \
            exit ( EXIT_FAILURE );                                 \
        }                                                          \
    } while ( 0 )

static inline int
u_strcmp ( const char * str1, const char * str2 )
{
    while ( *str1 && *str2 && *str1 == *str2 ) str1++, str2++;
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

int32_t
findGraphicsQueueFamily ( Engine * engine )
{

    VkBool32 presentSupport = false;
    for ( uint32_t i = 0; i < engine->queueFamilies->count; i++ )
    {
        if ( vkGetPhysicalDeviceSurfaceSupportKHR ( //
                 engine->physicalDevice,
                 i,
                 *engine->surface,
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
    VkResult                  rcode            = VK_INCOMPLETE;
    const char **             vkExtensionsFull = NULL;
    VkLayerProperties *       availableLayers  = NULL;
    VkPhysicalDevice *        devices          = NULL;
    VkQueueFamilyProperties * queues           = NULL;

    /* Debug logger instantiated */
    VkDebugUtilsMessengerCreateInfoEXT debugMsgrCreateInfo = {};
    debugMsgrCreateInfo.sType =
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugMsgrCreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
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
    appInfo.pEngineName             = "BiGBlantCheatx Engine";
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
        goto defer_cleanup;
    }

    U_ALLOC ( vkExtensionsFull,
              const char *,
              extensionCount + engine->customInstanceExt.size );

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
        goto defer_cleanup;
    }

    U_ALLOC(availableLayers, VkLayerProperties, layerCount);

    if ( ( opResult = vkEnumerateInstanceLayerProperties (
               &layerCount, availableLayers ) ) )
    {
        _DEBUG_P ( "error: getting available layers: %d\n", opResult );
        goto defer_cleanup;
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
            goto defer_cleanup;
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
    engine->vkInstance = malloc ( sizeof ( VkInstance ) );
    if ( ! engine->vkInstance )
    {
        _DEBUG_P ( "error: malloc vkInstance of size: %zu\n",
                   sizeof ( VkInstance ) );
        goto defer_cleanup;
    }
    if ( ( opResult =
               vkCreateInstance ( &createInfo, NULL, engine->vkInstance ) ) )
    {
        free ( engine->vkInstance );
        engine->vkInstance = NULL;
        _DEBUG_P ( "error: creating VK instance: %d\n", opResult );
        goto defer_cleanup;
    }

    /* Debug Messenger Creation */
    if ( engine->validationLayers.size )
    {
        engine->debugMessenger =
            malloc ( sizeof ( VkDebugUtilsMessengerEXT ) );
        if ( ! engine->debugMessenger )
        {
            _DEBUG_P ( "error: malloc debugMessenger of size: %zu\n",
                       sizeof ( VkDebugUtilsMessengerEXT ) );
            goto defer_cleanup;
        }
        PFN_vkCreateDebugUtilsMessengerEXT func =
            ( PFN_vkCreateDebugUtilsMessengerEXT ) vkGetInstanceProcAddr (
                *engine->vkInstance, "vkCreateDebugUtilsMessengerEXT" );
        if ( ! func )
        {
            _DEBUG_P ( "error: retrieving DebugUtilsEXT func: %d\n",
                       opResult );
            goto defer_cleanup;
        }
        if ( ( opResult = func ( *engine->vkInstance,
                                 &debugMsgrCreateInfo,
                                 NULL,
                                 engine->debugMessenger ) ) )
        {
            free ( engine->debugMessenger );
            engine->debugMessenger = NULL;
            _DEBUG_P ( "error: creating DebugUtilsMessenger: %d\n",
                       opResult );
            goto defer_cleanup;
        }
    }

    /* Create VK+GLFW Surface */

    engine->surface = malloc ( sizeof ( VkSurfaceKHR ) );
    if ( ! engine->surface )
    {
        _DEBUG_P ( "error: malloc surface of size: %zu\n",
                   sizeof ( VkSurfaceKHR ) );
        goto defer_cleanup;
    }
    if ( ( opResult = glfwCreateWindowSurface ( //
               *engine->vkInstance,
               engine->window,
               NULL,
               engine->surface ) ) != VK_SUCCESS )
    {
        free ( engine->surface );
        engine->surface = NULL;
        _DEBUG_P ( "error: failed to create VK surface: %d\n", opResult );
        goto defer_cleanup;
    }

    /* Physical Device Selection */

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices ( *engine->vkInstance, &deviceCount, NULL );
    if ( deviceCount == 0 )
    {
        _DEBUG_P ( "error: no physical devices found" );
        goto defer_cleanup;
    }

    devices = malloc ( deviceCount * sizeof ( VkPhysicalDevice ) );

    if ( ! devices )
    {
        _DEBUG_P ( "error: malloc devices of size : %zu\n",
                   deviceCount * sizeof ( VkPhysicalDevice ) );
        goto defer_cleanup;
    }

    if ( ( opResult = vkEnumeratePhysicalDevices (
               *engine->vkInstance, &deviceCount, devices ) ) )
    {
        _DEBUG_P ( "error: getting Physical Devices : %d\n", opResult );
        goto defer_cleanup;
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
        goto defer_cleanup;
    }

    /* Get Queue Families for current device */
    uint32_t queueFamilyCount = 0;

    vkGetPhysicalDeviceQueueFamilyProperties (
        engine->physicalDevice, &queueFamilyCount, NULL );

    if ( ! queueFamilyCount )
    {
        _DEBUG_P ( "error: no queues available on the device" );
        goto defer_cleanup;
    }

    queues = ( VkQueueFamilyProperties * ) malloc (
        queueFamilyCount * sizeof ( VkQueueFamilyProperties ) );
    if ( ! queues )
    {
        _DEBUG_P ( "error: malloc queues of size : %zu\n",
                   queueFamilyCount * sizeof ( VkQueueFamilyProperties ) );
        goto defer_cleanup;
    }

    vkGetPhysicalDeviceQueueFamilyProperties (
        engine->physicalDevice, &queueFamilyCount, queues );

    /* Save queues to engine instance */
    QueueFamilies * queuesFamilies =
        ( QueueFamilies * ) malloc ( sizeof ( QueueFamilies ) );

    if ( ! queuesFamilies )
    {
        _DEBUG_P ( "error: malloc queueFamilies struct of size : %zu\n",
                   sizeof ( QueueFamilies ) );
        goto defer_cleanup;
    }

    queuesFamilies->count  = queueFamilyCount;
    queuesFamilies->queues = queues;
    engine->queueFamilies  = queuesFamilies;

    /* Find suitable graphics queue */
    uint32_t graphicsFamilyIdx = findGraphicsQueueFamily ( engine );
    if ( graphicsFamilyIdx == -1 )
    {
        _DEBUG_P ( "error: suitable queue Family not fount\n" );
        goto defer_cleanup;
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

    engine->device = malloc ( sizeof ( VkDevice ) );
    if ( ! engine->device )
    {
        _DEBUG_P ( "error: malloc device of size: %zu\n",
                   sizeof ( VkDevice ) );
        goto defer_cleanup;
    }

    if ( ( opResult = vkCreateDevice ( //
               engine->physicalDevice,
               &logDeviceInfo,
               NULL,
               engine->device ) ) )
    {
        free ( engine->device );
        engine->device = NULL;
        _DEBUG_P ( "error: creating Logical Device : %d\n", opResult );
        goto defer_cleanup;
    }

    /* Get Vk Queues */

    vkGetDeviceQueue (
        *engine->device, graphicsFamilyIdx, 0, &( engine->graphicsQueue ) );
    vkGetDeviceQueue (
        *engine->device, graphicsFamilyIdx, 0, &( engine->presentQueue ) );

    rcode = VK_SUCCESS;

defer_cleanup:
    if ( rcode ) BasedVKCleanup ( engine );
    if ( devices )
    {
        free ( devices );
        devices = NULL;
    };
    if ( vkExtensionsFull )
    {
        free ( vkExtensionsFull );
        vkExtensionsFull = NULL;
    }
    if ( availableLayers )
    {
        free ( availableLayers );
        availableLayers = NULL;
    }
    return rcode;
}

VkResult
CringedCommandBuffer ( Engine * engine )
{

    VkResult opResult, rcode = VK_INCOMPLETE;

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = engine->graphicsQueueIdx;

    engine->commandPool =
        ( VkCommandPool * ) malloc ( sizeof ( VkCommandPool ) );
    if ( ! engine->commandPool )
    {
        _DEBUG_P ( "error: malloc command Pool: %zu\n",
                   sizeof ( VkCommandPool ) );
        goto defer_cleanup;
    }

    if ( ( opResult = vkCreateCommandPool (
               *engine->device, &poolInfo, NULL, engine->commandPool ) ) !=
         VK_SUCCESS )
    {
        free ( engine->commandPool );
        engine->commandPool = NULL;
        _DEBUG_P ( "error: creating commandPool: %d\n", opResult );
        goto defer_cleanup;
    }

    engine->commandBufferCount = engine->MaxFramesInFlight;

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = *engine->commandPool;
    allocInfo.level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = engine->commandBufferCount;

    engine->commandBuffer = ( VkCommandBuffer * ) malloc (
        engine->commandBufferCount * sizeof ( VkCommandBuffer ) );
    if ( ! engine->commandBuffer )
    {
        _DEBUG_P ( "error: malloc command Buffer: %zu\n",
                   engine->commandBufferCount * sizeof ( VkCommandBuffer ) );
        goto defer_cleanup;
    }

    if ( ( opResult = vkAllocateCommandBuffers (
               *engine->device, &allocInfo, engine->commandBuffer ) ) !=
         VK_SUCCESS )
    {
        free ( engine->commandBuffer );
        engine->commandBuffer = NULL;
        _DEBUG_P ( "error: creating commandBuffer: %d\n", opResult );
        goto defer_cleanup;
    }
    rcode = VK_SUCCESS;

defer_cleanup:
    if ( rcode ) CringedCommandBufferCleanup ( engine );
    return rcode;
}

VkResult
CringedCommandBufferCleanup ( Engine * engine )
{
    if ( engine->commandBuffer )
    {
        vkFreeCommandBuffers ( *engine->device,
                               *engine->commandPool,
                               engine->commandBufferCount,
                               engine->commandBuffer );
        free ( engine->commandBuffer );
        engine->commandBuffer = NULL;
    }
    if ( engine->commandPool )
    {
        vkDestroyCommandPool ( *engine->device, *engine->commandPool, NULL );
        free ( engine->commandPool );
        engine->commandPool = NULL;
    }
    return VK_SUCCESS;
}

VkResult
CringedSwapChain ( Engine * engine )
{
    VkResult opResult, rcode = VK_INCOMPLETE;

    /* Get SwapChain Supported Params */
    SwapChainSupportDetails * details = &engine->swapChainDetails;

    VkSurfaceCapabilitiesKHR capabilities;
    if ( ( opResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR (
               engine->physicalDevice, *engine->surface, &capabilities ) ) !=
         VK_SUCCESS )
    {
        _DEBUG_P ( "error: failed to query surface capabilities: %d\n",
                   opResult );
        goto defer_cleanup;
    }
    details->capabilities = capabilities;

    uint32_t formatCount = 0;
    if ( ( opResult =
               vkGetPhysicalDeviceSurfaceFormatsKHR ( engine->physicalDevice,
                                                      *engine->surface,
                                                      &formatCount,
                                                      NULL ) ) != VK_SUCCESS )
    {
        _DEBUG_P ( "error: failed to get surface format count: %d\n",
                   opResult );
        goto defer_cleanup;
    }

    if ( formatCount > 0 )
    {
        details->formats = ( VkSurfaceFormatKHR * ) malloc (
            formatCount * sizeof ( VkSurfaceFormatKHR ) );
        if ( ! details->formats )
        {
            _DEBUG_P ( "error: malloc failed for formats\n" );
            goto defer_cleanup;
        }

        if ( vkGetPhysicalDeviceSurfaceFormatsKHR ( engine->physicalDevice,
                                                    *engine->surface,
                                                    &formatCount,
                                                    details->formats ) !=
             VK_SUCCESS )
        {
            _DEBUG_P ( "error: failed to get surface formats\n" );
            goto defer_cleanup;
        }
    }
    details->formatCount = formatCount;

    uint32_t modeCount = 0;
    if ( vkGetPhysicalDeviceSurfacePresentModesKHR (
             engine->physicalDevice, *engine->surface, &modeCount, NULL ) !=
         VK_SUCCESS )
    {
        _DEBUG_P ( "error: failed to get present mode count\n" );
        goto defer_cleanup;
    }

    if ( modeCount > 0 )
    {
        details->modes = ( VkPresentModeKHR * ) malloc (
            modeCount * sizeof ( VkPresentModeKHR ) );
        if ( ! details->modes )
        {
            _DEBUG_P ( "error: malloc failed for present modes\n" );
            goto defer_cleanup;
        }

        if ( vkGetPhysicalDeviceSurfacePresentModesKHR (
                 engine->physicalDevice,
                 *engine->surface,
                 &modeCount,
                 details->modes ) != VK_SUCCESS )
        {
            _DEBUG_P ( "error: failed to get present modes\n" );
            goto defer_cleanup;
        }
    }
    details->modeCount = modeCount;

    int win_width, win_height;
    glfwGetFramebufferSize ( engine->window, &win_width, &win_height );

    engine->swapChainConfig = chooseBestSwapChainConfig (
        &( engine->swapChainDetails ), win_width, win_height );

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType         = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface       = *engine->surface;
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

    engine->swapChain = malloc ( sizeof ( VkSwapchainKHR ) );
    if ( ! engine->swapChain )
    {
        _DEBUG_P ( "error: malloc swapChain of size: %zu",
                   sizeof ( VkSwapchainKHR ) );
        goto defer_cleanup;
    }
    if ( ( opResult = vkCreateSwapchainKHR ( //
               *engine->device,
               &createInfo,
               NULL,
               engine->swapChain ) ) != VK_SUCCESS )
    {
        free ( engine->swapChain );
        engine->swapChain = NULL;
        _DEBUG_P ( "error: swapChain creation failed: %d", opResult );
        goto defer_cleanup;
    }

    uint32_t imageCount = 0;
    if ( ( opResult = vkGetSwapchainImagesKHR ( //
               *engine->device,
               *( engine->swapChain ),
               &imageCount,
               NULL ) ) != VK_SUCCESS ||
         ( ! imageCount ) )
    {
        _DEBUG_P ( "error: retrieving VkImages count: %d and count %d",
                   opResult,
                   imageCount );
        goto defer_cleanup;
    }
    engine->swapChainImages =
        ( VkImage * ) malloc ( imageCount * sizeof ( VkImage ) );
    if ( ! engine->swapChainImages )
    {
        _DEBUG_P ( "error: malloc Vkimages of size: %zu",
                   imageCount * sizeof ( VkImage ) );
        goto defer_cleanup;
    }

    if ( ( opResult = vkGetSwapchainImagesKHR ( //
               *engine->device,
               *( engine->swapChain ),
               &imageCount,
               engine->swapChainImages ) ) != VK_SUCCESS )
    {
        _DEBUG_P ( "error: retrieving VkImages: %d and count %d",
                   opResult,
                   imageCount );
        goto defer_cleanup;
    }
    engine->swapChainImagesCount = imageCount;

    engine->swapChainImageViews = ( VkImageView * ) malloc (
        engine->swapChainImagesCount * sizeof ( VkImageView ) );
    if ( ! engine->swapChainImages )
    {
        _DEBUG_P ( "error: malloc VkImageViews of size: %zu",
                   engine->swapChainImagesCount * sizeof ( VkImageView ) );
        goto defer_cleanup;
    }

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
                              *engine->device,
                              &swIView_createinfo,
                              NULL,
                              &( engine->swapChainImageViews[ i ] ) ) !=
                          VK_SUCCESS ) )
        {
            /* Current one failed:
             * During cleanup need to vkDestroy + dealloc previous 'i' */
            engine->swapChainImagesCount = i;
            _DEBUG_P ( "error: creating vkImageView for Image idx %d: %d",
                       i,
                       opResult );
            goto defer_cleanup;
        }
    }

    rcode = VK_SUCCESS;
defer_cleanup:
    if ( rcode ) CringedSwapChainCleanup ( engine );
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
BasedSyncSetup ( Engine * engine )
{
    VkResult opResult, rcode = VK_INCOMPLETE;

    engine->sync = malloc ( engine->MaxFramesInFlight *
                            sizeof ( BasedSynchronization ) );
    if ( ! engine->sync )
    {
        _DEBUG_P ( "error: malloc sync array of size: %zu",
                   engine->MaxFramesInFlight *
                       sizeof ( BasedSynchronization ) );
        goto defer_cleanup;
    }

    for ( uint32_t m = 0; m < engine->MaxFramesInFlight; m++ )
    {
        VkSemaphore ** semaphores[] = {
            &( engine->sync[ m ].imageAvailable ),
            &( engine->sync[ m ].renderFinished ) };
        VkFence ** fences[] = { &( engine->sync[ m ].inFlight ) };

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags             = VK_FENCE_CREATE_SIGNALED_BIT;
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for ( uint32_t i = 0;
              i < sizeof ( semaphores ) / sizeof ( semaphores[ 0 ] );
              i++ )
        {
            *semaphores[ i ] = malloc ( sizeof ( VkSemaphore ) );
            if ( ! *semaphores[ i ] )
            {
                engine->syncCount = i + 1;
                _DEBUG_P ( "error: malloc semaphore of size: %zu\n",
                           sizeof ( VkSemaphore ) );
                goto defer_cleanup;
            }

            if ( ( opResult = vkCreateSemaphore ( //
                       *engine->device,
                       &semaphoreInfo,
                       NULL,
                       *semaphores[ i ] ) ) != VK_SUCCESS )
            {
                engine->syncCount = i + 1;
                free ( *semaphores[ i ] );
                *semaphores[ i ] = NULL;
                _DEBUG_P ( "error: creating frame semaphore [%d]: code %d\n",
                           i,
                           opResult );
                goto defer_cleanup;
            }
        }

        for ( uint32_t i = 0; i < sizeof ( fences ) / sizeof ( fences[ 0 ] );
              i++ )
        {
            *fences[ i ] = malloc ( sizeof ( VkFence ) );
            if ( ! *fences[ i ] )
            {
                engine->syncCount = i + 1;
                _DEBUG_P ( "error: malloc Fence of size: %zu\n",
                           sizeof ( VkFence ) );
                goto defer_cleanup;
            }

            if ( ( opResult = vkCreateFence ( //
                       *engine->device,
                       &fenceInfo,
                       NULL,
                       *fences[ i ] ) ) != VK_SUCCESS )
            {
                engine->syncCount = i + 1;
                free ( *fences[ i ] );
                *fences[ i ] = NULL;
                _DEBUG_P ( "error: creating frame fence [%d]: code %d\n",
                           i,
                           opResult );
                goto defer_cleanup;
            }
        }
    }
    engine->syncCount = engine->MaxFramesInFlight;

    rcode = VK_SUCCESS;

defer_cleanup:
    if ( rcode ) BasedGraphicsPipelineCleanup ( engine );
    return rcode;
}

VkResult
BasedSyncCleanup ( Engine * engine )
{

    if ( engine->sync )
        for ( uint32_t m = 0; m < engine->syncCount; m++ )
        {
            VkSemaphore ** semaphores[] = {
                &( engine->sync[ m ].imageAvailable ),
                &( engine->sync[ m ].renderFinished ) };
            VkFence ** fences[] = { &( engine->sync[ m ].inFlight ) };
            for ( uint32_t i = 0;
                  i < sizeof ( semaphores ) / sizeof ( semaphores[ 0 ] );
                  i++ )
            {

                if ( *semaphores[ i ] )
                {
                    vkDestroySemaphore (
                        *engine->device, **semaphores[ i ], NULL );
                    free ( *semaphores[ i ] );
                    *semaphores[ i ] = NULL;
                }
            }

            for ( uint32_t i = 0;
                  i < sizeof ( fences ) / sizeof ( fences[ 0 ] );
                  i++ )
            {

                if ( *fences[ i ] )
                {
                    vkDestroyFence ( *engine->device, **fences[ i ], NULL );
                    free ( *fences[ i ] );
                    *fences[ i ] = NULL;
                }
            }
        }

    if ( engine->sync )
    {
        free ( engine->sync );
        engine->sync = NULL;
    }
    return VK_SUCCESS;
}

VkResult
CringedFrameBuffers ( Engine * engine )
{
    VkResult opResult, rcode = VK_INCOMPLETE;

    engine->swapChainFrameBufferCount = engine->swapChainImagesCount;
    engine->swapChainFrameBuffers     = malloc (
        engine->swapChainFrameBufferCount * sizeof ( VkFramebuffer ) );
    if ( ! engine->swapChainFrameBuffers )
    {

        _DEBUG_P ( "error: malloc framebuffers of size: %zu\n",
                   engine->swapChainFrameBufferCount *
                       sizeof ( VkFramebuffer ) );
        goto defer_cleanup;
    }

    for ( uint32_t i = 0; i < engine->swapChainFrameBufferCount; i++ )
    {
        VkImageView attachments[] = { engine->swapChainImageViews[ i ] };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = *engine->renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments    = attachments;
        framebufferInfo.width  = engine->swapChainConfig.extent.width;
        framebufferInfo.height = engine->swapChainConfig.extent.height;
        framebufferInfo.layers = 1;

        if ( ( opResult = vkCreateFramebuffer ( //
                   *engine->device,
                   &framebufferInfo,
                   NULL,
                   engine->swapChainFrameBuffers + i ) ) != VK_SUCCESS )
        {
            /* Processed prevous `i` framebuffers, need to destroy. */
            engine->swapChainFrameBufferCount = i;
            _DEBUG_P (
                "error: creating frame buffer [%d]: code %d\n", i, opResult );
            goto defer_cleanup;
        }
    }

    rcode = VK_SUCCESS;

defer_cleanup:
    if ( rcode ) BasedGraphicsPipelineCleanup ( engine );
    return rcode;
}

VkResult
CringedFrameBuffersCleanup ( Engine * engine )
{
    if ( engine->swapChainFrameBuffers )
    {

        for ( size_t i = 0; i < engine->swapChainFrameBufferCount; i++ )
        {

            vkDestroyFramebuffer (
                *engine->device, engine->swapChainFrameBuffers[ i ], NULL );
        }

        free ( engine->swapChainFrameBuffers );
        engine->swapChainFrameBuffers = NULL;
    }
    return VK_SUCCESS;
}

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
        goto defer_cleanup;
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
               *engine->device,
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
               *engine->device,
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
    fragShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = *( engine->triFrag->s_module );
    fragShaderStageInfo.pName  = "main";

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
               *engine->device,
               &pipelineLayoutInfo,
               NULL,
               engine->pipelineLayout ) ) != VK_SUCCESS )
    {
        free ( engine->pipelineLayout );
        engine->pipelineLayout = NULL;
        _DEBUG_P ( "error: creating PipelineLayout: %d\n", opResult );
        goto defer_cleanup;
    }

    /* Render Pass */

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format  = engine->swapChainConfig.surfaceFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment            = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorAttachmentRef;

    /* NOTE: dep to wait until image ready */
    VkSubpassDependency dependency = {};
    dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass          = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments    = &colorAttachment;
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &dependency;

    engine->renderPass = malloc ( sizeof ( VkRenderPass ) );
    if ( ! engine->renderPass )
    {
        _DEBUG_P ( "error: malloc render pass of size: %zu\n",
                   sizeof ( VkRenderPass ) );
        goto defer_cleanup;
    }
    if ( ( opResult = vkCreateRenderPass ( //
               *engine->device,
               &renderPassInfo,
               NULL,
               engine->renderPass ) ) != VK_SUCCESS )
    {
        free ( engine->renderPass );
        engine->renderPass = NULL;
        _DEBUG_P ( "error: creating RenderPass: %d\n", opResult );
        goto defer_cleanup;
    }

    /* GRAPHICS PIPELINE */
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages    = shaderStages;
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = NULL;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = NULL;
    pipelineInfo.layout              = *engine->pipelineLayout;
    pipelineInfo.renderPass          = *engine->renderPass;
    pipelineInfo.subpass             = 0;
    pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex   = -1;

    engine->pipeline = malloc ( sizeof ( VkPipeline ) );
    if ( ! engine->pipeline )
    {
        _DEBUG_P ( "error: malloc Graphics Pipeline of size: %zu\n",
                   sizeof ( VkPipeline ) );
        goto defer_cleanup;
    }
    if ( ( opResult = vkCreateGraphicsPipelines ( //
               *engine->device,
               VK_NULL_HANDLE,
               1,
               &pipelineInfo,
               NULL,
               engine->pipeline ) ) != VK_SUCCESS )
    {
        free ( engine->pipeline );
        engine->pipeline = NULL;
        _DEBUG_P ( "error: creating Graphics Pipeline: %d\n", opResult );
        goto defer_cleanup;
    }

    rcode = VK_SUCCESS;

defer_cleanup:
    if ( rcode ) BasedGraphicsPipelineCleanup ( engine );
    return rcode;
}

VkResult
BasedGraphicsPipelineCleanup ( Engine * engine )
{
    if ( engine->pipeline )
    {
        vkDestroyPipeline ( *engine->device, *engine->pipeline, NULL );
        free ( engine->pipeline );
        engine->pipeline = NULL;
    }
    if ( engine->renderPass )
    {
        vkDestroyRenderPass (
            *engine->device, *( engine->renderPass ), NULL );
        free ( engine->renderPass );
        engine->renderPass = NULL;
    }
    if ( engine->pipelineLayout )
    {
        vkDestroyPipelineLayout (
            *engine->device, *( engine->pipelineLayout ), NULL );
        free ( engine->pipelineLayout );
        engine->pipelineLayout = NULL;
    }
    if ( engine->triFrag->s_module )
    {
        vkDestroyShaderModule (
            *engine->device, *( engine->triFrag->s_module ), NULL );
        free ( engine->triFrag->s_module );
        engine->triFrag->s_module = NULL;
    }
    if ( engine->triVert->s_module )
    {
        vkDestroyShaderModule (
            *engine->device, *( engine->triVert->s_module ), NULL );
        free ( engine->triVert->s_module );
        engine->triVert->s_module = NULL;
    }
    if ( engine->triFrag ) cringedDestroyShader ( engine->triFrag );
    if ( engine->triVert ) cringedDestroyShader ( engine->triVert );
    return VK_SUCCESS;
}

void
CringedSwapChainRecreate ( Engine * engine )
{
    int width = 0, height = 0;
    glfwGetFramebufferSize ( engine->window, &width, &height );
    while ( width == 0 || height == 0 )
    {
        glfwGetFramebufferSize ( engine->window, &width, &height );
        glfwWaitEvents ();
    }
    vkDeviceWaitIdle ( *engine->device );

    /* Cleanup */
    CringedFrameBuffersCleanup ( engine );
    CringedSwapChainCleanup ( engine );

    /* Recreate */
    CringedSwapChain ( engine );
    CringedFrameBuffers ( engine );
}

VkResult
CringedSwapChainCleanup ( Engine * engine )
{
    if ( engine->swapChainDetails.formats )
    {
        free ( engine->swapChainDetails.formats );
        engine->swapChainDetails.formats = NULL;
    }
    if ( engine->swapChainDetails.modes )
    {
        free ( engine->swapChainDetails.modes );
        engine->swapChainDetails.modes = NULL;
    }

    if ( engine->swapChainImages )
    {
        free ( engine->swapChainImages );
        engine->swapChainImages = NULL;
    }
    if ( engine->swapChain )
    {
        vkDestroySwapchainKHR (
            *engine->device, *( engine->swapChain ), NULL );
        free ( engine->swapChain );
        engine->swapChain = NULL;
    }
    if ( engine->swapChainImageViews )
    {
        for ( size_t i = 0; i < engine->swapChainImagesCount; i++ )
            vkDestroyImageView (
                *engine->device, engine->swapChainImageViews[ i ], NULL );
        free ( engine->swapChainImageViews );
        engine->swapChainImageViews = NULL;
    }
    return VK_SUCCESS;
}

VkResult
BasedVKCleanup ( Engine * engine )
{
    if ( engine->debugMessenger )
    {
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            ( PFN_vkDestroyDebugUtilsMessengerEXT ) vkGetInstanceProcAddr (
                *engine->vkInstance, "vkDestroyDebugUtilsMessengerEXT" );
        if ( func != NULL )
        {
            func ( *engine->vkInstance, *engine->debugMessenger, NULL );
        }
        free ( engine->debugMessenger );
        engine->debugMessenger = NULL;
    }
    if ( engine->queueFamilies )
    {
        if ( engine->queueFamilies->queues )
            free ( engine->queueFamilies->queues );
        free ( engine->queueFamilies );
        engine->queueFamilies = NULL;
    }
    if ( engine->surface )
    {
        vkDestroySurfaceKHR ( *engine->vkInstance, *engine->surface, NULL );
        free ( engine->surface );
        engine->surface = NULL;
    }
    if ( engine->device )
    {
        vkDestroyDevice ( *engine->device, NULL );
        free ( engine->device );
        engine->device = NULL;
    }
    if ( engine->vkInstance )
    {
        vkDestroyInstance ( *engine->vkInstance, NULL );
        free ( engine->vkInstance );
        engine->vkInstance = NULL;
    }
    return VK_SUCCESS;
}

/* =============================================
 *            COMMAND BUFFER UTILS
 * ============================================= */

VkResult
CringedRecordCommandBuffer ( Engine *          engine,
                             VkCommandBuffer * commandBuffer,
                             uint32_t          imageIndex )
{
    VkResult opResult, rcode = VK_INCOMPLETE;

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags            = 0;
    beginInfo.pInheritanceInfo = NULL;
    if ( ( opResult = vkBeginCommandBuffer ( *commandBuffer, &beginInfo ) ) !=
         VK_SUCCESS )
    {
        _DEBUG_P ( "error: beginCommandBuffer: %d\n", opResult );
        goto abort;
    }

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass  = *engine->renderPass;
    renderPassInfo.framebuffer = engine->swapChainFrameBuffers[ imageIndex ];
    renderPassInfo.renderArea.offset.x = 0;
    renderPassInfo.renderArea.offset.y = 0;
    renderPassInfo.renderArea.extent   = engine->swapChainConfig.extent;
    VkClearValue clearColor            = {
                   .color = { .float32 = { 0.0f, 0.0f, 0.0f, 1.0f } } };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues    = &clearColor;
    vkCmdBeginRenderPass (
        *commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
    vkCmdBindPipeline (
        *commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *engine->pipeline );

    /* NOTE: Viewport and Scissors are static. No need to set up. */
    vkCmdDraw ( *commandBuffer, 3, 1, 0, 0 );
    vkCmdEndRenderPass ( *commandBuffer );
    if ( ( opResult = vkEndCommandBuffer ( *commandBuffer ) ) != VK_SUCCESS )
    {
        _DEBUG_P ( "error: endCommandBuffer: %d\n", opResult );
        goto abort;
    }

    rcode = VK_SUCCESS;
abort:
    return rcode;
}
