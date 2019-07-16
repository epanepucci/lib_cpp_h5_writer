#include <iostream>
#include <sstream>

#include "WriterManager.hpp"

using namespace std;

void writer_utils::set_process_id(int user_id)
{

    #ifdef DEBUG_OUTPUT
        using namespace date;
        cout << "[" << std::chrono::system_clock::now() << "]";
        cout << "[writer_utils::set_process_id] Setting process uid to " << user_id << endl;
    #endif

    if (setgid(user_id)) {
        stringstream error_message;
        using namespace date;
        error_message << "[" << std::chrono::system_clock::now() << "]";
        error_message << "[writer_utils::set_process_id] Cannot set group_id to " << user_id << endl;

        throw runtime_error(error_message.str());
    }

    if (setuid(user_id)) {
        stringstream error_message;
        using namespace date;
        error_message << "[" << std::chrono::system_clock::now() << "]";
        error_message << "[writer_utils::set_process_id] Cannot set user_id to " << user_id << endl;

        throw runtime_error(error_message.str());
    }
}

void writer_utils::create_destination_folder(const string& output_file)
{
    auto file_separator_index = output_file.rfind('/');

    if (file_separator_index != string::npos) {
        string output_folder(output_file.substr(0, file_separator_index));
        using namespace date;
        cout << "[" << std::chrono::system_clock::now() << "]";
        cout << "[writer_utils::create_destination_folder] Creating folder " << output_folder << endl;

        string create_folder_command("mkdir -p " + output_folder);
        system(create_folder_command.c_str());
    }
}

WriterManager::WriterManager(const unordered_map<string, DATA_TYPE>& parameters_type, 
    const string& output_file, uint64_t n_frames):
        parameters_type(parameters_type), output_file(output_file), n_frames(n_frames), 
        running_flag(true), killed_flag(false), n_received_frames(0), n_written_frames(0), 
        n_lost_frames(0), mode_category(std::make_tuple(false, ""));
{
    #ifdef DEBUG_OUTPUT
        using namespace date;
        cout << "[" << std::chrono::system_clock::now() << "]";
        cout << "[WriterManager::WriterManager] Writer manager for n_frames " << n_frames << endl;
    #endif
}

WriterManager::~WriterManager(){}

void WriterManager::stop()
{
    #ifdef DEBUG_OUTPUT
        using namespace date;
        cout << "[" << std::chrono::system_clock::now() << "]";
        cout << "[WriterManager::stop] Stopping the writer manager." << endl;
    #endif

    running_flag = false;
}

void WriterManager::kill()
{
    #ifdef DEBUG_OUTPUT
        using namespace date;
        cout << "[" << std::chrono::system_clock::now() << "]";
        cout << "[WriterManager::kills] Killing writer manager." << endl;
    #endif

    killed_flag = true;

    stop();
}

string WriterManager::get_status()
{
    if (running_flag) {
        return "receiving";
    } else if (n_received_frames.load() > n_written_frames) {
        return "writing";
    } else if (!are_all_parameters_set()) {
        return "waiting for parameters";
    } else {
        return "finished";
    }
}

string WriterManager::get_output_file() const
{
    return output_file;
}

unordered_map<string, uint64_t> WriterManager::get_statistics() const
{
    unordered_map<string, uint64_t> result = {{"n_received_frames", n_received_frames.load()},
                                    {"n_written_frames", n_written_frames.load()},
                                    {"n_lost_frames", n_lost_frames.load()},
                                    {"total_expected_frames", n_frames}};

    return result;
}

unordered_map<string, boost::any> WriterManager::get_parameters()
{
    lock_guard<mutex> lock(parameters_mutex);

    return parameters;
}

void WriterManager::set_parameters(const unordered_map<string, boost::any>& new_parameters)
{
    lock_guard<mutex> lock(parameters_mutex);

    #ifdef DEBUG_OUTPUT
        stringstream output_message;
        using namespace date;
        output_message << "[" << std::chrono::system_clock::now() << "]";
        output_message << "[WriterManager::set_parameters] Setting parameters: ";
    #endif

    for (const auto& parameter : new_parameters) {
        auto& parameter_name = parameter.first;
        auto& parameter_value = parameter.second;

        parameters[parameter_name] = parameter_value;

        #ifdef DEBUG_OUTPUT
            output_message << parameter_name << ", ";
        #endif
    }

    #ifdef DEBUG_OUTPUT
        cout << output_message.str() << endl;
    #endif
}

