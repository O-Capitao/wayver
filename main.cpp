

#include <string>

#include "audio.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"


std::shared_ptr<spdlog::logger> initLogging();
/****
 *  MAIN
 */
int main(int argc, char *argv[])
{
    if (argc < 2){
        throw std::runtime_error("Please supply a path to the audio file");
        return 1;
    }

    auto logger = initLogging();
    
    std::string file_path = argv[1];

    logger->debug("Opening {}", file_path);

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