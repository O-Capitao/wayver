#pragma once

#include <vector>

#include <wayver-defines.hpp>
#include <wayver-util.hpp>
#include <wayver-bus.hpp>

#include <boost/lockfree/spsc_queue.hpp>
#include <spdlog/spdlog.h>

#include <sndfile.hh>

#include <SDL.h>
#include <SDL_ttf.h>

namespace Wayver {

    namespace UI {

        struct Globals{
            // Window Layout - Master Measurements
            const SDL_Point _WIN_SIZE = {1024,768};
            const int _PADDING = 20;
            const int _INNER_PADDING = 2;
            const int _SCRUBBER_HEIGHT = 60;
            const int _INFO_WIDTH = 300;

            // Colors
            const SDL_Color _FOREGROUND_1 = { .r = 255, .g = 255, .b = 255, .a = 255 };

            // rgb(0, 255, 204)
            const SDL_Color _FOREGROUND_2 = { .r = 0, .g = 255, .b = 204, .a = 255 };
            const SDL_Color _BACKGROUND_1 = { .r = 0, .g = 0, .b = 0, .a = 255 };
        };

        class UIComponent {

            protected:
                SDL_Renderer *_renderer = NULL;
                std::shared_ptr<spdlog::logger> _logger;

            public:
                SDL_Rect _content_rect;
                const int _INNER_PADDING = 3;
                Globals globals;

                UIComponent(
                    const SDL_Rect &contentRect,
                    SDL_Renderer *r,
                    std::shared_ptr<spdlog::logger> logger
                );

                // ~UIComponent();

                void draw();
        };

        struct SDL_FLine  {
            SDL_FPoint pa;
            SDL_FPoint pb;
        };


        // UI Components

        /**
         * HELP panel
        */
       class Help: public UIComponent {

            bool _visible = false;

            TTF_Font *_font;
            SDL_FRect _help_rect;
            
            const std::string _text = 
                "Q - Quit    SPACE - Play/Pause    ARROW UP/DWN - Volume";
            
            public:
                Help(
                    const SDL_Rect &contentRect,
                    SDL_Renderer *r,
                    std::shared_ptr<spdlog::logger> logger,
                    TTF_Font *f
                );

                void draw();
                void toggle();

       };

        /***
         * SCRUBBER
         * is the play bar
         * gives info on time ellapsed and total time
        */
        class Scrubber : public UIComponent{

            SF_INFO _sf_info;
            SDL_FRect _scrub_bar_rect_outer;
            SDL_FRect _scrub_bar_rect_inner;

            float _max_scrubber_bar_width = 0;

            int _total_ms = 0;
            int _frame_counter;

            TTF_Font *_font;
            SDL_FPoint _timeLabelPosition;

            const std::string _ms_to_time_string(int time_ms);

            std::shared_ptr<spdlog::logger> _logger;

            void _draw_TimeText();

            public:
                Scrubber(
                    const SDL_Rect &contentRect,
                    SDL_Renderer *r,
                    std::shared_ptr<spdlog::logger> logger,
                    const SF_INFO &sfi,
                    TTF_Font *f
                );

                ~Scrubber();

                void update(
                    int sample_counter
                );

                void draw();
        };









        class StaticInfo : public UIComponent{
            public:
                StaticInfo(
                    const SDL_Rect &contentRect,
                    SDL_Renderer *r
                );
                void draw();
        };

        class Spectrum : public UIComponent{

            float _min_x_value;
            float _max_x_value;

            // int n;

            SDL_Rect _spectrumBox_inCanvas;
            std::vector<float> _x_axis_grid_divisions;

            public:

                SDL_FPoint point_toWindowCoords( const SDL_FPoint &_point_in_grid);
                SDL_FLine line_toWindowCoords ( const SDL_FLine &_line );

                Spectrum(
                    const SDL_Rect &contentRect,
                    SDL_Renderer *r,
                    int n = 10,
                    float min_x = 20,
                    float max_x = 20000
                );
                
                // in grid coords - ranged 0 to 1
                std::vector<SDL_FLine> grid_lines;
                void draw();
                    
        };





        class WayverUi {

            Globals _globals;

            std::shared_ptr<spdlog::logger> _logger;
            
            // input from Audio thread
            Bus::Queues *_queues_ptr;
            
            // array with samples we'll read from the queue
            std::vector<float> _samples_in_vec;
            int _n_samples_in;
            int _n_frames_per_buffer;

            // computed layout stuff
            SDL_Rect _spectrum_rect;
            SDL_Rect _scrubber_rect;
            SDL_Rect _info_rect;
            SDL_Rect _help_rect;

            SF_INFO _sfInfo;
            
            // init frames counter to 0
            int _frames_counter = 0;

            SDL_Window* window;
            SDL_Renderer* renderer;
            SDL_Texture* canvas;

            // Fonts
            TTF_Font *title_font = NULL;
            TTF_Font *body_font = NULL;
            TTF_Font *labels_font = NULL;

            // flags
            bool _QUIT = false;

            // Throttle user events
            const int _THROTTLE_TIME_MS = 300;
            int _throttleTimer_start = 0;
            bool _throttleActive = false;


            // spectogram grid
            // Spectrum *_spectrum = NULL;
            Scrubber *_scrubber = NULL;
            Help *_help_component = NULL;

            // private initializations
            void _initFonts();

            // Draw
            void _draw();

            void _update();

            void _handleEvents();

            // Utils
            SDL_Point _getSize(SDL_Texture *texture);

            public:

                WayverUi();
                ~WayverUi();

                // public inits -> called from main
                void initUiState(
                    Bus::Queues *_q_ptr,
                    const SF_INFO &info
                );

                void setSfInfo( const SF_INFO &sfi);
                
                void initWindow();

                // runtime
                void run();
                void stop();

        };


    }
}

