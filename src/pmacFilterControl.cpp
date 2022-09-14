// An application to listen on a ZMQ channel for histograms and adjust a filter set

#include <iostream>  // std::cout
#include <functional>  // std::bind
#include <time.h>  // timespec, CLOCK_REALTIME
#include <sstream>  // std::stringstream, std::getline

#include "pmacFilterControl.h"

#ifdef __ARM_ARCH
#include "gplib.h"
#endif

#define VERSION 106

const int FILTER_TRAVEL = 100;  // Filter travel in counts to move a filter into the beam
const int MAX_ATTENUATION = 15;  // All filters in: 1 + 2 + 4 + 8
const long POLL_TIMEOUT = 100;  // Length of ZMQ poll in milliseconds
const int FILTER_COUNT = 4;  // Number of filters

// Command to send to motion controller to execute the motion program and move to the set demands
char RUN_PROG_1[] = "&2 #1,2,3,4J/ B1R";
char CLOSE_SHUTTER[] = "#5J=1000";

// Control message keys
const std::string COMMAND = "command";
const std::string COMMAND_SHUTDOWN = "shutdown";
const std::string COMMAND_CONFIGURE = "configure";
const std::string COMMAND_RESET = "reset";
const std::string PARAMS = "params";
const std::string CONFIG_PIXEL_COUNT_THRESHOLD = "pixel_count_threshold";
const std::string CONFIG_MODE = "mode";
const std::string CONFIG_MODE_IDLE = "idle";
const std::string CONFIG_MODE_ACTIVE = "active";
const std::string CONFIG_MODE_ONESHOT = "oneshot";
const std::string CONFIG_IN_POSITIONS = "in_positions";
const std::string FILTER_1_KEY = "filter1";
const std::string FILTER_2_KEY = "filter2";
const std::string FILTER_3_KEY = "filter3";
const std::string FILTER_4_KEY = "filter4";
const std::map<std::string, int> FILTER_MAP = {
    {FILTER_1_KEY, 0},
    {FILTER_2_KEY, 1},
    {FILTER_3_KEY, 2},
    {FILTER_4_KEY, 3}
};

// Data message keys
const std::string FRAME_NUMBER = "frame_number";
const std::string PARAMETERS = "parameters";
const std::string PARAM_LOW1 = "low1";
const std::string PARAM_LOW2 = "low2";
const std::string PARAM_HIGH1 = "high1";
const std::string PARAM_HIGH2 = "high2";

// The priority in which to process the thresholds.
// If PARAM_HIGH2 is triggered, then apply it, else if PARAM_HIGH1 is triggered, apply that, etc.
const std::vector<std::string> THRESHOLD_PRECEDENCE = {
    PARAM_HIGH2, PARAM_HIGH1, PARAM_LOW2, PARAM_LOW1
};

// The attenuation adjustments to apply for a given threshold
// PARAM_HIGH2 -> Add 2 levels of attenuation
// PARAM_HIGH1 -> Add a level of attenuation
// PARAM_LOW1 -> Subtract 1 level of attenuation
// PARAM_LOW2 -> Subtract 2 levels of attenuation
const std::map<std::string, int> THRESHOLD_ADJUSTMENTS = {
    {PARAM_HIGH2, 2}, {PARAM_HIGH1, 1}, {PARAM_LOW2, -2}, {PARAM_LOW1, -1}
};

// An initial invalid value to compare with `last_processed_frame_` that always passes the ignore frame checks
const int64_t NO_FRAMES_PROCESSED = -1;


/*!
    @brief Constructor

    Setup ZeroMQ sockets

    @param[in] control_port Port number to bind control socket to
    @param[in] data_endpoints Vector of endpoints (`IP`:`PORT`) to subscribe on for data messages
*/
PMACFilterController::PMACFilterController(
    const std::string& control_port,
    const std::vector<std::string>& data_endpoints
) :
    control_channel_endpoint_("tcp://*:" + control_port),
    data_channel_endpoints_(data_endpoints),
    zmq_context_(),
    zmq_control_socket_(zmq_context_, ZMQ_REP),
    zmq_data_sockets_(),
    shutdown_(false),
    last_processed_frame_(NO_FRAMES_PROCESSED),
    new_attenuation_(0),
    current_attenuation_(0),
    current_demand_(FILTER_COUNT, 0),
    post_in_demand_(FILTER_COUNT, 0),
    final_demand_(FILTER_COUNT, 0),
    // Default config parameter values
    mode_(ControlMode::active),
    pixel_count_threshold_(2),
    in_positions_({100, 100, 100, 100})
{
    this->zmq_control_socket_.bind(control_channel_endpoint_.c_str());

    // Open sockets to subscribe to data endpoints
    for (int idx = 0; idx != this->data_channel_endpoints_.size(); idx++) {
        this->zmq_data_sockets_.push_back(zmq::socket_t(this->zmq_context_, ZMQ_SUB));
        // Only recv most recent message
        const int conflate = 1;
        this->zmq_data_sockets_[idx].setsockopt(ZMQ_CONFLATE, &conflate, sizeof(conflate));
        // Subscribe to all topics ("" -> No topic filter)
        this->zmq_data_sockets_[idx].setsockopt(ZMQ_SUBSCRIBE, "", 0);
        this->zmq_data_sockets_[idx].connect(this->data_channel_endpoints_[idx].c_str());
    }
}

