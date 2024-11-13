#pragma once
#ifndef BASED_CODE_VK_INIT_H
#define BASED_CODE_VK_INIT_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <cglm/cglm.h>
#include <stdint.h>
#include <stdio.h>
#include <vulkan/vulkan.h>

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
    SDL_Window *             window;
    VkInstance               vkInstance;
    VkDevice                 device;
    VkQueue                  graphicsQueue;
    QueueFamilies *          queueFamilies;
    VkPhysicalDevice         physicalDevice;
    VkDebugUtilsMessengerEXT debugMessenger;
    size_t                   validationLayerCount;
    const char **            validationLayers;
} Engine;

VkResult
BasedVKInit ( Engine * engine );

VkResult
BasedVKCleanup ( Engine * engine );

#endif /* BASED_CODE_VK_INIT_H */
