// An application to listen on a ZMQ channel for histograms and adjust a filter set

#include <iostream>

#include "pmacFilterControl.h"


const static std::string SHUTDOWN = "shutdown";


PMACFilterController::PMACFilterController(std::string& control_port, std::string& data_endpoint) :
    control_channel_endpoint_("tcp://*:" + control_port),
    data_channel_endpoint_("tcp://" + data_endpoint),
    zmq_context_(),
    zmq_control_socket_(zmq_context_, ZMQ_REP),
    zmq_data_socket_(zmq_context_, ZMQ_SUB),
    shutdown_(false)
{
    this->zmq_control_socket_.bind(control_channel_endpoint_.c_str());
    this->zmq_data_socket_.connect(data_channel_endpoint_.c_str());
    this->zmq_data_socket_.setsockopt(ZMQ_SUBSCRIBE, "", 0);  // No topic filter
}

PMACFilterController::~PMACFilterController()
{
    this->zmq_control_socket_.close();
    this->zmq_data_socket_.close();
}

void PMACFilterController::run() {
    // Start data handler thread
    this->listenThread_ = std::thread(std::bind(&PMACFilterController::_process_data_channel, this));

    // Listen for control messages
    std::string identity, command;
    while (!this->shutdown_) {
        zmq::message_t identity_msg;
        zmq::message_t command_msg;
        this->zmq_control_socket_.recv(&command_msg);
        command = std::string(static_cast<char*>(command_msg.data()), command_msg.size());

        std::string response;
        if (command == SHUTDOWN) {
            std::cout << "Shutting down" << std::endl;
            response = "ACK | ";
            this->shutdown_ = true;
        } else {
            std::cout << "Unknown command received: " << command << std::endl;
            response = "NACK | Unknown command: ";
        }

        response += command;
        zmq::message_t response_msg(response.size());
        memcpy(response_msg.data(), response.c_str(), response.size());
        this->zmq_control_socket_.send(response_msg, 0);
    }

    this->listenThread_.join();
    std::cout << "Finished run" << std::endl;
}

void PMACFilterController::_process_data_channel() {
    std::cout << "Listening on " << data_channel_endpoint_ << std::endl;

    std::string message;
    while (!this->shutdown_) {
        if (this->_poll(100)) {
            zmq::message_t zmq_message;
            bool received_message = this->zmq_data_socket_.recv(&zmq_message);
            message = std::string(static_cast<char*>(zmq_message.data()), zmq_message.size());
            std::cout << "Data received: " << message << std::endl;
        }
    }
}

bool PMACFilterController::_poll(long timeout_ms)
{
  zmq::pollitem_t pollitems[] = {{this->zmq_data_socket_, 0, ZMQ_POLLIN, 0}};
  zmq::poll(pollitems, 1, timeout_ms);

  return (pollitems[0].revents & ZMQ_POLLIN);
}

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cout << "Must pass control_port and data_endpoint, e.g. '10000 127.0.0.1:10000'" << std::endl;
        return 1;
    }

    std::string control_port(argv[1]);
    std::string data_endpoint(argv[2]);
    PMACFilterController pfc(control_port, data_endpoint);

    pfc.run();

    return 0;
}
