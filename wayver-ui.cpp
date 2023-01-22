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

    _help_rect = {
        0,
        0,
        _globals._WIN_SIZE.x,
        _globals._WIN_SIZE.y
    };

    _logger->info("Constructed");
    _logger->flush();    
}






WayverUi::~WayverUi()
{    
    delete _scrubber;
    delete _help_component;
    delete _static_info;

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
    const SF_INFO &info,
    const std::string &fpath )
{
    this->_sfInfo = info;
    this->_n_samples_in = info.channels * _n_frames_per_buffer;
    this->_samples_in_vec.reserve( _n_samples_in );
    this->_queues_ptr = _q_ptr;
    this->path_to_file = fpath;

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
        _logger,
        _sfInfo,
        body_font
    );

    _help_component = new Help(
        _help_rect,
        renderer,
        _logger,
        body_font
    );
    
    _static_info = new StaticInfo(
        _info_rect,
        renderer,
        _logger,
        _sfInfo,
        path_to_file,
        _globals._BACKGROUND_1,
        _globals._FOREGROUND_1,
        body_font,
        title_font
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

    title_font = TTF_OpenFont( "assets/Instruction.ttf", 72 );
    body_font = TTF_OpenFont( "assets/Instruction.ttf", 21 );
    labels_font = TTF_OpenFont( "assets/Instruction.ttf", 16 );
    
    if (title_font == NULL || body_font == NULL || labels_font == NULL){

        std::string errMsg = SDL_GetError();
        throw std::runtime_error(errMsg);
    }
}






/**
 * Runtime
*/
void WayverUi::run(){

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
        
        _update();
        _draw();

        boost::this_thread::sleep_for( boost::chrono::milliseconds( UI_WAIT_TIME ) );
    }

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

    _scrubber->draw();
    _help_component->draw();
    _static_info->draw();

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

    // take care of throttle_counter;
    if (_throttleActive && ((SDL_GetTicks() - _throttleTimer_start) >= _THROTTLE_TIME_MS)){
        _throttleActive = false;
    }
    
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
                    if (!_throttleActive){
                        _queues_ptr->_queue_commands.push( Bus::Command::PAUSE_PLAY );
                        _throttleActive = true;
                        _throttleTimer_start = SDL_GetTicks();
                    }
                    break;

                case SDLK_DOWN:
                    if (!_throttleActive){
                        _queues_ptr->_queue_commands.push( Bus::Command::NUDGE_GAIN_DWN );
                        _throttleActive = true;
                        _throttleTimer_start = SDL_GetTicks();
                    }
                    break;
                
                case SDLK_UP:
                    if (!_throttleActive){
                        _queues_ptr->_queue_commands.push( Bus::Command::NUDGE_GAIN_UP );
                        _throttleActive = true;
                        _throttleTimer_start = SDL_GetTicks();
                    }
                case SDLK_h:
                    if (!_throttleActive){
                        _logger -> debug("SDL_k pressed");
                        _help_component->toggle();
                        _throttleActive = true;
                        _throttleTimer_start = SDL_GetTicks();
                    }
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
    SDL_Renderer *r,
    std::shared_ptr<spdlog::logger> logger
):_renderer(r),
_content_rect(contentRect),
_logger(logger)
{}



