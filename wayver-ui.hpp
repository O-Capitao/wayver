#pragma once

#include <vector>

#include "wayver-defines.hpp"
#include "wayver-util.hpp"

#include <boost/lockfree/spsc_queue.hpp>
#include "spdlog/spdlog.h"

namespace Wayver {

    class WayverUi {

        std::shared_ptr<spdlog::logger> _logger;
        boost::lockfree::spsc_queue<float,boost::lockfree::capacity<W_QUEUE_SIZE>> *_rawDataTap_ptr;
        
        // array with samples we'll read from the queue
        std::vector<float> _samples_in_vec;
        int _n_samples_in;
        int _n_frames_per_buffer;
        int _n_channels;

        bool _stop = false;

        public:

            void init(
                int n_channels,
                boost::lockfree::spsc_queue<float,boost::lockfree::capacity<W_QUEUE_SIZE>> *dataTap_ptr
            );
            
            void run();

            void stop();

    };

}

