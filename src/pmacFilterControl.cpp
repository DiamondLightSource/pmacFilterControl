// An application to listen on a ZMQ channel for histograms and adjust a filter set

#include <iostream>

#include "pmacFilterControl.h"


// Configure message keys
static const std::string COMMAND = "command";
static const std::string COMMAND_SHUTDOWN = "shutdown";
static const std::string COMMAND_CONFIGURE = "configure";
static const std::string PARAMS = "params";
static const std::string CONFIG_PIXEL_COUNT_THRESHOLD = "pixel_count_threshold";
// Data message keys
static const std::string FRAME_NUMBER = "frame_number";
static const std::string PARAMETERS = "parameters";
static const std::string PARAM_LOW1 = "low1";
static const std::string PARAM_LOW2 = "low2";
static const std::string PARAM_HIGH1 = "high1";
static const std::string PARAM_HIGH2 = "high2";
// Filter calculation definitions
static const std::vector<std::string> THRESHOLD_PRECEDENCE = {
    PARAM_HIGH2, PARAM_HIGH1, PARAM_LOW2, PARAM_LOW1
};
static const std::map<std::string, int> THRESHOLD_ADJUSTMENTS = {
    {PARAM_HIGH2, 2}, {PARAM_HIGH1, 1}, {PARAM_LOW2, -2}, {PARAM_LOW1, -1}
};


PMACFilterController::PMACFilterController(const std::string& control_port, const std::string& data_endpoint) :
    control_channel_endpoint_("tcp://*:" + control_port),
    data_channel_endpoint_("tcp://" + data_endpoint),
    zmq_context_(),
    zmq_control_socket_(zmq_context_, ZMQ_REP),
    zmq_data_socket_(zmq_context_, ZMQ_SUB),
    shutdown_(false),
    pixel_count_threshold_(2),
    last_actioned_frame_(0)
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

bool PMACFilterController::_configure(const json& config) {
    std::cout << "Received new config: " << config.dump() << std::endl;

    bool success = false;
    if (config.contains(CONFIG_PIXEL_COUNT_THRESHOLD)) {
        this->pixel_count_threshold_ = config[CONFIG_PIXEL_COUNT_THRESHOLD];
        success = true;
    }

    return success;
}

void PMACFilterController::run() {
    // Start data handler thread
    this->listenThread_ = std::thread(std::bind(&PMACFilterController::_process_data_channel, this));

    // Listen for control messages
    std::string request_str;
    while (!this->shutdown_) {
        zmq::message_t request_msg;
        this->zmq_control_socket_.recv(&request_msg);
        request_str = std::string(static_cast<char*>(request_msg.data()), request_msg.size());

        json request = json::parse(request_str);
        bool success;
        std::string response;
        if (request[COMMAND] == COMMAND_SHUTDOWN) {
            std::cout << "Shutting down" << std::endl;
            this->shutdown_ = true;
            success = true;
        } else if (request[COMMAND] == COMMAND_CONFIGURE) {
            success = this->_configure(request[PARAMS]);
        } else {
            std::cout << "Unknown command received: " << request << std::endl;
            success = false;
        }

        if (success) {
            response = "ACK | " + request_str;
        } else {
            response = "NACK | " + request_str;
        }

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
            this->_process_data_message(message);
        }
    }
}

void PMACFilterController::_process_data_message(const std::string& data_message) {
    json data = json::parse(data_message);

    if (data[FRAME_NUMBER] = this->last_actioned_frame_ + 1) {
        // Don't process two frames in succession as changes won't have taken effect
        return;
    }

    json histogram = data[PARAMETERS];
    std::vector<std::string>::const_iterator threshold;
    for (threshold = THRESHOLD_PRECEDENCE.begin(); threshold != THRESHOLD_PRECEDENCE.end(); threshold++) {
        if (histogram[*threshold] > this->pixel_count_threshold_) {
            std::cout << *threshold << " threshold triggered: " << histogram[*threshold] << std::endl;
            int adjustment = THRESHOLD_ADJUSTMENTS.at(*threshold);
            this->_send_filter_adjustment(adjustment);
            break;
        }
    }

    this->last_actioned_frame_ = data[FRAME_NUMBER];
}

void PMACFilterController::_send_filter_adjustment(int adjustment) {
    std::cout << "Send filter adjustment " << adjustment << std::endl;
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
