#ifndef ZMQSENDER_H
#define ZMQSENDER_H

#include <string>
#include <memory>
#include <tuple>
#include <zmq.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#include <boost/property_tree/json_parser.hpp>
#include <chrono>
#include "date.h"

#include "RingBuffer.hpp"

class ZmqSender
{
    const std::string connect_address;
    const std::string filter;
    const int receive_timeout;
    std::shared_ptr<zmq::socket_t> sender = NULL;
    std::shared_ptr<zmq::context_t> context = NULL;

    public:
        ZmqSender(const std::string& connect_address, 
                  const int n_io_threads, const int receive_timeout);

        virtual ~ZmqSender(){};

        void bind();

        void send(const std::string& filter, zmq::message_t message_data);

};

#endif