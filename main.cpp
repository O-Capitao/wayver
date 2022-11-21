#include <string>
#include <iostream>

#include <boost/thread.hpp>
#include <boost/chrono.hpp>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

#include "audio.hpp"
#include "wayver-ui.hpp"

/***
 * Main Fn Headers
*/
std::shared_ptr<spdlog::logger> initLogging();
void printHelp();


/****
 *  MAIN FN
 */
int main(int argc, char *argv[])
{

    std::string path;

    // check argvd
    if (argc < 3 || strcmp(argv[1],"-h") == 0) {
        printHelp();
        return 1;
    }

    if ( strcmp(argv[1],"-f") == 0 ){
        path = argv[2];
    }


    auto logger = initLogging();
    logger->debug("Opening {}", path);

    // init the QUEUE
    boost::lockfree::spsc_queue<float,boost::lockfree::capacity<W_QUEUE_SIZE>> *queue_audio_to_ui = 
        new boost::lockfree::spsc_queue<float,boost::lockfree::capacity<W_QUEUE_SIZE>>();

    Wayver::AudioEngine engine;

    engine.loadFile(path.c_str());
    SF_INFO sound_file_info = engine.getSoundFileInfo();
    
    int N_CHANNELS = sound_file_info.channels;

    engine.setAudioToUiQueue( queue_audio_to_ui );

    Wayver::WayverUi ui;

    ui.init(
        N_CHANNELS,
        queue_audio_to_ui
    );

    // start audio thread
    boost::thread playT{boost::bind(&Wayver::AudioEngine::run, &engine)};
    boost::thread uiT{boost::bind(&Wayver::WayverUi::run, &ui)};
    
    // block until finished
    // player thread exits first;
    playT.join();
    ui.stop();
    uiT.join();
    
    logger->debug("Joined threads");

	return 0;
}

std::shared_ptr<spdlog::logger> initLogging()
{
    std::shared_ptr<spdlog::logger> _logger;

    _logger = spdlog::basic_logger_mt("MAIN", "wayver.log");
    spdlog::set_level( spdlog::level::debug );
    spdlog::flush_every(std::chrono::seconds(2));
    _logger->info("Starting WAYVER\n");

    return _logger;

}

void printHelp(){
    printf("Please supply a set of valid options:\n\n");
    printf("-f [filename]         -   reads and plays audio file\n");
    printf("-h                    -   display this message\n");

}