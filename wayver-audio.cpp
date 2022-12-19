#include <wayver-audio.hpp>
#include <string>
#include <filesystem>
#include <math.h>

using namespace Wayver::Audio;



InternalAudioData::InternalAudioData(
    const std::string &p
):file(sf_open( p.c_str(), SFM_READ, &info))
{}

InternalAudioData::~InternalAudioData(){
    delete _rawDataTap_ptr;    
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


AudioEngine::AudioEngine(){

    //init logging
    _logger = spdlog::basic_logger_mt("AUDIO ENGINE", "wayver.log");
    _logger->debug( "Constructed" );
}

AudioEngine::~AudioEngine(){
    delete _data;
}


void AudioEngine::loadFile( const std::string& path){

    _logger->debug("loadFile()");

    if (_data != NULL){
        closeFile();
        delete _data;
    }

    _data = new InternalAudioData( path );
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
    float val;
    int pushedItems;

    // push read values into queue
    if (p_data->_rawDataTap_ptr->read_available() == 0){
        for (int i = 0; i < frameCount * p_data->info.channels; i++)
        {
            val = out[i];
            pushedItems = p_data->_rawDataTap_ptr->read_available();

            if (pushedItems == W_QUEUE_SIZE){
                p_data->_rawDataTap_ptr->pop();
            }

            if (!p_data->_rawDataTap_ptr->push( val )){

                int writespace = p_data->_rawDataTap_ptr->write_available();
                printf("Failed to push value %f into position %d.\n", val, i);
                printf("Queue already had %d items. And can take %d.\n", pushedItems, writespace );
                return paAbort;
            
            }

        }
    }
    
    
    /*  If we couldn't read a full frameCount of samples we've reached EOF */
    if (num_read < frameCount)
    {
        return paComplete;
    }
    
    return paContinue;
}

// https://github.com/hosackm/wavplayer/blob/master/src/wavplay.c
void AudioEngine::run(){

    _logger->debug("Starting playFile()");
    PaError err = Pa_Initialize();

    if (err != paNoError){
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
        FRAMES_IN_BUFFER,
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

    closeFile();

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


const SF_INFO &AudioEngine::getSoundFileInfo(){
    return _data->info;
}

boost::lockfree::spsc_queue<float,boost::lockfree::capacity<W_QUEUE_SIZE>> *AudioEngine::getRawDataTap_ptr(){
    return _data->_rawDataTap_ptr;
}

void AudioEngine::setAudioToUiQueue( 
    boost::lockfree::spsc_queue<float,boost::lockfree::capacity<W_QUEUE_SIZE>> *queue_ptr 
){
    _data->_rawDataTap_ptr = queue_ptr;
}
