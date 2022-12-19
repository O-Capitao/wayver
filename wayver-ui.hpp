#pragma once

#include <vector>

#include "wayver-defines.hpp"
#include "wayver-util.hpp"

#include <boost/lockfree/spsc_queue.hpp>
#include "spdlog/spdlog.h"

#include "SDL.h"
#include <SDL_ttf.h>

namespace Wayver {

    namespace UI {

        struct Globals{
            // Window Layout - Master Measurements
            const SDL_Point _WIN_SIZE = {1024,768};
            const int _PADDING = 20;
            const int _INNER_PADDING = 2;
            const int _SCRUBBER_HEIGHT = 100;
            const int _INFO_WIDTH = 300;

            // Colors
            const SDL_Color _FOREGROUND_1 = { .r = 255, .g = 255, .b = 255, .a = 255 };
            const SDL_Color _FOREGROUND_2 = { .r = 255, .g = 255, .b = 255, .a = 190 };
            const SDL_Color _BACKGROUND_1 = { .r = 0, .g = 0, .b = 0, .a = 255 };
        };


        class UIComponent {

            SDL_Renderer *_renderer = NULL;

            public:
                SDL_Rect _content_rect;
                const int _INNER_PADDING = 3;
                Globals globals;

                UIComponent(
                    const SDL_Rect &contentRect,
                    SDL_Renderer *r
                );

                // ~UIComponent();

                void draw();
        };

        struct SDL_FLine  {
            SDL_FPoint pa;
            SDL_FPoint pb;
        };


        // UI Components
        class Scrubber : public UIComponent{

            int _total_samples;
            int _current_sample;
            int _current_buffer;
            int _frames_per_buffer;

            SDL_FRect _scrub_bar_rect;

            public:
                Scrubber(
                    const SDL_Rect &contentRect,
                    SDL_Renderer *r,
                    int total_samples,
                    int current_sample,
                    int current_buffer,
                    int frames_per_buffer
                );

                ~Scrubber();

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
            boost::lockfree::spsc_queue<float,boost::lockfree::capacity<W_QUEUE_SIZE>> *_rawDataTap_ptr;
            
            // array with samples we'll read from the queue
            std::vector<float> _samples_in_vec;
            int _n_samples_in;
            int _n_frames_per_buffer;
            int _n_channels;

            // computed layout stuff
            SDL_Rect _spectrum_rect;
            SDL_Rect _scrubber_rect;
            SDL_Rect _info_rect;
            
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
            bool _stop = false;



            // spectogram grid
            Spectrum *_spectrum = NULL;

            // private initializations
            void _initFonts();

            // Draw
            void _draw();

            // Draw steps
            void _draw_sampleCounter();
            void _draw_Spectrum();
            void _draw_Scrubber();
            void _draw_Info();


            // Utils
            SDL_Point _getSize(SDL_Texture *texture);

            public:

                WayverUi();
                ~WayverUi();

                // public inits -> called from main
                void initUiState(
                    int n_channels,
                    boost::lockfree::spsc_queue<float,boost::lockfree::capacity<W_QUEUE_SIZE>> *dataTap_ptr
                );
                
                void initWindow();
                
                // runtime
                void run();
                void stop();

        };


    }
}

