#include <map>  // std::map
#include <thread>  // std::thread
#include <vector>  // std::vector

#include "zmq/zmq.hpp"
#include "nlohmann_json/json.hpp"
using json = nlohmann::json;

#ifndef PMAC_FILTER_CONTROLLER_H_
#define PMAC_FILTER_CONTROLLER_H_

enum ControlMode {
    DISABLE,  // Ignore messages
    CONTINUOUS,  // Run forever
    SINGLESHOT,  // Run until attenuation stablises and then pause at that attenuation until restarted

    CONTROL_MODE_SIZE  // Convenience for checking valid value range of ControlMode
};

enum ControlState {
    IDLE,  // Ignoring all messages
    WAITING,  // At max attenuation and waiting for messages
    ACTIVE,  // Receiving messages and healthy
    TIMEOUT,  // Waiting for timeout to be cleared to enter WAITING again
    SINGLESHOT_COMPLETE,  // Attenuation stablised in singleshot run and waiting for next run

    CONTROL_STATE_SIZE  // Convenience for checking valid value range of ControlState
};

class PMACFilterController
{
    public:
        PMACFilterController(
            const std::string& control_port,
            const std::vector<std::string>& subscribe_endpoints
        );
        ~PMACFilterController();
        void run();

    private:
        /* ZMQ */
        // Endpoint for control channel
        std::string control_channel_endpoint_;
        // Endpoints for data message subscribe channels
        std::vector<std::string> subscribe_channel_endpoints_;
        // Context
        zmq::context_t zmq_context_;
        // Sockets
        zmq::socket_t zmq_control_socket_;
        std::vector<zmq::socket_t> zmq_subscribe_sockets_;

        /* Internal Logic */
        // The current logic state
        ControlState state_;
        // The frame number of the last message that was received, but not necessarily processed - used to determine
        // that the attenuation level is stable in single-shot mode
        int64_t last_received_frame_;
        // The frame number of the last message that was successfully processed - used to decide to ignore some frames
        int64_t last_processed_frame_;
        // Time in seconds since last message received - not necessarily causing processing
        size_t time_since_last_message_;
        // Duration in microseconds of previous process
        size_t process_duration_;
        // Time elapsed in microseconds from one process to the next. This will include any time spent waiting for
        // messages and other housekeeping
        size_t process_period_;
        // Flag to start a new single shot run
        bool singleshot_start_;
        // Flag to reset timeout and re-enter continuous mode
        bool clear_timeout_;
        // Flag to interrupt listen loop and shutdown process
        bool shutdown_;
        // Thread to subscribe to data channel and control filters
        std::thread subscribe_thread_;

        /* Filter Logic */
        // Local store of current attenuation to compare against the next attenuation change request
        int current_attenuation_;
        // Filter positions from previous process for calculation of positions after filter in move
        std::vector<int> current_demand_;
        // Filter positions after filter in move applied
        std::vector<int> post_in_demand_;
        // Filter positions after filter in and out moves applied
        std::vector<int> final_demand_;

        /* Control Channel Parameters */
        // The current mode of operation
        ControlMode mode_;
        // Filter in and out positions in counts (can be +ve or -ve)
        std::vector<int> in_positions_;
        std::vector<int> out_positions_;
        // Thresholds for histogram bins above which some action should be taken
        std::map<std::string, uint64_t> pixel_count_thresholds_;

        bool _handle_request(const json& request, json& response);
        void _handle_status(json& response);
        bool _handle_config(const json& config);
        bool _set_mode(const ControlMode mode);
        bool _set_positions(std::vector<int>& positions, const json new_positions);
        bool _set_pixel_count_thresholds(json thresholds);
        void _process_data_channel();
        void _transition_state(ControlState state);
        void _process_singleshot_state();
        void _set_max_attenuation();
        void _calculate_process_metrics(const struct timespec& start_ts, struct timespec& end_ts);
        bool _process_data(const json& data);
        void _send_filter_adjustment(const int adjustment);
};

/* Helper methods */
json _parse_json_string(const std::string& json_string);
std::vector<std::string> _parse_endpoints(std::string endpoint_arg);
bool _message_queued(zmq::pollitem_t& pollitem);
bool _is_valid_request(const json& request);
void _get_time(struct timespec* ts);
size_t _useconds_since(const struct timespec& start_ts);
size_t _seconds_since(const struct timespec& start_ts);

#endif  // PMAC_FILTER_CONTROLLER_H_
