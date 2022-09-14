#include <map>  // std::map
#include <thread>  // std::thread
#include <vector>  // std::vector

#include "zmq/zmq.hpp"
#include "nlohmann_json/json.hpp"
using json = nlohmann::json;

#ifndef PMAC_FILTER_CONTROLLER_H_
#define PMAC_FILTER_CONTROLLER_H_

enum ControlMode {idle, active, oneshot};

class PMACFilterController
{
    public:
        PMACFilterController(
            const std::string& control_port,
            const std::vector<std::string>& data_endpoints
        );
        ~PMACFilterController();
        void run();

    private:
        // Endpoint for control channel
        std::string control_channel_endpoint_;
        // Endpoint for data channels
        std::vector<std::string> data_channel_endpoints_;
        // ZMQ Context
        zmq::context_t zmq_context_;
        // ZMQ Sockets
        zmq::socket_t zmq_control_socket_;
        std::vector<zmq::socket_t> zmq_data_sockets_;
        // Thread to subscribe to data channel and control filters
        std::thread listenThread_;
        // Flag to interupt listen loop and shutdown process
        bool shutdown_;
        // The frame number of the last frame that was successfully processed
        // - Used to decide to ignore some frames
        int64_t last_processed_frame_;

        // Local store of current attenuation to compare against the next attenuation change request
        int current_attenuation_;
        // New attenuation value to apply after attenuation change is processed
        int new_attenuation_;
        // Vectors of individual filter positions for a given attenuation level
        // Local store of filter positions for calculation of positions after filter in move
        std::vector<int> current_demand_;
        // Filter positions after filter in move applied
        std::vector<int> post_in_demand_;
        // Filter positions after filter in and out moves applied
        std::vector<int> final_demand_;

        // Duration in microseconds of previous process
        size_t process_time_;

        /* Control Channel Parameters */
        // The current mode of operation
        ControlMode mode_;
        // Threshold for a histogram bin above which some action should be taken
        uint64_t pixel_count_threshold_;
        // Filter in positions in counts (can be +ve or -ve)
        std::vector<int> in_positions_;

        bool _handle_request(const json& request);
        bool _handle_config(const json& config);
        bool _set_mode(std::string);
        bool _set_in_positions(json positions);
        void _process_data_channel();
        void _calculate_process_time(struct timespec& start_ts);
        void _process_data(const json& data);
        json _parse_json_string(const std::string& json_string);
        void _send_filter_adjustment(int adjustment);
};

/* Helper methods */
std::vector<std::string> _parse_endpoints(std::string endpoint_arg);
bool _message_queued(zmq::pollitem_t& pollitem);

#endif  // PMAC_FILTER_CONTROLLER_H_
