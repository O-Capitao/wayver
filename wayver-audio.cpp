#include <wayver-audio.hpp>
#include <string>
#include <filesystem>
#include <math.h>

using namespace Wayver::Audio;



InternalAudioData::InternalAudioData(
    const std::string &p
):file(sf_open( p.c_str(), SFM_READ, &info)),
_logger(spdlog::basic_logger_mt("AUDIO INTERNAL", "wayver.log"))
{
}

InternalAudioData::~InternalAudioData(){
    // delete _rawDataTap_ptr;    
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


AudioEngine::AudioEngine()
:_logger(spdlog::basic_logger_mt("AUDIO ENGINE", "wayver.log"))
{
    _logger->debug( "Constructed" );
    _logger->flush();
}

AudioEngine::~AudioEngine(){
    _logger->info("~AudioEngine()");
    _logger->flush();
    delete _data;
}


void AudioEngine::loadFile( const std::string& path){

    _logger->debug("loadFile()");

    if (_data != NULL){
        _closeFile();
        delete _data;
    }

    _data = new InternalAudioData( path );

    _logger ->info(
        "Successfully loaded file:\n  channels= {}\n  sample rate= {}\n  total Frames= {}\n  sections= {}\n  seekable= {}\n  format={}",
        _data->info.channels,
        _data->info.samplerate, 
        _data->info.frames,
        _data->info.sections,
        _data->info.seekable,
        _data->info.format );

    _logger->flush();
}

int AudioEngine::_paStreamCallback(
    const void *input
    ,void *output /* The data sent to the Audio Device */
    ,unsigned long frameCount /* frames in buffer - samples / channels */
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
    

    p_data->readHead += (num_read / p_data->info.channels);
    p_data->_q_ptr->head = p_data->readHead;

    // float val;
    // int pushedItems;

    // OUTPUT
    
    // if (p_data->_q_ptr->_queue_audio_to_ui.read_available() == 0){
    
    // for (int i = 0; i < frameCount * p_data->info.channels; i++)
    // {
    //     val = out[i];
    //     pushedItems = p_data->_q_ptr->_queue_audio_to_ui.read_available();
    //     p_data->readHead++;

    //     p_data->_logger->debug("_paStreamCallback - pushedItems={} - read head={}",pushedItems, p_data->readHead);

    //     if (pushedItems == W_QUEUE_SIZE){
    //         p_data->_logger->debug("Queue full, popping");
    //         if (!p_data->_q_ptr->_queue_audio_to_ui.pop()){
    //             p_data->_logger->error("Error popping...");
    //         }
    //     }

    //     if (!p_data->_q_ptr->_queue_audio_to_ui.push( val )){

    //         int writespace = p_data->_q_ptr->_queue_audio_to_ui.write_available();

    //         printf("Failed to push value %f into position %d.\n", val, i);
    //         printf("Queue already had %d items. And can take %d.\n", pushedItems, writespace );
    //         return paAbort;
        
    //     }

    // }
    // }

    
    // p_data->_logger->flush();

    /*  If we couldn't read a full frameCount of samples we've reached EOF */
    if (num_read < frameCount)
    {
        p_data->_logger->debug("File reached the end");
        p_data->_logger->flush();

        return paComplete;
    }
    
    return paContinue;
}

// https://github.com/hosackm/wavplayer/blob/master/src/wavplay.c
void AudioEngine::run(){

    _logger->debug("Starting playFile()");
    
    if ( Pa_Initialize() != paNoError){
        _logger->error("run() : Error initing the AudioEngine");
        throw std::runtime_error("Error initing the AudioEngine");
    }

    if ( _data == NULL ){
        Pa_Terminate();
        _logger->error("run() : No data to play, shutting down.");
        throw std::runtime_error("No data to play, shutting down.");
    }

    _openStream();
    _startStream();
    
    
    /* Main Event Loop */
    while (!_QUIT_SIG){

        Bus::Command _cmd;
        // Process User Actions
        while ( _queues_ptr->_queue_commands.pop(_cmd)) {

            
            
            if ( _cmd == Bus::Command::QUIT ){

                _QUIT_SIG = true;

                // pass data to user obj
                // let the callback stop gracefully.
                // _data->STOP_SIGNALLED = true;

                // stop stream from Callback
                if (_PLAYING_SIG){
                    _stopStream();
                }
                
            
            } 

            if ( _cmd == Bus::Command::PAUSE_PLAY ){
                

                if (_PLAYING_SIG){
                    _logger->debug("run() - cmd == Bus::Command::PAUSE_PLAY, PAUSING");
                    // _data->STOP_SIGNALLED = true;
                    // Pa_Sleep(100);
                    // // stop stream from Callback
                    _stopStream();
                } else {
                    _logger->debug("run() - cmd == Bus::Command::PAUSE_PLAY, PLAYING");
                    // _data->STOP_SIGNALLED = false;
                    _startStream();
                }
            }
        }

        Pa_Sleep(500);
    }

    _closeStream();
    _closeFile();

}

void AudioEngine::_openStream(){

    _logger->debug("AudioEngine::_openStream()");

    PaError e = Pa_OpenDefaultStream(
        &stream,
        0,
        _data->info.channels,
        paFloat32,
        _data->info.samplerate,
        FRAMES_IN_BUFFER,
        _paStreamCallback,
        _data
    );

    if (e != paNoError){
        std::string msg = Pa_GetErrorText( e );
        _logger->error( "Error opening stream. msg; {}", msg );
        throw std::runtime_error("Could not Open stream.");
    }

}

void AudioEngine::_startStream(){

    _logger->debug("AudioEngine::_startStream()");
    PaError e = Pa_StartStream(stream);
    _PLAYING_SIG = true;

    if (e != paNoError){
        std::string msg = Pa_GetErrorText( e );
        _logger->error( "Error starting stream. msg; {}", msg );
        throw std::runtime_error("Could not Start stream.");
    }
}

void AudioEngine::_stopStream(){

    _logger->debug("AudioEngine::_stopStream()");
    PaError e = Pa_StopStream(stream);
    _PLAYING_SIG = false;

    if (e != paNoError){
        std::string msg = Pa_GetErrorText( e );
        _logger->error( "Error stopping stream. msg; {}", msg );
        throw std::runtime_error("Could not Stop stream.");
    }
}

void AudioEngine::_closeStream(){
    _logger->debug("AudioEngine::_closeStream()");
    PaError e = Pa_CloseStream(stream);

    stream = NULL; 

    if (e != paNoError){
        std::string msg = Pa_GetErrorText( e );
        _logger->error( "Error closing stream. msg; {}", msg );
        throw std::runtime_error("Could not close stream.");
    }

}

void AudioEngine::_closeFile()
{   
    _logger->info("Closing File");
    _logger->flush();
    
    /* Close the soundfile */
    sf_close(_data->file);
    
    PaError err = Pa_Terminate();
    if(err != paNoError)
    {
        throw std::runtime_error("Error terminating Portaudio.\n");
    }

}


const SF_INFO &AudioEngine::getSoundFileInfo(){
    return _data->info;
}

void AudioEngine::registerQueues( 
    Bus::Queues *q_ptr
){
    this->_queues_ptr = q_ptr;
    _data->_q_ptr = q_ptr;
}
