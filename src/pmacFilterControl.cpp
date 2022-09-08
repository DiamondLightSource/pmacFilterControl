// An application to listen on a ZMQ channel for histograms and adjust a filter set

#include <iostream>
#include <functional>  // std::bind
#include <time.h>  // timespec, CLOCK_REALTIME

#include "pmacFilterControl.h"

#ifdef __ARM_ARCH
#include "gplib.h"
#endif

#define VERSION 106

// Filter travel in counts to move a filter into the beam
#define FILTER_TRAVEL 100

// Command to send to motion controller to execute the motion program and move to the set demands
char RUN_PROG_1[] = "&2 #1,2,3,4J/ B1R";

// Control message keys
static const std::string COMMAND = "command";
static const std::string COMMAND_SHUTDOWN = "shutdown";
static const std::string COMMAND_CONFIGURE = "configure";
static const std::string COMMAND_RESET = "reset";
static const std::string PARAMS = "params";
static const std::string CONFIG_PIXEL_COUNT_THRESHOLD = "pixel_count_threshold";
// Data message keys
static const std::string FRAME_NUMBER = "frame_number";
static const std::string PARAMETERS = "parameters";
static const std::string PARAM_LOW1 = "low1";
static const std::string PARAM_LOW2 = "low2";
static const std::string PARAM_HIGH1 = "high1";
static const std::string PARAM_HIGH2 = "high2";

// The priority in which to process the thresholds.
// If PARAM_HIGH2 is triggered, then apply it, else if PARAM_HIGH1 is triggered, apply that, etc.
static const std::vector<std::string> THRESHOLD_PRECEDENCE = {
    PARAM_HIGH2, PARAM_HIGH1, PARAM_LOW2, PARAM_LOW1
};

// The attenuation adjustments to apply for a given threshold
// PARAM_HIGH2 -> Add 2 levels of attenuation
// PARAM_HIGH1 -> Add a level of attenuation
// PARAM_LOW1 -> Subtract 1 level of attenuation
// PARAM_LOW2 -> Subtract 2 levels of attenuation
static const std::map<std::string, int> THRESHOLD_ADJUSTMENTS = {
    {PARAM_HIGH2, 2}, {PARAM_HIGH1, 1}, {PARAM_LOW2, -2}, {PARAM_LOW1, -1}
};

// An initial invalid value to compare with `last_processed_frame_` that always passes the ignore frame checks
int64_t NO_FRAMES_PROCESSED = -1;


/*!
    @brief Constructor

    Setup ZeroMQ sockets

    @param[in] control_port Port number to bind control socket to
    @param[in] data_endpoint Endpoint (<IP>:<PORT>) to subscribe on for data messages
*/
PMACFilterController::PMACFilterController(
    const std::string& control_port,
    const std::string& data_endpoint
) :
    control_channel_endpoint_("tcp://*:" + control_port),
    data_channel_endpoint_("tcp://" + data_endpoint),
    zmq_context_(),
    zmq_control_socket_(zmq_context_, ZMQ_REP),
    zmq_data_socket_(zmq_context_, ZMQ_SUB),
    shutdown_(false),
    pixel_count_threshold_(2),
    last_processed_frame_(NO_FRAMES_PROCESSED),
    new_attenuation_(0),
    current_attenuation_(0),
    current_demand_(4, 0),
    post_in_demand_(4, 0),
    final_demand_(4, 0)
{
    this->zmq_control_socket_.bind(control_channel_endpoint_.c_str());
    this->zmq_data_socket_.connect(data_channel_endpoint_.c_str());
    this->zmq_data_socket_.setsockopt(ZMQ_SUBSCRIBE, "", 0);  // "" -> No topic filter
}

/*!
    @brief Destructor

    Close ZeroMQ sockets

*/
PMACFilterController::~PMACFilterController()
{
    this->zmq_control_socket_.close();
    this->zmq_data_socket_.close();
}

/*!
    @brief Handle the JSON request from the control channel

    @param[in] request json object of request

    @return true if the request was applied successfully, else false
*/
bool PMACFilterController::_handle_request(const json& request) {
    bool success = false;

    if (request[COMMAND] == COMMAND_SHUTDOWN) {
        std::cout << "Received shutdown command" << std::endl;
        this->shutdown_ = true;
        success = true;
    } else if (request[COMMAND] == COMMAND_RESET) {
        std::cout << "Resetting frame counter" << std::endl;
        this->last_processed_frame_ = NO_FRAMES_PROCESSED;
        success = true;
    } else if (request[COMMAND] == COMMAND_CONFIGURE) {
        json config = request[PARAMS];
        std::cout << "Received new config: " << config.dump() << std::endl;
        if (config.contains(CONFIG_PIXEL_COUNT_THRESHOLD)) {
            // TODO: Falls over if value is string
            this->pixel_count_threshold_ = config[CONFIG_PIXEL_COUNT_THRESHOLD];
            success = true;
        }
    }

    return success;
}

