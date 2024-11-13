#pragma once
#ifndef BASED_CODE_VK
#define BASED_CODE_VK

// NOTE: define to use software rendering:
// #define LLVM_MESA

// NOTE: define to remove validation layers:
// #define NDEBUG

#include "vkinit.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <cglm/cglm.h>
#include <stdint.h>
#include <stdio.h>
#include <vulkan/vulkan.h>

// +------------------------------------------+
// |  Allocate memory for VK instance and etc |
// |     (on stack, afaik)                    |
// +------------------------------------------+

Engine * CRINGE_ENGINE;

Engine *
CringeInitEngine ( void );

const char * layers[] = { "VK_LAYER_KHRONOS_validation" };

uint8_t
init ();

uint8_t
mainLoop ();

uint8_t
cleanup ();

#endif /* BASED_CODE_VK */
