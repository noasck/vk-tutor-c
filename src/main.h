#pragma once
#ifndef BASED_CODE_VK
#define BASED_CODE_VK

// NOTE: define to use software rendering:
// #define LLVM_MESA

// NOTE: define to remove validation layers:
// #define NDEBUG

#include "vkinit.h"

#include <cglm/cglm.h>
#include <stdint.h>
#include <stdio.h>
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// +------------------------------------------+
// |  Allocate memory for VK instance and etc |
// |     (on stack, afaik)                    |
// +------------------------------------------+

Engine * CRINGE_ENGINE;

Engine *
CringeInitEngine ( void );

const char * layers[]             = { "VK_LAYER_KHRONOS_validation" };
const char * instanceExtensions[] = {
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
};
const char * deviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

uint8_t
init ();

uint8_t
mainLoop ();

uint8_t
cleanup ();

#endif /* BASED_CODE_VK */
