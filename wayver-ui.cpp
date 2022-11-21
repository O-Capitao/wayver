#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include "spdlog/sinks/basic_file_sink.h"


#include "wayver-ui.hpp"


using namespace Wayver;

void WayverUi::init(
    int n_c,
    boost::lockfree::spsc_queue<float,boost::lockfree::capacity<W_QUEUE_SIZE>> *rdptr
){
    _logger = spdlog::basic_logger_mt("UI", "wayver.log");
    // this->_input_from_audio = ifa;
    
    this->_n_channels = n_c;
    this->_n_samples_in = _n_channels * _n_frames_per_buffer;
    this->_samples_in_vec.reserve( _n_samples_in );
    this->_rawDataTap_ptr = rdptr;

    _logger->debug("Finished Constructor");
}


void WayverUi::run(){
    _logger->debug("Starting run()");

    // Aquire data from queue.
    while (!_stop)
    {
        float fliflu;

        if (_rawDataTap_ptr->read_available() > 0){
            _rawDataTap_ptr->pop(fliflu);
            _logger->debug("Got message! : {}", fliflu);
        }

        boost::this_thread::sleep_for( boost::chrono::milliseconds( UI_WAIT_TIME ) );
    }
    _logger->debug("Exiting run()");
}

void WayverUi::stop(){
    _logger->debug("Received STOP signal.");
    _stop = true;
}
