#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include "spdlog/sinks/basic_file_sink.h"
#include <string>
#include <math.h>


#include "wayver-ui.hpp"


using namespace Wayver::UI;



WayverUi::WayverUi()
:_logger(spdlog::basic_logger_mt("UI", "wayver.log"))
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

    _logger->info("Constructed");
    _logger->flush();    
}






WayverUi::~WayverUi(){
    
    delete _scrubber;

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
    Bus::Queues *_q_ptr,
    const SF_INFO &info
)
{
    
    this->_sfInfo = info;
    this->_n_samples_in = info.channels * _n_frames_per_buffer;
    this->_samples_in_vec.reserve( _n_samples_in );
    this->_queues_ptr = _q_ptr;

    _logger->debug("Finished Constructor");
    _logger->flush();
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

    _scrubber = new Scrubber(
        _scrubber_rect,
        renderer,
        _sfInfo,
        body_font
    );

    _logger->debug("initWindow()");
    _logger->flush();
}






/*
* Private Initializations
*/
void WayverUi::_initFonts(){
    
    int ttfInited = TTF_Init();
    assert(ttfInited >= 0);

    title_font = TTF_OpenFont( "assets/Instruction.ttf", 59 );
    body_font = TTF_OpenFont( "assets/Instruction.ttf", 21 );
    labels_font = TTF_OpenFont( "assets/Instruction.ttf", 17 );
    
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
    _logger->flush();

    // initWindow();
    // std::vector<float> *frames = new std::vector<float>();
    // float item = 0;

    // auto consume_pending = [&] {
    //     while ( _queues_ptr->_queue_audio_to_ui.pop(item)) {
    //         frames->push_back(item);
    //     }
    // };

    while (!_QUIT ) {

        _handleEvents();

        // int items_in_queue = _queues_ptr->_queue_audio_to_ui.read_available();
        _frames_counter = _queues_ptr->head;
        

        // consume_pending();

        // if (items_in_queue > 0){
        //     _logger->debug("consumed pending.");
        //     _logger->flush();
        // }
        
        
        _update();

        _draw();

        boost::this_thread::sleep_for( boost::chrono::milliseconds( UI_WAIT_TIME ) );
    }

    // frames->empty();
    // delete frames;
    _logger->debug("Exiting run()");
    _logger->flush();
}

void WayverUi::stop(){
    _logger->debug("Received STOP signal.");
    _logger->flush();
    _QUIT = true;
}






/**
 * Draw
*/
void WayverUi::_draw(){

    //Clear screen
    SDL_SetRenderDrawColor( 
        renderer, 
        _globals._BACKGROUND_1.r, 
        _globals._BACKGROUND_1.g, 
        _globals._BACKGROUND_1.b, 
        _globals._BACKGROUND_1.a 
    );
    
    SDL_RenderClear( renderer );

    // _draw_Spectrum();
    // _draw_Info();
    _scrubber->draw();
    // _draw_sampleCounter();
    

    SDL_RenderPresent(renderer);
}

void WayverUi::_update(){
    _scrubber->update( _frames_counter );
}





/***
 * Util
*/
SDL_Point WayverUi::_getSize(SDL_Texture *texture) {
    SDL_Point size;
    SDL_QueryTexture(texture, NULL, NULL, &size.x, &size.y);
    return size;
}

void WayverUi::setSfInfo(const SF_INFO &sfi){
    _sfInfo = sfi;
}