/*!
    @brief Spawn data monitor thread and listen for control requests until shutdown
*/
void PMACFilterController::run() {
    // Start data handler thread
    this->listenThread_ = std::thread(
        std::bind(&PMACFilterController::_process_data_channel, this)
    );

    // Listen for control messages
    std::string request_str;
    while (!this->shutdown_) {
        zmq::message_t request_msg;
        this->zmq_control_socket_.recv(&request_msg);
        request_str = std::string(
            static_cast<char*>(request_msg.data()), request_msg.size()
        );

        std::cout << "Request received: " << request_str << std::endl;
        bool success = false;
        json request = this->_parse_json_string(request_str);
        if (!request.is_null()) {
            success = this->_handle_request(request);
        }

        std::string response;
        if (success) {
            response = "ACK | " + request_str;
        } else {
            response = "NACK | " + request_str;
        }

        zmq::message_t response_msg(response.size());
        memcpy(response_msg.data(), response.c_str(), response.size());
        this->zmq_control_socket_.send(response_msg, 0);
    }

    std::cout << "Shutting down" << std::endl;

    this->listenThread_.join();
}

/*!
    @brief Listen on ZeroMQ data channel for messages and hand off for processing

    This function should be run in a spawned thread and will return when `shutdown_`.
*/
void PMACFilterController::_process_data_channel() {
    std::cout << "Listening on " << data_channel_endpoint_ << std::endl;

    std::string data_str;
    struct timespec start_ts, end_ts;
    size_t start_ns, end_ns;
    while (!this->shutdown_) {
        if (this->_poll(100)) {
            clock_gettime(CLOCK_REALTIME, &start_ts);
            zmq::message_t data_message;
            this->zmq_data_socket_.recv(&data_message);
            data_str = std::string(
                static_cast<char*>(data_message.data()), data_message.size()
            );
            std::cout << "Data received: " << data_str << std::endl;

            json data = this->_parse_json_string(data_str);
            if (!data.is_null()) {
                this->_process_data(data);
            }
            clock_gettime(CLOCK_REALTIME, &end_ts);

            // TODO: Put this in a method
            start_ns = ((size_t) start_ts.tv_sec * 1000000000) + (size_t) start_ts.tv_nsec;
            end_ns = ((size_t) end_ts.tv_sec * 1000000000) + (size_t) end_ts.tv_nsec;
            this->process_time_ = (end_ns - start_ns) / 1000;
            std::cout << "Process time: " << this->process_time_ << "us" << std::endl;
        }
    }
}

/*!
    @brief Handle the JSON request from the control channel

    Determine if the `data` should be processed based on the frame number and then if the histogram values require the
    attenuation level to be adjusted.

    @param[in] data json structure of data message
*/
void PMACFilterController::_process_data(const json& data) {
    if (data[FRAME_NUMBER] <= this->last_processed_frame_) {  // TODO: Crashes if no frame number, or parameters
        std::cout << "Ignoring frame " << data[FRAME_NUMBER]
            << " - already processed " << this->last_processed_frame_ << std::endl;
        return;
    }
    if (data[FRAME_NUMBER] == this->last_processed_frame_ + 1) {
        std::cout << "Ignoring subsequent frame" << std::endl;
        // Don't process two frames in succession as changes won't have taken effect
        return;
    }

    json histogram = data[PARAMETERS];
    std::vector<std::string>::const_iterator threshold;
    for (threshold = THRESHOLD_PRECEDENCE.begin(); threshold != THRESHOLD_PRECEDENCE.end(); threshold++) {
        // TODO: Should threshold be inclusive?
        if (histogram[*threshold] > this->pixel_count_threshold_) {
            std::cout << *threshold << " threshold triggered" << std::endl;
            std::cout << "Current threshold: " << this->pixel_count_threshold_ << std::endl;
            int adjustment = THRESHOLD_ADJUSTMENTS.at(*threshold);
            this->_send_filter_adjustment(adjustment);
            break;
        }
    }

    this->last_processed_frame_ = data[FRAME_NUMBER];
}