/***
 * SCRUBBER
*/
Scrubber::Scrubber(
    const SDL_Rect &contentRect,
    SDL_Renderer *r,
    std::shared_ptr<spdlog::logger> logger,
    const SF_INFO &sfi,
    TTF_Font *f
):UIComponent(contentRect, r, logger),
_font(f)
{
    _logger = spdlog::basic_logger_mt("UI::Scrubber", "wayver.log");

    _sf_info = sfi;

    _scrub_bar_rect_outer = {
        (float)_content_rect.x,
        (float)_content_rect.y,
        (float)_content_rect.w,
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

    text = TTF_RenderText_Shaded( 
        _font, 
        _temp_text.c_str(), 
        globals._FOREGROUND_2,
        globals._BACKGROUND_1 );

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







/***
 * Help
*/
Help::Help(
    const SDL_Rect &contentRect,
    SDL_Renderer *r,
    std::shared_ptr<spdlog::logger> logger,
    TTF_Font *f )
:UIComponent(contentRect, r, logger),
_font(f)
{
    _help_rect = {
        (float)_content_rect.x + 100,
        (float)_content_rect.y + 100,
        (float)_content_rect.w - 200,
        8
    };

}


void Help::toggle(){
    _visible = !_visible;
}

void Help::draw(){

    if (_visible){

        SDL_SetRenderDrawColor(
            _renderer,
            globals._BACKGROUND_1.r,
            globals._BACKGROUND_1.g,
            globals._BACKGROUND_1.b,
            globals._BACKGROUND_1.a );

        SDL_RenderFillRectF( _renderer, &_help_rect );

        SDL_SetRenderDrawColor(
            _renderer,
            globals._FOREGROUND_1.r,
            globals._FOREGROUND_1.g,
            globals._FOREGROUND_1.b,
            globals._FOREGROUND_1.a );

        SDL_Surface* text_surface;

        text_surface = TTF_RenderText_Shaded( 
            _font, 
            _text.c_str(), 
            globals._FOREGROUND_2,
            globals._BACKGROUND_1 );
                
        SDL_Texture* text_texture;

        text_texture = SDL_CreateTextureFromSurface( _renderer, 
            text_surface );

        SDL_Rect src = {
            0,0,text_surface->w, text_surface->h
        };

        SDL_Rect dest = { 
            (int)floor(_help_rect.x),
            (int)floor(_help_rect.y),
            text_surface->w, 
            text_surface->h
        };

        SDL_RenderCopy( _renderer, text_texture, &src, &dest );
        
        // cleanup
        SDL_DestroyTexture( text_texture );
        SDL_FreeSurface( text_surface );
        
    }
}







// LAbel
Label::Label(
    const SDL_Rect &contentRect,
    SDL_Renderer *r,
    std::shared_ptr<spdlog::logger> logger,
    TTF_Font *font,
    SDL_Color bg_color,
    SDL_Color fg_color,
    SDL_Point position
):UIComponent(contentRect, r, logger),
_font(font),
_content(""),
_fg_color(fg_color),
_bg_color(bg_color),
_position(position)
{
}

Label::~Label(){
    SDL_DestroyTexture( _texture );
    SDL_FreeSurface( _surface );
}

void Label::draw(){
    if (_isVisible){
        SDL_RenderCopy( _renderer, _texture, &_src, &_tgt );
    }
}

void Label::updateContents( const std::string &_newContents ){

    if (_surface != NULL){
        SDL_DestroyTexture( _texture );
        SDL_FreeSurface( _surface );
    }

    _surface = TTF_RenderText_Shaded( 
        _font, 
        _newContents.c_str(), 
        _fg_color,
        _bg_color );
    
    _texture = SDL_CreateTextureFromSurface( _renderer, 
        _surface );

    _src =  {
        0,0,_surface->w, _surface->h
    };

    _tgt = {
        (int)floor(_position.x),
        (int)floor(_position.y),
        _surface->w, 
        _surface->h
    };
}

void Label::toggleVisibility(){
    _isVisible = !_isVisible;
}




/****
 * StaticInfo contents
*/
StaticInfo::StaticInfo(
    const SDL_Rect &contentRect,
    SDL_Renderer *r,
    std::shared_ptr<spdlog::logger> logger,
    const SF_INFO &sfi,
    const std::string &filename,
    SDL_Color bg_color,
    SDL_Color fg_color,
    TTF_Font *small_font,
    TTF_Font *lrg_font
):UIComponent(contentRect, r, logger),
_filename_label( contentRect, r, logger, lrg_font, bg_color, fg_color, {contentRect.x, contentRect.y}),
_channels_label( contentRect, r, logger, small_font, bg_color, fg_color, {contentRect.x, contentRect.y + 150} ),
_framerate_label( contentRect, r, logger, small_font, bg_color, fg_color, {contentRect.x, contentRect.y + 200})
{
    _filename_label.updateContents(filename);
    _channels_label.updateContents( "Channels: " + std::to_string(sfi.channels) );
    _framerate_label.updateContents( "Sample Rate: " + std::to_string( sfi.samplerate ) + " Hz" );
}

void StaticInfo::draw(){
    _filename_label.draw();
    _channels_label.draw();
    _framerate_label.draw();
}
