#include <string>
#include <iostream>

#include <boost/thread.hpp>
#include <boost/chrono.hpp>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

#include "audio.hpp"

std::shared_ptr<spdlog::logger> initLogging();

void printHelp();

/****
 *  MAIN
 */
int main(int argc, char *argv[])
{

    std::string path;

    // check argvd
    if (argc < 3 || strcmp(argv[1],"-h") == 0) {
        // kthrow std::runtime_error("Please supply a path to the audio file");
        printHelp();
        return 1;
    }

    if ( strcmp(argv[1],"-f") == 0 ){
        path = argv[2];
    }


    auto logger = initLogging();
    logger->debug("Opening {}", path);
    logger->flush();

    Wayver::AudioEngine engine( 128 );
    engine.loadFile(path.c_str());
    
    // start audio thread
    boost::thread playT{boost::bind(&Wayver::AudioEngine::playFile, &engine)};
    


    //block until finished
    playT.join();
    engine.closeFile();


	return 0;
}





std::shared_ptr<spdlog::logger> initLogging()
{
    std::shared_ptr<spdlog::logger> _logger;

    _logger = spdlog::basic_logger_mt("MAIN", "wayver.log");
    spdlog::set_level( spdlog::level::debug );
    _logger->info("Starting WAYVER\n");

    return _logger;

}

void printHelp(){
    printf("Please supply a set of valid options:\n\n");
    printf("-f [filename]         -   reads and plays audio file\n");
    printf("-h                    -   display this message\n");

}