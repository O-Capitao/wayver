#pragma once

#include <vector>

#include "wayver-defines.hpp"
#include "wayver-util.hpp"

#include <boost/lockfree/spsc_queue.hpp>
#include "spdlog/spdlog.h"

#include "SDL.h"
#include <SDL_ttf.h>

namespace Wayver {

    class WayverUi {

        std::shared_ptr<spdlog::logger> _logger;
        boost::lockfree::spsc_queue<float,boost::lockfree::capacity<W_QUEUE_SIZE>> *_rawDataTap_ptr;
        
        // array with samples we'll read from the queue
        std::vector<float> _samples_in_vec;
        int _n_samples_in;
        int _n_frames_per_buffer;
        int _n_channels;

        int _win_w = 1024;
        int _win_h = 768;

        // init frames counter to 0
        int _frames_counter = 0;

        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_Texture* canvas;
        TTF_Font *gFont = NULL;

        // Colors
        SDL_Color _foregroundColor = { .r = 200, .g = 100, .b = 100, .a = 255 };
        SDL_Color _backgroundColor = { .r = 0, .g = 0, .b = 0, .a = 255 };

        // flags
        bool _stop = false;

        void _draw();

        void _loadFonts();

        void _draw_sampleCounter();

        // Utils
        SDL_Point _getSize(SDL_Texture *texture);

        public:

            WayverUi(){};

            ~WayverUi();

            void initUiState(
                int n_channels,
                boost::lockfree::spsc_queue<float,boost::lockfree::capacity<W_QUEUE_SIZE>> *dataTap_ptr
            );
            
            void initWindow();

            void run();

            void stop();

    };

}

