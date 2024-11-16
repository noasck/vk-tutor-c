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

/* I DONT NEED IT RIGHT NOW
BasedShader *
cringedLoadShader ( const char * shaderFilePath )
{
    BasedShader * shader =
        ( BasedShader * ) malloc ( sizeof ( BasedShader ) );

    shader->data = readFileContent ( shaderFilePath, &( shader->size ) );

    if ( ! shader )
    {
        free ( shader );
        return NULL;
    }

    return shader;
}

char *
readFileContent ( const char * filename, size_t * outSize )
{
    char fullPath[ CRINGED_PATH_MAX ];
    if ( getcwd ( fullPath, CRINGED_PATH_MAX ) == NULL )
    {
        _DEBUG_P ( "error: failed to get current directory %s\n", filename );
        return NULL;
    }

    size_t currentDirLen = strlen ( fullPath );
    size_t filenameLen   = strlen ( filename );

    if ( currentDirLen + filenameLen + 2 > CRINGED_PATH_MAX )
    {
        _DEBUG_P ( "File path is too long %s\n", filename );
        return NULL;
    }

    if ( currentDirLen > 0 &&
         fullPath[ currentDirLen - 1 ] != CRINGED_DIR_SEP )
    {
        fullPath[ currentDirLen ]     = CRINGED_DIR_SEP;
        fullPath[ currentDirLen + 1 ] = '\0';
        currentDirLen++;
    }

    strcat ( fullPath, filename );

    _DEBUG_P ( "helper: reading file %s\n", fullPath );

    FILE * file = fopen ( fullPath, "rb" );
    if ( ! file )
    {
        _DEBUG_P ( "error: failed to open file %s\n", filename );
        return NULL;
    }

    fseek ( file, 0, SEEK_END );
    long fileSize = ftell ( file );
    if ( fileSize < 0 )
    {
        _DEBUG_P ( "error: failed to determine file size %s\n", filename );
        fclose ( file );
        return NULL;
    }

    rewind ( file );

    char * buffer = ( char * ) malloc ( fileSize );
    if ( ! buffer )
    {
        _DEBUG_P ( "error: failed to allocate memory for %s of size %zu\n",
                   filename,
                   fileSize );
        fclose ( file );
        return NULL;
    }

    size_t bytesRead = fread ( buffer, 1, fileSize, file );
    if ( bytesRead != ( size_t ) fileSize )
    {
        _DEBUG_P ( "error: failed to read file %s\n", filename );
        free ( buffer );
        fclose ( file );
        return NULL;
    }

    fclose ( file );

    if ( outSize ) { *outSize = fileSize; }

    return buffer;
} */
