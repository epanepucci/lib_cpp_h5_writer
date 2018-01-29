#ifndef WRITERMANAGER_H
#define WRITERMANAGER_H

#include <map>
#include <string>
#include <atomic>
#include <mutex>
#include <boost/any.hpp>

#include "H5Format.hpp"

class WriterManager
{
    
    std::map<std::string, boost::any> parameters = {};
    std::mutex parameters_mutex;

    // Initialize in constructor.
    std::map<std::string, DATA_TYPE>* parameters_type;
    size_t n_frames;
    std::atomic_bool running_flag;
    std::atomic_bool killed_flag;
    std::atomic<uint64_t> n_received_frames;
    std::atomic<uint64_t> n_written_frames;

    public:
        WriterManager(std::map<std::string, DATA_TYPE>* parameters_type, uint64_t n_frames=0);
        void stop();
        void kill();
        bool is_running();
        bool is_killed();
        std::string get_status();
        bool are_all_parameters_set();

        std::map<std::string, DATA_TYPE>* get_parameters_type();
        std::map<std::string, boost::any> get_parameters();
        void set_parameters(std::map<std::string, boost::any>& new_parameters);
        
        std::map<std::string, uint64_t> get_statistics();
        void received_frame(size_t frame_index);
        void written_frame(size_t frame_index);
};

#endif