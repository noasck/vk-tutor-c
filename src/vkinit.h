#pragma once
#ifndef BASED_CODE_VK_INIT_H
#define BASED_CODE_VK_INIT_H

#include "shaderUtils.h"

#include <cglm/cglm.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifndef NDEBUG
#define _DEBUG_P( ... ) printf ( __VA_ARGS__ )
#else
#define _DEBUG_P( ... ) ( ( void ) 0 )
#endif

typedef struct
{
    VkQueueFamilyProperties * queues;
    uint32_t                  count;
} QueueFamilies;

typedef struct
{
    size_t        size;
    const char ** data;

} ConstParamArr;

typedef struct
{
    VkSurfaceFormatKHR *     formats;
    VkPresentModeKHR *       modes;
    VkSurfaceCapabilitiesKHR capabilities;
    size_t                   formatCount;
    size_t                   modeCount;
} SwapChainSupportDetails;

typedef struct
{
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR   presentMode;
    VkExtent2D         extent;
    uint32_t           imageCount;
} SwapChainConfig;

typedef struct
{
    VkInstance vkInstance;
    /* Platform Extension */
    GLFWwindow *     window;
    VkSurfaceKHR     surface;
    VkDevice         device;
    VkPhysicalDevice physicalDevice;
    /* Queues */
    QueueFamilies * queueFamilies;
    uint32_t        graphicsQueueIdx;
    VkQueue         graphicsQueue;
    uint32_t        presentQueueIdx;
    VkQueue         presentQueue;
    /* Extensions & Validation & Debug */
    VkDebugUtilsMessengerEXT debugMessenger;
    ConstParamArr            validationLayers;
    ConstParamArr            customInstanceExt;
    ConstParamArr            customDeviceExt;
    /*Swap Chain*/
    VkSwapchainKHR          swapChain;
    SwapChainSupportDetails swapChainDetails;
    SwapChainConfig         swapChainConfig;
    uint32_t                swapChainImagesCount;
    VkImage *               swapChainImages;
    VkImageView *           swapChainImageViews;
    /* Pipeline */
    /* triangle GLSL compiled shaders */
    BasedShader *      triVert;
    BasedShader *      triFrag;
    VkPipelineLayout * pipelineLayout;

} Engine;

VkResult
BasedVKInit ( Engine * engine );

VkResult
BasedVKCleanup ( Engine * engine );

VkResult
BasedSwapChainCleanup ( Engine * engine );

VkResult
CringedSwapChain ( Engine * engine );

VkResult
BasedGraphicsPipeline ( Engine * engine );

VkResult
BasedGraphicsPipelineCleanup ( Engine * engine );

#endif /* BASED_CODE_VK_INIT_H */
