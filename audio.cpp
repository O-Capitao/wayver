#include "audio.hpp"
#include <string>
#include  <filesystem>
#include <math.h>

using namespace Wayver;

InternalAudioData::InternalAudioData(int bs, const std::string &p){
    
    frames_in_buffer = bs;
    file = sf_open( p.c_str(), SFM_READ, &info);
    buffer_copy_arr = new float[ frames_in_buffer * info.channels ];


}

InternalAudioData::~InternalAudioData(){
    delete[] buffer_copy_arr;
}


std::string arrayToString(float *arr, int len){
    std::string retval = "";
    for (int i=0;i<len;i++){

        retval += std::to_string(i) + ": ";
        retval += std::to_string(arr[i]);

        if (i != len - 1 ){
            retval += "; ";
        }
    }
    return retval;
}


AudioEngine::AudioEngine( int bs ){

    //init logging
    _logger = spdlog::basic_logger_mt("AUDIO ENGINE", "wayver.log");
    _logger->debug( "Starting AudioEngine.\nSAMPLES IN BUFFER={}.", bs );
    
    _frames_in_buffer = bs;
}

AudioEngine::~AudioEngine(){
    delete _data;
}


void AudioEngine::loadFile( const std::string& path){

    if (_data != NULL){
        closeFile();
        delete _data;
    }

    _data = new InternalAudioData( _frames_in_buffer, path );
}

int AudioEngine::_paStreamCallback(
    const void *input
    ,void *output
    ,unsigned long frameCount
    ,const PaStreamCallbackTimeInfo *timeInfo
    ,PaStreamCallbackFlags statusFlags
    ,void *userData
){

    float *out;
    InternalAudioData *p_data = (InternalAudioData*)userData;

    sf_count_t num_read;

    out = (float*)output;
    p_data = (InternalAudioData*)userData;

    /* clear output buffer */
    memset(out, 0, sizeof(float) * frameCount * p_data->info.channels);

    /* read directly into output buffer */
    num_read = sf_read_float(p_data->file, out, frameCount * p_data->info.channels);
    
    memcpy( p_data->buffer_copy_arr, out,  p_data->frames_in_buffer * p_data->info.channels );
    
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
        _frames_in_buffer,
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
  
    /* Run until EOF is reached */
    while(Pa_IsStreamActive(stream))
    {
        Pa_Sleep(100);
    }

}

void AudioEngine::closeFile()
{   
    _logger->info("Closing File");
    
    PaError err;

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