const unordered_map<string, DATA_TYPE>& WriterManager::get_parameters_type() const
{
    return parameters_type;
}

bool WriterManager::is_running()
{
    // Take into account n_frames only if it is <> 0.
    if (n_frames && n_received_frames.load() >= n_frames) {
        running_flag = false;
    }

    return running_flag.load();
}

bool WriterManager::is_killed() const
{
    return killed_flag.load();
}

void WriterManager::received_frame(size_t frame_index)
{
    n_received_frames++;
}

void WriterManager::written_frame(size_t frame_index)
{
    n_written_frames++;
}

void WriterManager::lost_frame(size_t frame_index)
{
    n_lost_frames++;
}

bool WriterManager::are_all_parameters_set()
{
    lock_guard<mutex> lock(parameters_mutex);

    for (const auto& parameter : parameters_type) {
        const auto& parameter_name = parameter.first;

        if (parameters.count(parameter_name) == 0) {
            #ifdef DEBUG_OUTPUT
                using namespace date;
                cout << "[" << std::chrono::system_clock::now() << "]";
                cout << "[WriterManager::are_all_parameters_set] Parameter " << parameter_name << " not set." << endl;
            #endif

            return false;
        }
    }

    return true;
}

size_t WriterManager::get_n_frames()
{
    return n_frames;
}

size_t WriterManager::get_n_written_frames()
{
    return n_written_frames;
}

size_t WriterManager::get_n_received_frames()
{
    return n_received_frames;
}

int WriterManager::get_user_id()
{
    return user_id;
}

uint64_t WriterManager::get_n_lost_frames()
{
    return n_lost_frames;
}

uint64_t WriterManager::get_first_pulse_id()
{
    return first_pulse_id;
}

std::chrono::steady_clock::time_point WriterManager::get_start_time()
{
    return start_time;
}

void WriterManager::set_first_pulse_id_time_start(uint64_t pulse_id, std::chrono::steady_clock::time_point timestamp)
{
    first_pulse_id = pulse_id;
    time_start = timestamp;
}

std::string WriterManager::get_filter(){
    return "statisticsWriter";
}

void set_mode_category(bool new_mode, std::string new_cat)
{
    mode_category = std::make_tuple(new_mode, new_cat);
}

std::tuple<bool, std::string> get_mode_category()
{
    return mode_category;
}

void set_processing_rate(std::chrono::duration<double> diff)
{
    processing_rate = diff;
}

pt::ptree WriterManager::get_statistics(){
    pt::ptree root;
    pt::ptree stats_json;
    switch (  std::get<1>(mode_category) ) {
        case "start":
            stats_json.put("first_frame_id", first_pulse_id);
            stats_json.put("n_frames", get_n_frames() );
            stats_json.put("output_file", get_output_file());
            stats_json.put("user_id", get_user_id());
            stats_json.put("timestamp", time_start);
            stats_json.put("compression_method", );
            root.add_child("statistics_wr_finish", stats_json);
            break;
        case "adv":
            // calculates the elapsed time from beginning
            auto time_diff = (std::chrono::system_clock::now() - time_start)
            // received_rate = total number of received frames / elapsed time
            auto receiving_rate = get_n_received_frames() / time_diff;
            // writting_rate = total number of written frames / elapsed time
            auto writting_rate = get_n_written_frames() / time_diff;
            stats_json.put("n_written_frames", get_n_written_frames());
            stats_json.put("n_received_frames", get_n_received_frames());
            stats_json.put("n_free_slots", ring_buffer.free_slots());
            stats_json.put("enable": "true");
            stats_json.put("processing_rate", processing_rate.count());
            stats_json.put("receiving_rate", receiving_rate);
            stats_json.put("writting_rate", writting_rate);
            stats_json.put("avg_compressed_size", 0.0);
            root.add_child("statistics_wr_adv", stats_json);
            break;
        case "end":
            // creates the finish statistics json
            stats_json.put("end_time", time_end);
            stats_json.put("enable", "true");
            stats_json.put("n_lost_frames", get_n_lost_frames());
            stats_json.put("n_total_written_frames", get_n_written_frames());
            root.add_child("statistics_wr_finish", stats_json);
        default:
            root.add_child("unidentified_mode", "unidentified_mode");
            break;
    }    
    return root;
}