/*!
    @brief Destructor

    Close ZeroMQ sockets

*/
PMACFilterController::~PMACFilterController() {
    this->zmq_control_socket_.close();
    std::vector<zmq::socket_t>::iterator it;
    for (it = this->zmq_data_sockets_.begin(); it != this->zmq_data_sockets_.end(); it++) {
        it->close();
    }
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
        if (request.contains(PARAMS)) {
            json config = request[PARAMS];
            std::cout << "Received new config: " << config.dump() << std::endl;
            try {
                success = this->_handle_config(config);
            } catch (json::type_error& e) {
                std::cout << "Type error when handling config" << std::endl;
                success = false;
            }
        } else {
            std::cout << "Received config command with no parameters" << std::endl;
            success = false;
        }
    }

    return success;
}

/*!
    @brief Handle a configuration request

    @param[in] config json object of config parameters

    @throw json::type_error if given a config parameter with the wrong type

    @return true if all given parameters applied successfully, false if one or more failed or no parameters found
*/
bool PMACFilterController::_handle_config(const json& config) {
    bool success = false;

    if (config.contains(CONFIG_PIXEL_COUNT_THRESHOLD)) {
        this->pixel_count_threshold_ = config[CONFIG_PIXEL_COUNT_THRESHOLD];
        success = true;
    }
    if (config.contains(CONFIG_MODE)) {
        success = this->_set_mode(config[CONFIG_MODE]);
    }
    if (config.contains(CONFIG_IN_POSITIONS)) {
        success = this->_set_in_positions(config[CONFIG_IN_POSITIONS]);
    }

    if (!success) {
        std::cout << "Found no valid config parameters" << std::endl;
    }

    return success;
}

/*!
    @brief Set the mode enum based on a string representation

    @param[in] mode String of mode to set

    @throw json::type_error if given a config parameter with the wrong type

    @return true if the mode was set successfully, else false
*/
bool PMACFilterController::_set_mode(std::string mode) {
    bool success = true;

    std::cout << "Changing to " << mode << " mode" << std::endl;

    if (mode == CONFIG_MODE_IDLE) {
        this->mode_ = ControlMode::idle;
    }
    else if (mode == CONFIG_MODE_ACTIVE) {
        this->mode_ = ControlMode::active;
    }
    else if (mode == CONFIG_MODE_ONESHOT) {
        this->mode_ = ControlMode::oneshot;
    } else {
        std::cout << "Unknown mode: " << mode << std::endl;
        success = false;
    }

    return success;
}