void WayverUi::_handleEvents(){
    SDL_Event e;

    while( SDL_PollEvent( &e ) != 0 )
    {
        switch(e.type) {

            case SDL_QUIT:
                _queues_ptr->_queue_commands.push( Bus::Command::QUIT );
                _QUIT = true;
                break;

            case SDL_KEYUP:
                switch (e.key.keysym.sym)
                {
                case 'q':
                    _queues_ptr->_queue_commands.push( Bus::Command::QUIT );
                    _QUIT = true;
                    break;
                
                default:
                    break;
                }
            case SDL_KEYDOWN:
                switch (e.key.keysym.sym)
                {
                case SDLK_SPACE:
                    _PAUSE = true;
                    _queues_ptr->_queue_commands.push( Bus::Command::PAUSE_PLAY );
                    break;
                
                default:
                    break;
                }
            default:
            break;
        }
    }
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


























/***
 * SCRUBBER
*/
Scrubber::Scrubber(
    const SDL_Rect &contentRect,
    SDL_Renderer *r,
    const SF_INFO &sfi,
    TTF_Font *f
):UIComponent(contentRect, r),
_font(f)
{
    _logger = spdlog::basic_logger_mt("UI::Scrubber", "wayver.log");

    _sf_info = sfi;

    _scrub_bar_rect_outer = {
        (float)_content_rect.x + 10,
        (float)_content_rect.y + 10,
        (float)_content_rect.w - 20,
        8
    };

    _scrub_bar_rect_inner = {
        _scrub_bar_rect_outer.x,
        _scrub_bar_rect_outer.y,
        0,
        _scrub_bar_rect_outer.h
    };

    _timeLabelPosition = {
        _scrub_bar_rect_outer.x,
        _scrub_bar_rect_outer.y + _scrub_bar_rect_outer.h + 5
    };

    _max_scrubber_bar_width = _scrub_bar_rect_outer.w;

    _total_ms = 1000 * (float)sfi.frames / (float)sfi.samplerate;
    std::string cc = _ms_to_time_string(_total_ms);

    _logger->debug(
        "Constructed\n     total frame count = {}\n     total time:{}\n     total time (pretty): '{}'", 
        sfi.frames,
        _total_ms,
        cc
    );

    _logger->flush();

}

Scrubber::~Scrubber(){

    _logger->debug("~Scrubber()\n   total frames processed: {}", _frame_counter );
    _logger->flush();

}

const std::string Scrubber::_ms_to_time_string(int time_ms){
    std::string retval = "";

    int total_seconds = time_ms / 1000;

    int total_minutes = total_seconds / 60;
    int leftover_seconds = total_seconds % 60;

    retval = 
        ((total_minutes < 10) ? "0" : "") +
        std::to_string( total_minutes ) + ":" + 
        ((leftover_seconds < 10) ? "0" : "") +
        std::to_string( leftover_seconds );

    return retval;
}



void Scrubber::draw(){
    SDL_SetRenderDrawColor(
        _renderer,
        globals._FOREGROUND_1.r,
        globals._FOREGROUND_1.g,
        globals._FOREGROUND_1.b,
        globals._FOREGROUND_1.a
    );

    SDL_RenderDrawRect( _renderer, &_content_rect );
    SDL_RenderFillRectF( _renderer, &_scrub_bar_rect_outer );

    SDL_SetRenderDrawColor(
        _renderer,
        globals._FOREGROUND_2.r,
        globals._FOREGROUND_2.g,
        globals._FOREGROUND_2.b,
        globals._FOREGROUND_2.a
    );
    SDL_RenderFillRectF(_renderer, &_scrub_bar_rect_inner );

    _draw_TimeText();
}

void Scrubber::update( int sc ){
    _frame_counter = sc;
    // int _ellapsed_ms = sc / (_sf_info.channels * _sf_info.samplerate / 1000);
    float gone_by_ratio = (float)(sc)
        /(float)(_sf_info.frames);
    
    // recalc play rect
    _scrub_bar_rect_inner.w = gone_by_ratio * _max_scrubber_bar_width;

}

// privates:
void Scrubber::_draw_TimeText(){

    SDL_Surface* text;
    
    std::string _temp_text = _ms_to_time_string( 
        1000 * (float)_frame_counter / (float)_sf_info.samplerate)
        + " / "
        + _ms_to_time_string( _total_ms );

    text = TTF_RenderText_Solid( 
        _font, 
        _temp_text.c_str(), 
        globals._FOREGROUND_2 );

    SDL_Texture* text_texture;
    text_texture = SDL_CreateTextureFromSurface( _renderer, text );

    SDL_Rect src = {
        0,0,text->w, text->h
    };

    SDL_Rect dest = { 
        (int)floor(_timeLabelPosition.x),
        (int)floor(_timeLabelPosition.y),
        text->w, text->h };

    SDL_RenderCopy( _renderer, text_texture, &src, &dest );

    // cleanup
    SDL_DestroyTexture( text_texture );
    SDL_FreeSurface( text );
}





















/**
 * Static Info
*/
StaticInfo::StaticInfo(
    const SDL_Rect &contentRect,
    SDL_Renderer *r
):UIComponent(contentRect, r)
{}

void StaticInfo::draw(){}