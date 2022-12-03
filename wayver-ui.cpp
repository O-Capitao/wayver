#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include "spdlog/sinks/basic_file_sink.h"
#include <string>


#include "wayver-ui.hpp"


using namespace Wayver;

WayverUi::~WayverUi(){
        //Destroy window	
	SDL_DestroyRenderer( renderer );
	SDL_DestroyWindow( window );
    SDL_DestroyTexture(canvas);
    TTF_CloseFont( gFont );

	window = NULL;
	renderer = NULL;
    canvas = NULL;
    gFont = NULL;

    TTF_Quit();
	SDL_Quit();
}

void WayverUi::initUiState(
    int n_c,
    boost::lockfree::spsc_queue<float,boost::lockfree::capacity<W_QUEUE_SIZE>> *rdptr
){
    _logger = spdlog::basic_logger_mt("UI", "wayver.log");
    // this->_input_from_audio = ifa;
    
    this->_n_channels = n_c;
    this->_n_samples_in = _n_channels * _n_frames_per_buffer;
    this->_samples_in_vec.reserve( _n_samples_in );
    this->_rawDataTap_ptr = rdptr;

    

    _logger->debug("Finished Constructor");
}

void WayverUi::initWindow(){
    int sdlIsInitted = SDL_Init( SDL_INIT_VIDEO );

    assert(sdlIsInitted >= 0);
    
    window = SDL_CreateWindow( 
        "Wayver", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED,
        1024, 
        768, 
        SDL_WINDOW_SHOWN
    );
    assert( window != NULL );

    // Create vsynced renderer for window
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
    assert( renderer != NULL ); 

    SDL_RendererInfo info;

    /* Checking if this renderer supports target textures */
    SDL_GetRendererInfo(renderer, &info);
    int renderer_has_target_texture_support = info.flags & SDL_RENDERER_TARGETTEXTURE;

    SDL_SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );

        // setup Canvas texture
    canvas = SDL_CreateTexture(
        renderer, 
        SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_TARGET, 
        1024, 
        768
    );

    assert(canvas != NULL );

    int ttfInited = TTF_Init();
    assert(ttfInited >= 0);


    gFont = TTF_OpenFont( "assets/Instruction.ttf", 12 );
    
    // assert(gFont != NULL );
    if (gFont == NULL){
        std::string errMsg = SDL_GetError();
        throw std::runtime_error(errMsg);
    }
}

void WayverUi::run(){
    _logger->debug("Starting run()");

    // initWindow();
    std::vector<float> *frames = new std::vector<float>();
    float item = 0;
    // AccumFunctor fun(frames);
    auto consume_pending = [&] {
        while (_rawDataTap_ptr->pop(item)) {
            frames->push_back(item);
        }
    };

    // Main loop
    while (!_stop)
    {
        // check if new data ojn data tap exists
        // as soon aas no new data is available -> close the window
        int items_in_queue = _rawDataTap_ptr->read_available();
        _frames_counter += items_in_queue;

        consume_pending();

        _draw();

        boost::this_thread::sleep_for( boost::chrono::milliseconds( UI_WAIT_TIME ) );
    }

    frames->empty();
    delete frames;
    _logger->debug("Exiting run()");
}

void WayverUi::stop(){
    _logger->debug("Received STOP signal.");
    _stop = true;
}

void WayverUi::_draw(){
    SDL_Event e;
    //Handle events on queue
    while( SDL_PollEvent( &e ) != 0 )
    {
        //User requests quit
        if( e.type == SDL_QUIT )
        {
            _stop = true;
        }
    }

    //Clear screen
    SDL_SetRenderDrawColor( 
        renderer, 
        _backgroundColor.r, 
        _backgroundColor.g, 
        _backgroundColor.b, 
        _backgroundColor.a 
    );
    
    SDL_RenderClear( renderer );

    _draw_sampleCounter();

    SDL_RenderPresent(renderer);

}

void WayverUi::_draw_sampleCounter(){
    
    std::string _sample_counter_text = std::to_string( _frames_counter );

    SDL_Surface *textSurface = TTF_RenderText_Solid( gFont, _sample_counter_text.c_str(), _foregroundColor );
    SDL_Texture *textTexture = SDL_CreateTextureFromSurface( renderer, textSurface );

    SDL_Point _texture_size = _getSize( textTexture );

    SDL_Rect src = {
        .x = 0, .y=0, .w= _texture_size.x, .h=_texture_size.y
    };
    SDL_Rect tgt = {
        .x = 0, .y=0, .w=_texture_size.x, .h=_texture_size.y
    };


    SDL_RenderCopy(
        renderer,
        textTexture,
        &src,
        &tgt
    );

    SDL_FreeSurface( textSurface );
}

SDL_Point WayverUi::_getSize(SDL_Texture *texture) {
    SDL_Point size;
    SDL_QueryTexture(texture, NULL, NULL, &size.x, &size.y);
    return size;
}

