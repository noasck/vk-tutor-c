#pragma once
#ifndef CRINGED_SHADER_UTILS_H
#define CRINGED_SHADER_UTILS_H

#define CRINGED_PATH_MAX 4096
#define CRINGED_DIR_SEP  '/'

#ifndef NDEBUG
#define _DEBUG_P( ... ) printf ( __VA_ARGS__ )
#else
#define _DEBUG_P( ... ) ( ( void ) 0 )
#endif

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

typedef struct
{
    unsigned char *  data;
    VkShaderModule * s_module;
    size_t           size;
} BasedShader;

BasedShader *
cringedCreateShader ( unsigned char * data, size_t size );

void
cringedDestroyShader ( BasedShader * shader );

#endif /* CRINGED_SHADER_UTILS_H */
