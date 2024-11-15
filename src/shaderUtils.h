#pragma once
#ifndef CRINGED_SHADER_UTILS_H
#define CRINGED_SHADER_UTILS_H

#ifndef NDEBUG
#define _DEBUG_P( ... ) printf ( __VA_ARGS__ )
#else
#define _DEBUG_P( ... ) ( ( void ) 0 )
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *
readFileContent ( const char * filename, size_t * outSize );

uint8_t
cringedLoadShaders ( BasedShader ** shaders, size_t shaderCount );

typedef struct
{
    char *       data;
    const char * path;
    size_t       size;
} BasedShader;

#endif /* CRINGED_SHADER_UTILS_H */
