#include <iostream>
#include <stdexcept>

#include "config.hpp"
#include "ZmqSender.hpp"
#include "H5Format.hpp"

using namespace std;
namespace pt = boost::property_tree;

ZmqSender::ZmqSender(const std::string& connect_address, const int n_io_threads, const int receive_timeout) :
        connect_address(connect_address), n_io_threads(n_io_threads), 
        receive_timeout(receive_timeout), sender(NULL)

{
    #ifdef DEBUG_OUTPUT
        using namespace date;
        cout << "[" << std::chrono::system_clock::now() << "]";
        cout << "[ZmqSender::ZmqSender] Creating ZMQ sender with";
        cout << " connect_address " << connect_address;
        cout << " n_io_threads " << n_io_threads;
        cout << " receive_timeout " << receive_timeout;
        cout << endl;
    #endif
}

void ZmqSender::bind()
{
    #ifdef DEBUG_OUTPUT
        using namespace date;
        cout << "[" << std::chrono::system_clock::now() << "]";
        cout << "[ZmqSender::bind] Binding to address " << connect_address;
        cout << " with n_io_threads " << n_io_threads << endl;
    #endif

    context = make_shared<zmq::context_t>(n_io_threads);
    sender = make_shared<zmq::socket_t>(*context, ZMQ_PUB);

    sender->bind(connect_address);
}

void ZmqSender::send(const std::string& filter, zmq::message_t message_data)
{
    if (!sender) {
        stringstream error_message;
        using namespace date;
        error_message << "[" << std::chrono::system_clock::now() << "]";
        error_message << "[ZmqSender::send] Cannot send before connecting. ";
        error_message << "Connect first." << endl;

        throw runtime_error(error_message.str());
    }
    
    // Send the message 
    // rv is the return value
    auto rv0 = sender->send_string(filter, ZMQ_SNDMORE)
    auto rv1 = sender->send(message_data)

    // verifies the return value
    if (rv0 != 0 && rv1 != 0) {
        using namespace date;
        cout << "[" << std::chrono::system_clock::now() << "]";
        cout << "[ZmqSender::send] Error while sending statistics via ZMQ."; 
    }
}

void ZmqSender::get_stat(){
    return stat;
}

void ZmqSender::get_mode(){
    return mode;
}
