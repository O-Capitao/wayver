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

// about FFTs:
// https://www.dataq.com/data-acquisition/general-education-tutorials/fft-fast-fourier-transform-waveform-analysis.html
namespace Wayver {
    
    // PortAudio Callback
    // User obj
    struct AudioFile {

        AudioFile(int bufferSize, const std::string &file_path);
        ~AudioFile();

        SNDFILE* file = NULL;
        SF_INFO  info;
        
        int buffer_size;
        std::string filePath;

        int readHead = 0;
        int count = 1;
        
        // copy of each buffer, used for output
        float *dft_in = NULL;

    };


    // for outbound communication
    // from the engine to some client
    struct ExternalAudioData {

        std::string filename;
        int channels;
        int sample_rate;

        int n_bands_exported = 0;
        float *bands;

        std::string stringify();
    };
    struct SpectrumSliceBand{
        float log10_freq;
        float freq;
        float amp_pcm;
        float amp_db;
    };

    struct SpectrumSlice{
        
        SpectrumSlice(
            int n_bands,
            float *input_amps,
            float *input_freqs,
            int input_size,
            float amp_zero,
            bool useLogScale = true );

        

        private:
            
            
            std::vector<SpectrumSliceBand> _bands;

            int _n_bands;

            int input_size;
            float *in_freq_array;
            float *in_amp_array;

            static int _findBeforeFreqIndex( 
                float *freq_array, 
                int array_size, 
                float freq,
                int start_cursor = 0);

            static float _calcInterpolatedValueForFrequency( 
                float freq_to_get,
                int array_size, 
                float *freq_array,
                float *amp_array );

            static float _calcAmp_DbFullRange( float sample_value );
    };

    /*
    Class for handling basic audio functions
    */
    class AudioEngine {

        private:
            
            int _samplesInBuffer;
            int _dftBandsCount;

            AudioFile* _data = NULL;
            PaStream *stream;

            fftwf_plan _fft_plan;

            fftwf_complex *_fft_result_arr;
            float *_band_intensities_arr;
            float *_fft_aux_values_arr;
            float *_windowFunctionPoints_arr;
            float *_dft_Output_Freqs_arr;

            bool PLAYING = false;

            void _unloadFile();

            static int _paStreamCallback( 
                const void *inputBuffer,
                void *outputBuffer,
                unsigned long framesPerBuffer,
                const PaStreamCallbackTimeInfo* timeInfo,
                PaStreamCallbackFlags statusFlags,
                void *userData
            );

            // https://en.wikipedia.org/wiki/Hann_function
            void _calc_windowFunction();
            void _calc_DFT_OutputFreqs();
            

            static void _reduceDFTDataToBands( float *src, float *tgt, int n_samnples_src, int n_samples_tgt );
            // static void _multiplyArrays(double *arr1, double*arr2, double *out, int l);
            static void _copyArray(float *src, float *tgt, int l);
            static void _copyChannelWithWindowing( float *src, float *tgt, float *window, int total_l, int channel );

        public:

            AudioEngine(
                int samplesInBuffer = 512,
                int dftBandsCount = 100
            );

            ~AudioEngine();

            // Player Actions
            void loadFile(const std::string& path);
            void playFile();
            void pauseFile();
            void closeFile();

            // Data access
            // const SoundFileInfo &getSoundFileInfo();
            const ExternalAudioData generateExternalAudioData();
            void getFrqDomainData();

    };
}