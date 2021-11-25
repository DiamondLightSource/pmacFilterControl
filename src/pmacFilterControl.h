#include <thread>
#include "zmq/zmq.hpp"

#ifndef PMAC_FILTER_CONTROLLER_H_
#define PMAC_FILTER_CONTROLLER_H_

class PMACFilterController
{
    public:
        PMACFilterController(std::string& data_endpoint, std::string& control_port);
        ~PMACFilterController();
        void run();

    private:
        // Endpoint for control channel
        std::string control_channel_endpoint_;
        // Endpoint for data channel
        std::string data_channel_endpoint_;
        // ZMQ Context
        zmq::context_t zmq_context_;
        // ZMQ Sockets
        zmq::socket_t zmq_control_socket_, zmq_data_socket_;
        // Thread to subscribe to data channel and control filters
        std::thread listenThread_;
        // Flag to interupt listen loop and shutdown process
        bool shutdown_;

        void _process_data_channel();
        bool _poll(long timeout_ms);
};

#endif  // PMAC_FILTER_CONTROLLER_H_
