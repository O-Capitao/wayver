#pragma once

#include "portaudio.h"
#include "sndfile.hh"
#include <string>
#include <vector>

// In particular, if you #include <complex.h> before <fftw3.h>,
// then fftw_complex is defined to be the native complex type 
// and you can manipulate it with ordinary arithmetic (e.g. x = y * (3+4*I), 
// where x and y are fftw_complex and I is the standard symbol for the imaginary unit)
#include <complex>
#include <fftw3.h>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <boost/lockfree/spsc_queue.hpp>

// about FFTs:
// https://www.dataq.com/data-acquisition/general-education-tutorials/fft-fast-fourier-transform-waveform-analysis.html
namespace Wayver {

    /***
     * Object that is passed to paCallback,
     * used to exchange data between Audio Thread
     * and outside world
    */
    struct InternalAudioData {

        InternalAudioData(
            int _frames_in_buffer, 
            const std::string &path
        );

        ~InternalAudioData();

        SNDFILE* file = NULL;
        SF_INFO  info;
        std::string file_path;

        int readHead = 0;
        int count = 1;
        
        int frames_in_buffer;

        boost::lockfree::spsc_queue<float, boost::lockfree::capacity<1000>> *_rawDataTap_ptr;
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
            PaStream *stream;
            std::shared_ptr<spdlog::logger> _logger;
            int _frames_in_buffer;


            static int _paStreamCallback( 
                const void *inputBuffer,
                void *outputBuffer,
                unsigned long framesPerBuffer,
                const PaStreamCallbackTimeInfo* timeInfo,
                PaStreamCallbackFlags statusFlags,
                void *userData
            );
        public:

            AudioEngine( 
                int nFramesInBuffer
            );
            
            ~AudioEngine();

            // Player Actions
            void loadFile(const std::string& path);

            void setAudioToUiQueue( 
                boost::lockfree::spsc_queue<float,boost::lockfree::capacity<1000>> *queue_ptr 
            );

            void closeFile(); 

            // Audio Thread
            void run();
            void pauseFile();

            boost::lockfree::spsc_queue<float,boost::lockfree::capacity<1000>>  *getRawDataTap_ptr();

            const SF_INFO &getSoundFileInfo();    
    };
}