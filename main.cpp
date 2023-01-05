#include <string>
#include <iostream>

#include <boost/thread.hpp>
#include <boost/chrono.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <wayver-audio.hpp>
#include <wayver-ui.hpp>
#include <wayver-bus.hpp>

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

    Wayver::Bus::Queues queues;
    Wayver::Audio::AudioEngine engine;

    engine.loadFile(path.c_str());
    SF_INFO sound_file_info = engine.getSoundFileInfo();

    engine.registerQueues( &queues );

    Wayver::UI::WayverUi ui;

    ui.initUiState(
        &queues,
        sound_file_info
    );

    ui.initWindow();

    // start audio thread
    boost::thread playT{boost::bind(&Wayver::Audio::AudioEngine::run, &engine)};
    
    ui.run();
    
    // block until finished
    playT.join();
    ui.stop();
    
    logger->info("Joined threads");
    logger->flush();
    
	return 0;
}

std::shared_ptr<spdlog::logger> initLogging()
{
    std::shared_ptr<spdlog::logger> _logger;

    _logger = spdlog::basic_logger_mt("MAIN", "wayver.log");
    spdlog::set_level( spdlog::level::debug );
    spdlog::flush_every(std::chrono::seconds(2));
    _logger->info("\n-------------\n------------\nStarting WAYVER\n");
    _logger->flush();
    return _logger;

}

void printHelp(){
    printf("Please supply a set of valid options:\n\n");
    printf("-f [filename]         -   reads and plays audio file\n");
    printf("-h                    -   display this message\n");

}