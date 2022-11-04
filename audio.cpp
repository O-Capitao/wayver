#include "audio.hpp"
#include <string>
#include  <filesystem>
#include <math.h>

using namespace Wayver;

AudioFile::AudioFile(int bs, const std::string &p){
    
    buffer_size = bs;
    filePath = p;

    file = sf_open( p.c_str(), SFM_READ, &info);
    dft_in = new float[buffer_size * info.channels];


}

AudioFile::~AudioFile(){
    delete[] dft_in;
    printf("EXIT InternalAudioData::~InternalAudioData()\n");
}

AudioEngine::AudioEngine( 
    int samplesInBuffer,
    int dftBandsCount 
){

    assert( samplesInBuffer % 2 == 0);

    _calc_windowFunction();

    _samplesInBuffer = samplesInBuffer;
    _dftBandsCount = dftBandsCount;

    _fft_result_arr = new fftwf_complex[ _samplesInBuffer ];
    _windowFunctionPoints_arr = new float[ _samplesInBuffer ];
    _dft_Output_Freqs_arr = new float[ _samplesInBuffer / 2 ];
    _band_intensities_arr = new float[ _dftBandsCount ];
}

AudioEngine::~AudioEngine(){

    delete[] _fft_result_arr;
    delete[] _band_intensities_arr;
    delete[] _fft_aux_values_arr;
    delete[] _windowFunctionPoints_arr;
    delete[] _dft_Output_Freqs_arr;

    _unloadFile();

    printf("EXIT AudioEngine::~AudioEngine()\n");
}


void AudioEngine::loadFile( const std::string& path){
    if (_data != NULL){
        _unloadFile();
    }

    _data = new AudioFile( _samplesInBuffer, path );
    
    _fft_aux_values_arr = new float[  _data->info.channels * _samplesInBuffer ];

    _fft_plan = fftwf_plan_dft_r2c_1d(
        _samplesInBuffer, 
        _fft_aux_values_arr, 
        _fft_result_arr, 
        FFTW_MEASURE
    );

    _calc_DFT_OutputFreqs();
}

void AudioEngine::_unloadFile(){

    if (_data == NULL) return;

    sf_close(_data->file);

    delete _data;
    _data = NULL;

}   

int AudioEngine::_paStreamCallback(
    const void *input
    ,void *output
    ,unsigned long frameCount
    ,const PaStreamCallbackTimeInfo *timeInfo
    ,PaStreamCallbackFlags statusFlags
    ,void *userData
){

    // if (QUIT){
    //     return paComplete;
    // }

    float *out;
    AudioFile *p_data = (AudioFile*)userData;

    sf_count_t num_read;

    out = (float*)output;
    p_data = (AudioFile*)userData;

    /* clear output buffer */
    memset(out, 0, sizeof(float) * frameCount * p_data->info.channels);

    /* read directly into output buffer */
    num_read = sf_read_float(p_data->file, out, frameCount * p_data->info.channels);
    
    _copyArray( out, p_data->dft_in, p_data->buffer_size * p_data->info.channels );
    
    /*  If we couldn't read a full frameCount of samples we've reached EOF */
    if (num_read < frameCount)
    {
        return paComplete;
    }
    
    return paContinue;
}

// https://github.com/hosackm/wavplayer/blob/master/src/wavplay.c
void AudioEngine::playFile(){
    
    PaError err = Pa_Initialize();

    if (err != paNoError){
        // fprintf( stderr, "Couldn't init the portaudio library!\n" );
        throw std::runtime_error("Error initing the AudioEngine");
    }

    if ( _data == NULL ){
        err = Pa_Terminate();

        throw std::runtime_error("No data to play, shutting down.\n");
    }

    err = Pa_OpenDefaultStream(
        &stream,
        0,
        _data->info.channels,
        paFloat32,
        _data->info.samplerate,
        _samplesInBuffer,
        _paStreamCallback,
        _data
    );

    if( err != paNoError )
    {
        throw std::runtime_error("Error opening Stream.\n");
    }

    err = Pa_StartStream(stream);

    if (err != paNoError){
        throw std::runtime_error("Error starting Stream\n");
    }
    PLAYING = true;

  

    /* Run until EOF is reached */
    while(Pa_IsStreamActive(stream))
    {
        Pa_Sleep(100);
    }

    closeFile();
}

void AudioEngine::pauseFile(){
    if (PLAYING){
        PaError err = Pa_CloseStream( stream );

        if (err != paNoError){
            throw std::runtime_error("Error Stopping stream\n");
        }
    }
}

void AudioEngine::closeFile()
{
    
    PaError err;

    PLAYING = false;
    /* Close the soundfile */
    sf_close(_data->file);

    /*  Shut down portaudio */
    err = Pa_CloseStream(stream);
    if(err != paNoError)
    {
        throw std::runtime_error("Error closing Stream.\n");
    }
    
    err = Pa_Terminate();
    if(err != paNoError)
    {
        throw std::runtime_error("Error terminating Portaudio.\n");
    }
}

void AudioEngine::_calc_windowFunction(){

    for (int i = 0; i < _samplesInBuffer; i++){

        _windowFunctionPoints_arr[i] = pow( sin( M_PI * i / (_samplesInBuffer - 1) ), 2);

    }

}

void AudioEngine::_calc_DFT_OutputFreqs(){

    assert( _data->file != NULL );

    float k = 0;
    float sr = (float)_data->info.samplerate;
    float fpb = (float)_samplesInBuffer;

    for (int i = 0; i < _samplesInBuffer / 2; i++){
        k = (float)i;
        _dft_Output_Freqs_arr[i] = ( k * sr ) / fpb;
    }


}

void AudioEngine::_copyArray(float *src, float *tgt, int l){
    for (int i = 0; i < l; i++){
        tgt[i] = src[i];
    }
}

void AudioEngine::_copyChannelWithWindowing( float *src, float *tgt, float *window, int total_l, int channel ){

    assert( total_l % 2 == 0);
    
    int n_samples = total_l / 2;

    for (int i = 0; i < n_samples; i++){
        tgt[i] = src[2 * i + channel] * window[i];
    }
}

void AudioEngine::_reduceDFTDataToBands( float *src, float *tgt, int n_samnples_src, int n_samples_tgt ){

}



const ExternalAudioData AudioEngine::generateExternalAudioData(){

    _copyChannelWithWindowing(
        _data->dft_in, _fft_aux_values_arr, 
        _windowFunctionPoints_arr,
        _samplesInBuffer * _data->info.channels, 
        0 
    );

    fftwf_execute( _fft_plan );

    float aux[_samplesInBuffer / 2];
    // float max = 0;

    // transform fft complex output into integer 0 - 10
    for (int i = 0; i < _samplesInBuffer / 2; i++){
        aux[i] = sqrt( pow(_fft_result_arr[i][0],2) + pow(_fft_result_arr[i][1], 2) );
        // max = (aux[i]> max) ? aux[i] : max;
    };

    for (int i = 0; i < _dftBandsCount; i++){
        _band_intensities_arr[i] = 0;
    }

    return {
        .filename = "dummy",
        .channels = _data->info.channels,
        .sample_rate = _data->info.samplerate,
        .bands = _band_intensities_arr,
        .n_bands_exported = _dftBandsCount
    };
}



std::string ExternalAudioData::stringify(){

    assert( n_bands_exported != 0);

    std::string retval = "BANDS:\n";

    for (int i = 0; i < n_bands_exported ; i++){
        retval.append(std::to_string( bands[i] ) + " ");
    }

    retval +="\n";

    return retval;
}


