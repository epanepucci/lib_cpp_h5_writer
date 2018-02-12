#include <iostream>
#include <stdexcept>

#include "config.hpp"
#include "ZmqReceiver.hpp"
#include "H5Format.hpp"

using namespace std;
namespace pt = boost::property_tree;

ZmqReceiver::ZmqReceiver(const std::string& connect_address, const int n_io_threads, const int receive_timeout) :
    connect_address(connect_address), n_io_threads(n_io_threads), receive_timeout(receive_timeout), receiver(NULL)
{
    #ifdef DEBUG_OUTPUT
        cout << "[ZmqReceiver::ZmqReceiver] Creating ZMQ receiver with";
        cout << " connect_address " << connect_address;
        cout << " n_io_threads " << n_io_threads;
        cout << " receive_timeout " << receive_timeout;
        cout << endl;
    #endif

    message_header = zmq::message_t(config::zmq_buffer_size_header);
    message_data = zmq::message_t(config::zmq_buffer_size_data);

    header_values_type.reset(
        new unordered_map<string, HEADER_DATA_TYPE>({
            {"pulse_id", UINT64 },
        }));
}

void ZmqReceiver::connect()
{
    #ifdef DEBUG_OUTPUT
        cout << "[ZmqReceiver::connect] Connecting to address " << connect_address;
        cout << " with n_io_threads " << n_io_threads << endl;
    #endif

    context = make_shared<zmq::context_t>(n_io_threads);
    receiver = make_shared<zmq::socket_t>(*context, ZMQ_PULL);

    receiver->setsockopt(ZMQ_RCVTIMEO, receive_timeout);
    receiver->connect(connect_address);
}

pair<shared_ptr<FrameMetadata>, char*> ZmqReceiver::receive()
{
    if (!receiver) {
        stringstream error_message;
        error_message << "[ZmqReceiver::receive] Cannot receive before connecting. ";
        error_message << "Connect first." << endl;

        throw runtime_error(error_message.str());
    }

    // Get the message header.
    if (!receiver->recv(&message_header)){
        return {NULL, NULL};
    }

    auto header_string = string(static_cast<char*>(message_header.data()), message_header.size());
    auto frame_metadata = read_json_header(header_string);

    // Get the message data.
    if (!receiver->recv(&message_data)) {
        cout << "[ZmqReceiver::receive] ERROR: Error while reading from ZMQ. Frame index " << frame_metadata->frame_index << " lost."; 
        cout << " Trying to continue with the next frame." << endl;\

        return {NULL, NULL};
    }

    frame_metadata->frame_bytes_size = message_data.size();

    return {frame_metadata, static_cast<char*>(message_data.data())};
}

boost::any ZmqReceiver::get_value_from_json(const pt::ptree& json_header, const string& value_name, const HEADER_DATA_TYPE data_type)
{
    switch(data_type) {
        case UINT8 : 
            return json_header.get<uint8_t>(value_name);
        case UINT16 : 
            return json_header.get<uint16_t>(value_name);
        case UINT32 : 
            return json_header.get<uint32_t>(value_name);
        case UINT64 : 
            return json_header.get<uint64_t>(value_name);
        case INT8 : 
            return json_header.get<int8_t>(value_name);
        case INT16 : 
            return json_header.get<int16_t>(value_name);
        case INT32 : 
            return json_header.get<int32_t>(value_name);
        case INT64 : 
            return json_header.get<int64_t>(value_name);
        case FLOAT32 :
            return json_header.get<float>(value_name);
        case FLOAT64 :
            return json_header.get<double>(value_name);
        default:
            stringstream error_message;
            error_message << "[ZmqReceiver::get_value_from_json] Unknown value type for header value " << value_name << endl;

            throw runtime_error(error_message.str());
    }
}

shared_ptr<FrameMetadata> ZmqReceiver::read_json_header(const string& header)
{   
    stringstream header_stream;
    header_stream << header << endl;
    pt::read_json(header_stream, json_header);

    auto header_data = make_shared<FrameMetadata>();

    header_data->frame_index = json_header.get<uint64_t>("frame");

    for (const auto& item : json_header.get_child("shape")) {
        header_data->frame_shape.push_back(item.second.get_value<size_t>());
    }

    // Array 1.0 specified little endian as the default encoding.
    header_data->endianness = json_header.get("endianness", "little");

    header_data->type = json_header.get<string>("type");

    for (const auto& value_mapping : *header_values_type) {
        
        const auto& name = value_mapping.first;
        const auto& data_type = value_mapping.second;
        const boost::any& value = get_value_from_json(json_header, name, data_type);

        header_data->header_values.insert(
            {name, value}
        );
    }

    return header_data;
}