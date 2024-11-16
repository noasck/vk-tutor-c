#include "shaderUtils.h"

void
cringedDestroyShader ( BasedShader * shader )
{
    free ( shader );
}

BasedShader *
cringedCreateShader ( unsigned char * data, size_t size )
{
    BasedShader * shader =
        ( BasedShader * ) malloc ( sizeof ( BasedShader ) );
    if ( ! shader ) return NULL;
    shader->data = data;
    shader->size = size;
    return shader;
}