/*!
    @brief Set the in position of the given filters

    @param[in] positions json dictionary of filter in positions - e.g. {"1": 100, "2": -100, ...}

    @throw json::type_error if given a config parameter with the wrong type

    @return true if at least one position was set, else false
*/
bool PMACFilterController::_set_in_positions(json positions) {
    bool success = false;

    std::map<std::string, int>::const_iterator item;
    for(item = FILTER_MAP.begin(); item != FILTER_MAP.end(); ++item) {
        if (positions.contains(item->first)) {
            this->in_positions_[item->second] = positions[item->first];
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
    // Construct pollitems for data sockets
    zmq::pollitem_t pollitems[this->zmq_data_sockets_.size()] = {};
    for (int idx = 0; idx != this->zmq_data_sockets_.size(); idx++) {
        zmq::pollitem_t pollitem = {this->zmq_data_sockets_[idx], 0, ZMQ_POLLIN, 0};
        pollitems[idx] = pollitem;
    }

    std::cout << "Listening for messages..." << std::endl;

    std::string data_str;
    struct timespec start_ts;
    while (!this->shutdown_) {
        if (this->mode_ != ControlMode::idle) {
            // Poll data sockets
            zmq::poll(&pollitems[0], this->zmq_data_sockets_.size(), POLL_TIMEOUT);
            for (int idx = 0; idx != this->zmq_data_sockets_.size(); idx++) {
                if (_message_queued(pollitems[idx])) {
                    clock_gettime(CLOCK_REALTIME, &start_ts);

                    zmq::message_t data_message;
                    this->zmq_data_sockets_[idx].recv(&data_message);

                    data_str = std::string(
                        static_cast<char*>(data_message.data()), data_message.size()
                    );
                    std::cout << "Data received: " << data_str << std::endl;

                    json data = this->_parse_json_string(data_str);
                    if (!data.is_null()) {
                        this->_process_data(data);
                    }

                    this->_calculate_process_time(start_ts);
                }
            }
        }
    }
}

/*!
    @brief Calculate and store time since given timespec

    @param[in] start_ts Timespec of process time
*/
void PMACFilterController::_calculate_process_time(struct timespec& start_ts) {
    struct timespec end_ts;
    size_t start_ns, end_ns;
    clock_gettime(CLOCK_REALTIME, &end_ts);
    start_ns = ((size_t) start_ts.tv_sec * 1000000000) + (size_t) start_ts.tv_nsec;
    end_ns = ((size_t) end_ts.tv_sec * 1000000000) + (size_t) end_ts.tv_nsec;
    this->process_time_ = (end_ns - start_ns) / 1000;
    std::cout << "Process time: " << this->process_time_ << "us" << std::endl;
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

    if (this->new_attenuation_ <= 0) {
        std::cout << "Min Attenuation" << std::endl;
        this->new_attenuation_ = 0;
    } else if (this->new_attenuation_ == MAX_ATTENUATION) {
        std::cout << "Max Attenuation" << std::endl;
    } else if (this->new_attenuation_ > MAX_ATTENUATION) {
        std::cout << "Max Attenuation Exceeded " << std::endl;
        this->new_attenuation_ = MAX_ATTENUATION;
#ifdef __ARM_ARCH
        CommandTS(CLOSE_SHUTTER);
#endif
    }

    std::cout << "New attenuation: " << this->new_attenuation_ << std::endl;

    std::cout << "Adjustments (Current | In | Final):" << std::endl;
    for (int idx = 0; idx < FILTER_COUNT; ++idx) {
        this->final_demand_[idx] = (this->new_attenuation_ >> idx) & 1;
        this->post_in_demand_[idx] = this->final_demand_[idx] | this->current_demand_[idx];
        std::cout << this->current_demand_[idx] << " | "
            << this->post_in_demand_[idx] << " | "
            << this->final_demand_[idx] << std::endl;
    }

#ifdef __ARM_ARCH
    std::cout << "Changing attenuation: "
        << this->current_attenuation_ << " -> " << this->new_attenuation_ << std::endl;

    // Set demands on ppmac (P407X and P408X)
    // - 0/1 * `position` -> 0 for out or `position` for in
    for (int idx = 0; idx < FILTER_COUNT; ++idx) {
        pshm->P[4071 + idx] = this->post_in_demand_[idx] * this->in_positions_[idx];
        pshm->P[4081 + idx] = this->final_demand_[idx] * this->in_positions_[idx];
    }

    // Run the motion program
    CommandTS(RUN_PROG_1);
#else
    std::cout << "Not changing attenuation "
        << this->current_attenuation_ << " -> " << this->new_attenuation_ << std::endl;
#endif

    // Update current values for next incremental change
    for (int idx = 0; idx < FILTER_COUNT; ++idx) {
        this->current_demand_[idx] = this->final_demand_[idx];
    }
    this->current_attenuation_ = this->new_attenuation_;
}

/*!
    @brief Application entrypoint

    Validate command line arguments, create PMACFilterController and run.

    @return 0 when shutdown cleanly, 1 when given invalid arguments
*/
int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " control_port data_endpoint\n"
            << "e.g. '" << argv[0] << " 10000 127.0.0.1:10001'" << std::endl;

        if (argc == 2 && std::string(argv[1]) == "--help") {
            return 0;
        }
        return 1;
    }

    std::cout << "Version: " << VERSION << std::endl;

    std::string control_port(argv[1]);
    std::vector<std::string> data_endpoints = _parse_endpoints(std::string(argv[2]));

    PMACFilterController pfc(control_port, data_endpoints);

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


/* Helper Methods */

/*!
    @brief Parse comma-separated string of endpoints from command line

    @param[in] endpoint_arg Comma-separated string of endpoints

    @return Vector of endpoints
*/
std::vector<std::string> _parse_endpoints(std::string endpoint_arg) {
    std::stringstream stream(endpoint_arg);

    std::vector<std::string> endpoint_vector;
    std::string endpoint;
    while (std::getline(stream, endpoint, ',')) {
        endpoint_vector.push_back("tcp://" + endpoint);
    }

    return endpoint_vector;
}

/*!
    @brief Check if a message is queued on the socket corresponding to the pollitem

    If this function returns true, then a `recv()` on the socket will return a message immediately.

    @param[in] pollitem `zmq_pollitem` corresponding to a `zmq_socket`

    @return true if there is a message queued on the socket, else false
*/
bool _message_queued(zmq::pollitem_t& pollitem) {
    return pollitem.revents & ZMQ_POLLIN;
}
