#include "main.h"

void
glfwErrorCallback ( int error, const char * description )
{
    _DEBUG_P ( "GLFW Error [%d]: %s\n", error, description );
}

uint8_t
BasedGLFWInit ( Engine * engine )
{
    glfwSetErrorCallback ( glfwErrorCallback );

    if ( ! glfwInit () )
    {
        printf ( "GLFW Init Error\n" );
        return 1;
    }

    glfwWindowHint ( GLFW_CLIENT_API, GLFW_NO_API );

    engine->window =
        glfwCreateWindow ( 800, 600, "Sample VK window", NULL, NULL );

    if ( ! engine->window )
    {
        printf ( "GLFW Create Window Error\n" );
        glfwTerminate ();
        return 1;
    }

    return 0;
}

uint8_t
init ()
{
    if ( BasedGLFWInit ( CRINGE_ENGINE ) ) return 1;

    if ( BasedVKInit ( CRINGE_ENGINE ) ) return 1;

    if ( CringedSwapChain ( CRINGE_ENGINE ) ) return 1;

    if ( BasedGraphicsPipeline ( CRINGE_ENGINE ) ) return 1;
    return 0;
}

uint8_t
mainLoop ()
{
    while ( ! glfwWindowShouldClose ( CRINGE_ENGINE->window ) )
    {
        glfwPollEvents ();
    }
    return 0;
}

uint8_t
cleanup ()
{
    BasedGraphicsPipelineCleanup ( CRINGE_ENGINE );
    BasedSwapChainCleanup ( CRINGE_ENGINE );
    BasedVKCleanup ( CRINGE_ENGINE );
    return 0;
}

Engine *
CringeInitEngine ( void )
{
    Engine * engine = ( Engine * ) malloc ( sizeof ( Engine ) );
    if ( engine == NULL ) { return NULL; }

    engine->physicalDevice = VK_NULL_HANDLE;

    engine->validationLayers.data  = layers;
    engine->customInstanceExt.data = instanceExtensions;
    engine->customDeviceExt.data   = deviceExtensions;

    engine->validationLayers.size =
        sizeof ( layers ) / sizeof ( layers[ 0 ] );
    engine->customInstanceExt.size =
        sizeof ( instanceExtensions ) / sizeof ( instanceExtensions[ 0 ] );
    engine->customDeviceExt.size =
        sizeof ( deviceExtensions ) / sizeof ( deviceExtensions[ 0 ] );

#ifdef NDEBUG
    engine->validationLayerCount = 0;
    engine->customExtensionCount = 0;
#endif

    return engine;
}

int
main ()
{
    int rcode     = 1;
    CRINGE_ENGINE = CringeInitEngine ();

    if ( ! CRINGE_ENGINE )
    {
        printf ( "Cringe failed\n" );
        return 1;
    }

    uint8_t error = 0;
    if ( ( error = init () ) )
    {
        printf ( "init() failed: %d\n", error );
        goto exit_base;
    }

    if ( ( error = mainLoop () ) )
    {
        printf ( "mainLoop() failed: %d\n", error );
        goto exit_base;
    }

    rcode = 0;

exit_base:
    cleanup ();

    if ( CRINGE_ENGINE->window )
    {
        glfwDestroyWindow ( CRINGE_ENGINE->window );
        glfwTerminate ();
    }
    free ( CRINGE_ENGINE );
    return rcode;
}
