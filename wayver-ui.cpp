#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include "spdlog/sinks/basic_file_sink.h"
#include <string>
#include <math.h>


#include "wayver-ui.hpp"


using namespace Wayver::UI;



WayverUi::WayverUi()
{
    /****
     * 
     *  Calc Layout Rectangles
     *  _________________________________________
     *  |____________  Scrubber  _______________|
     *  |  Info     |                           |
     *  |           |     Spectrum              |
     *  |___________|___________________________|
     * 
    */
    _scrubber_rect = { 
        _globals._PADDING, 
        _globals._PADDING, 
        _globals._WIN_SIZE.x - 2 * _globals._PADDING, 
        _globals._SCRUBBER_HEIGHT 
    };

    _info_rect = {
        _globals._PADDING, 
        _scrubber_rect.y + _scrubber_rect.h, 
        _globals._INFO_WIDTH,
        _globals._WIN_SIZE.y - _scrubber_rect.h - _scrubber_rect.y - _globals._PADDING
    };

    _spectrum_rect = { 
        _info_rect.x + _info_rect.w, 
        _scrubber_rect.y + _scrubber_rect.h, 
        _globals._WIN_SIZE.x - _info_rect.x - _info_rect.w - _globals._PADDING, 
        _globals._WIN_SIZE.y - _scrubber_rect.h - _scrubber_rect.y - _globals._PADDING 
    };

    // initWindow();
    _spectrum = new Spectrum(_spectrum_rect, renderer );

}






WayverUi::~WayverUi(){
    
    delete _spectrum;

    //Destroy window	
	SDL_DestroyRenderer( renderer );
	SDL_DestroyWindow( window );
    SDL_DestroyTexture(canvas);
    
    TTF_CloseFont( title_font );
    TTF_CloseFont( body_font );
    TTF_CloseFont( labels_font );

    TTF_Quit();
	SDL_Quit();

}






/****
 * Public Initializations - Called from main
*/
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
        _globals._WIN_SIZE.x, 
        _globals._WIN_SIZE.y, 
        SDL_WINDOW_SHOWN
    );
    assert( window != NULL );

    // Create vsynced renderer for window
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
    assert( renderer != NULL ); 

    SDL_RendererInfo info;

    /* Checking if this renderer supports target textures */
    SDL_GetRendererInfo(renderer, &info);
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

    _initFonts();

}






/*
* Private Initializations
*/
void WayverUi::_initFonts(){
    
    int ttfInited = TTF_Init();
    assert(ttfInited >= 0);

    title_font = TTF_OpenFont( "assets/Instruction.ttf", 59 );
    body_font = TTF_OpenFont( "assets/Instruction.ttf", 31 );
    labels_font = TTF_OpenFont( "assets/Instruction.ttf", 23 );
    
    if (title_font == NULL || body_font == NULL || labels_font == NULL){

        std::string errMsg = SDL_GetError();
        throw std::runtime_error(errMsg);
    }
}






/**
 * Runtime
*/
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






/**
 * Draw
*/
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
        _globals._BACKGROUND_1.r, 
        _globals._BACKGROUND_1.g, 
        _globals._BACKGROUND_1.b, 
        _globals._BACKGROUND_1.a 
    );
    
    SDL_RenderClear( renderer );

    _draw_Spectrum();
    _draw_Info();
    _draw_Scrubber();
    _draw_sampleCounter();
    
    SDL_RenderPresent(renderer);
}

void WayverUi::_draw_sampleCounter(){
    
    std::string _sample_counter_text = std::to_string( _frames_counter );

    SDL_Surface *textSurface = TTF_RenderText_Blended( body_font, _sample_counter_text.c_str(), _globals._FOREGROUND_1 );
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

    SDL_DestroyTexture( textTexture );
    SDL_FreeSurface( textSurface );
}


