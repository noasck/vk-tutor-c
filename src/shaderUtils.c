#include "shaderUtils.h"

uint8_t
cringedLoadShaders ( BasedShader ** shaders, size_t shaderCount )
{
    for ( size_t i = 0; i < shaderCount; ++i )
    {
        shaders[ i ]->data =
            readFileContent ( shaders[ i ]->path, &shaders[ i ]->size );
        if ( ! shaders[ i ]->data )
        {
            for ( size_t j = 0; j < i; ++j )
            {
                free ( shaders[ j ]->data );
                shaders[ j ]->data = NULL;
            }
            return 1;
        }
    }
    return 0;
}

char *
readFileContent ( const char * filename, size_t * outSize )
{
    FILE * file = fopen ( filename, "rb" );
    if ( ! file )
    {
        _DEBUG_P ( "Failed to open file" );
        return NULL;
    }

    fseek ( file, 0, SEEK_END );
    long fileSize = ftell ( file );
    if ( fileSize < 0 )
    {
        _DEBUG_P ( "Failed to determine file size" );
        fclose ( file );
        return NULL;
    }

    rewind ( file );

    char * buffer = ( char * ) malloc ( fileSize );
    if ( ! buffer )
    {
        _DEBUG_P ( "Failed to allocate memory" );
        fclose ( file );
        return NULL;
    }

    size_t bytesRead = fread ( buffer, 1, fileSize, file );
    if ( bytesRead != ( size_t ) fileSize )
    {
        _DEBUG_P ( "Failed to read file" );
        free ( buffer );
        fclose ( file );
        return NULL;
    }

    fclose ( file );

    if ( outSize ) { *outSize = fileSize; }

    return buffer;
}
