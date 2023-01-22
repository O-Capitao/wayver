#pragma once

#include <wayver-defines.hpp>
#include <wayver-ui.hpp>
#include <wayver-bus.hpp>

#include <portaudio.h>
#include <sndfile.hh>
#include <string>
#include <vector>

// In particular, if you #include <complex.h> before <fftw3.h>,
// then fftw_complex is defined to be the native complex type 
// and you can manipulate it with ordinary arithmetic (e.g. x = y * (3+4*I), 
// where x and y are fftw_complex and I is the standard symbol for the imaginary unit)
#include <complex>
#include <fftw3.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <boost/lockfree/spsc_queue.hpp>


// about FFTs:
// https://www.dataq.com/data-acquisition/general-education-tutorials/fft-fast-fourier-transform-waveform-analysis.html
namespace Wayver {

    namespace Audio {

        /***
         * Object that is passed to paCallback,
         * used to exchange data between Audio Thread
         * and outside world
        */
        struct InternalAudioData {

            InternalAudioData( const std::string &path );
            ~InternalAudioData();

            SNDFILE* file = NULL;
            SF_INFO  info;

            std::string file_path;

            /* Frames read */
            int readHead = 0;
            Bus::Queues *_q_ptr = NULL;
            std::shared_ptr<spdlog::logger> _logger;

            // set to true when stopping -> avoid pop
            bool STOPPED = false;

            float GAIN = 1;
        };

        /***
         * Wraps around the paCallback
         * 
         *      - Handles the File, Stream and holds copies of 
         *      Audio Data for outside world as well as accessors
        */
        class AudioEngine {

            private:
                
                InternalAudioData* _data = NULL;
                PaStream *stream = NULL;
                std::shared_ptr<spdlog::logger> _logger;
                Bus::Queues *_queues_ptr;

                static int _paStreamCallback( 
                    const void *inputBuffer,
                    void *outputBuffer,
                    unsigned long framesPerBuffer,
                    const PaStreamCallbackTimeInfo* timeInfo,
                    PaStreamCallbackFlags statusFlags,
                    void *userData
                );

                void _openStream();
                void _closeStream();
                void _startStream();
                void _stopStream();
                
                void _closeFile();

                const float _GAIN_STEP = 0.1;
                void _nudgeGain( bool DOWN = true );



                // Utility
                static void _applyFadeOut( float *samples_arr, int channels, int frames_in_buffer );
                static std::string _arrayToString(float *array, int length);
                static void _applyGain( float gain, float *arr, int size );

            public:

                AudioEngine();
                ~AudioEngine();

                // Player Actions
                void loadFile(const std::string& path);
                void registerQueues(Bus::Queues *_q_ptr);

                // Audio Thread
                void run();

                const SF_INFO &getSoundFileInfo(); 
                const std::string &getPathToFile();
        };

    }
}