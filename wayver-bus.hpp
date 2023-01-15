#pragma once

#include <boost/lockfree/spsc_queue.hpp>

#include <wayver-defines.hpp>

namespace Wayver {
    namespace Bus {
        
        enum Command {
            PAUSE_PLAY,
            STOP,
            QUIT,
            NUDGE_GAIN_UP,
            NUDGE_GAIN_DWN
        };

        struct Queues {

            boost::lockfree::spsc_queue<float,boost::lockfree::capacity<W_QUEUE_SIZE>> _queue_audio_to_ui;
            boost::lockfree::spsc_queue<Command,boost::lockfree::capacity<W_QUEUE_SIZE>> _queue_commands;
            int head = 0;

        };

    }
}