/*!
    @brief Validate and parse json from a string representation to create a json object

    Note that the returned json object can be invalid and should be tested with `is_null()` before access.

    @param[in] json_string String representation of a json structure

    @return json object parsed from string
*/
json PMACFilterController::_parse_json_string(const std::string& json_string) {
    json _json;
    // Call json::accept first to determine if the string is valid json, without throwing an exception, before calling
    // json::parse, which does throw an exception for invalid json
    if (json::accept(json_string)) {
        _json = json::parse(json_string);
    } else {
        std::cout << "Not valid JSON:\n" << json_string << std::endl;
    }

    return _json;
}

/*!
    @brief Send updated attenuation demand to the motion controller

    Calculate positions of individual filters based on a bitmask of the attenuation level, set the parameters on the
    motion controller and then execute the motion program to move the motors.

    The code to set variables through shared memory is inside of an ARM #ifdef fence, so when compiled for x86 it will
    just do the calculations and print a message.

    @param[in] adjustment Attenuation levels to change by (can be positive or negative)
*/
void PMACFilterController::_send_filter_adjustment(int adjustment) {
    this->new_attenuation_ = this->current_attenuation_ + adjustment;
    std::cout << "New attenuation: " << this->new_attenuation_ << std::endl;

    std::cout << "Adjustments (Current | In | Final):" << std::endl;
    for (int i = 0; i < 4; ++i) {
        this->final_demand_[i] = (this->new_attenuation_ >> i) & 1;
        this->post_in_demand_[i] = this->final_demand_[i] | this->current_demand_[i];
        std::cout << this->current_demand_[i] << " | "
            << this->post_in_demand_[i] << " | "
            << this->final_demand_[i] << std::endl;
    }

#ifdef __ARM_ARCH
    std::cout << "Changing attenuation: "
        << this->current_attenuation_ << " -> " << this->new_attenuation_ << std::endl;

    // Set demands on ppmac
    pshm->P[4071] = this->post_in_demand_[0] * FILTER_TRAVEL;
    pshm->P[4072] = this->post_in_demand_[1] * FILTER_TRAVEL;
    pshm->P[4073] = this->post_in_demand_[2] * FILTER_TRAVEL;
    pshm->P[4074] = this->post_in_demand_[3] * FILTER_TRAVEL;
    pshm->P[4081] = this->final_demand_[0] * FILTER_TRAVEL;
    pshm->P[4082] = this->final_demand_[1] * FILTER_TRAVEL;
    pshm->P[4083] = this->final_demand_[2] * FILTER_TRAVEL;
    pshm->P[4084] = this->final_demand_[3] * FILTER_TRAVEL;

    // Run the motion program
    CommandTS(RUN_PROG_1);
#else
    std::cout << "Not changing attenuation "
        << this->current_attenuation_ << " -> " << this->new_attenuation_ << std::endl;
#endif

    // Update current values for next incremental change
    for (int i = 0; i < 4; ++i) {
        this->current_demand_[i] = this->final_demand_[i];
    }
    this->current_attenuation_ = this->new_attenuation_;
}

/*!
    @brief Poll the ZeroMQ data socket for events

    If this function returns true then a subsequent `recv()` on the socket will return a message.

    @param[in] timeout_ms Poll duration in milliseconds

    @return true if there is a message on the socket, else false (after the timeout has elapsed)
*/
bool PMACFilterController::_poll(long timeout_ms)
{
    zmq::pollitem_t pollitems[] = {{this->zmq_data_socket_, 0, ZMQ_POLLIN, 0}};
    zmq::poll(pollitems, 1, timeout_ms);

    return (pollitems[0].revents & ZMQ_POLLIN);
}

/*!
    @brief Application entrypoint

    Validate command line arguments, create PMACFilterController and run.

    @return 0 when shutdown cleanly, 1 when given invalid arguments
*/
int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cout << "Must pass control_port and data_endpoint - "
            << "e.g. '10000 127.0.0.1:10000'" << std::endl;
        return 1;
    }

    std::cout << "Version: " << VERSION << std::endl;

    std::string control_port(argv[1]);
    std::string data_endpoint(argv[2]);
    PMACFilterController pfc(control_port, data_endpoint);

#ifdef __ARM_ARCH
    InitLibrary();
#endif

    pfc.run();
    std::cout << "Finished run" << std::endl;

#ifdef __ARM_ARCH
    CloseLibrary();
#endif

    return 0;
}
