#include "main.h"

uint8_t
BasedSDLInit ( Engine * engine )
{

    if ( SDL_Init ( SDL_INIT_VIDEO ) != 0 )
    {
        printf ( "SDL_Init Error: %s\n", SDL_GetError () );
        return 1;
    }

    engine->window = SDL_CreateWindow ( //
        "Sample VK window",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800,
        600,
        SDL_WINDOW_VULKAN );

    if ( ! engine->window )
    {
        printf ( "SDL_CreateWindow Error: %s\n", SDL_GetError () );
        SDL_Quit ();
        return 1;
    }
    return 0;
}

uint8_t
init ()
{
    if ( BasedSDLInit ( CRINGE_ENGINE ) ) return 1;

    if ( BasedVKInit ( CRINGE_ENGINE ) ) return 1;

    return 0;
}

uint8_t
mainLoop ()
{

    SDL_Event event;
    int       quit = 0;
    while ( ! quit )
    {
        while ( SDL_PollEvent ( &event ) )
        {
            if ( event.type == SDL_QUIT ) { quit = 1; }
        }
    }
    return 0;
}

uint8_t
cleanup ()
{
    BasedVKCleanup ( CRINGE_ENGINE );
    return 0;
}

Engine *
CringeInitEngine ( void )
{
    Engine * engine = ( Engine * ) malloc ( sizeof ( Engine ) );
    if ( engine == NULL ) { return NULL; }

    engine->physicalDevice   = VK_NULL_HANDLE;
    engine->validationLayers = layers;

#ifdef NDEBUG
    engine->validationLayerCount = 0;
#else
    engine->validationLayerCount = sizeof ( layers ) / sizeof ( layers[ 0 ] );
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

    if ( ( error = cleanup () ) )
    {
        printf ( "cleanup() failed: %d\n", error );
        goto exit_base;
    }

    rcode = 0;

exit_base:
    if ( CRINGE_ENGINE->window )
    {
        SDL_DestroyWindow ( CRINGE_ENGINE->window );
        SDL_Quit ();
    }
    free ( CRINGE_ENGINE );
    return rcode;
}