void WayverUi::_draw_Spectrum(){
    // draw borders
    SDL_SetRenderDrawColor( renderer, 
        _globals._FOREGROUND_1.r,
        _globals._FOREGROUND_1.g,
        _globals._FOREGROUND_1.b,
        _globals._FOREGROUND_1.a
    );

    SDL_RenderFillRect(renderer, &_spectrum_rect);

    // for (int i = 0; i < 4; i++){
    //     SDL_FLine &line_to_draw = _spectrum.borders[i];

    //     SDL_RenderDrawLineF(renderer, line_to_draw.pa.x, line_to_draw.pa.y, line_to_draw.pb.x, line_to_draw.pb.y );
        
    // }

    // // draw grid lines
    // for (auto ptr = _spectrum.grid_lines.begin(); ptr < _spectrum.grid_lines.end(); ptr++){

    //     const SDL_FLine &line_to_draw = _spectrum.line_toWindowCoords(*ptr);

    //     SDL_RenderDrawLineF(renderer, line_to_draw.pa.x, line_to_draw.pa.y, line_to_draw.pb.x, line_to_draw.pb.y );
    // }
}


void WayverUi::_draw_Scrubber(){
    // SDL_SetRenderDrawColor( renderer, 100, 120, 100, 255 );
    // SDL_RenderFillRect( renderer, &_scrubber_rect );

    // draw the sections outline:
    SDL_SetRenderDrawColor(
        renderer, 
        _globals._FOREGROUND_1.r, 
        _globals._FOREGROUND_1.g, 
        _globals._FOREGROUND_1.b, 
        _globals._FOREGROUND_1.a
    );

    SDL_Rect innerRect = {
        _scrubber_rect.x + _globals._INNER_PADDING,
        _scrubber_rect.y + _globals._INNER_PADDING,
        _scrubber_rect.w - 2 * _globals._INNER_PADDING,
        _scrubber_rect.h - 2 * _globals._INNER_PADDING
    };

    SDL_RenderDrawRect( renderer, &innerRect );
}


void WayverUi::_draw_Info(){
    SDL_SetRenderDrawColor( renderer, 200, 120, 100, 255 );
    SDL_RenderFillRect( renderer, &_info_rect );
}






/***
 * Util
*/
SDL_Point WayverUi::_getSize(SDL_Texture *texture) {
    SDL_Point size;
    SDL_QueryTexture(texture, NULL, NULL, &size.x, &size.y);
    return size;
}




/****
 * Components
*/
UIComponent::UIComponent(
    const SDL_Rect &contentRect, 
    SDL_Renderer *r
)
:
_renderer(r),
_content_rect(contentRect)

{}

// UIComponent::~UIComponent(){}

Spectrum::Spectrum(
    const SDL_Rect &contentRect,
    SDL_Renderer *r,
    int n_grid_x_divisions ,
    float min_x,
    float max_x
):
UIComponent(contentRect, r),
_min_x_value(min_x),
_max_x_value(max_x)
{

    // gen grid lines
    float grid_x_length = _max_x_value - _min_x_value;
    float grid_x_length_exp = log10f( grid_x_length );
    float delta_exp = grid_x_length_exp / (float)n_grid_x_divisions;

    // 1st x grid point is at lower bound,
    // from there on:
    //      2nd point -> lower_bound + 10^(delta_exp)
    //      3rd pont -> lower_bound + 10^(2 * delta_exp)
    //      and so on...
    for ( int i = 0; i < n_grid_x_divisions; i++ ){

        _x_axis_grid_divisions.push_back(_min_x_value + (i * delta_exp) );
        
        grid_lines.push_back({
            .pa = { ((float)i/(float)n_grid_x_divisions), 0 },
            .pb = { ((float)i/(float)n_grid_x_divisions), 1 }
        });

    }

}

void Spectrum::draw(){}

SDL_FPoint Spectrum::point_toWindowCoords(const SDL_FPoint &_p){
    return {
        _p.x*_spectrumBox_inCanvas.w + _spectrumBox_inCanvas.x,
        _p.y*_spectrumBox_inCanvas.h + _spectrumBox_inCanvas.y
    };
}

SDL_FLine Spectrum::line_toWindowCoords ( const SDL_FLine &_line ){
    return {
        point_toWindowCoords(_line.pa),
        point_toWindowCoords(_line.pb)
    };
}



Scrubber::Scrubber(
    const SDL_Rect &contentRect,
    SDL_Renderer *r,
    int total_samples,
    int current_sample,
    int current_buffer,
    int frames_per_buffer
):UIComponent(contentRect, r)
{

}

Scrubber::~Scrubber(){}

void Scrubber::draw(){

}

StaticInfo::StaticInfo(
    const SDL_Rect &contentRect,
    SDL_Renderer *r
):UIComponent(contentRect, r)
{}

void StaticInfo::draw(){}