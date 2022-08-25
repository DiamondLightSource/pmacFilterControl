#include <functional>
#include <map>
#include <thread>
#include <vector>

#include "zmq/zmq.hpp"
#include "nlohmann_json/json.hpp"
using json = nlohmann::json;

#ifndef PMAC_FILTER_CONTROLLER_H_
#define PMAC_FILTER_CONTROLLER_H_

class PMACFilterController
{
    public:
        PMACFilterController(
            const std::string& data_endpoint,
            const std::string& control_port
        );
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
        // Threshold for a histogram bin above which some action should be taken
        uint64_t pixel_count_threshold_;
        // The frame number of the last frame that was successfully processed
        // - Used to decide to ignore some frames
        int64_t last_processed_frame_;

        // Local store of current value to compare against the next attenuation change
        int current_attenuation_;
        // New attenuation value to apply after attenuation change is received
        int new_attenuation_;
        // Vectors of individual filter positions for a given attenuation level
        // Local store of filter positions for calculation of positions after filter in move
        std::vector<int> current_demand_;
        // Filter positions after filter in move applied
        std::vector<int> post_in_demand_;
        // Filter positions after filter in and out moves applied
        std::vector<int> final_demand_;

        bool _handle_request(const json& request);
        void _process_data_channel();
        bool _poll(long timeout_ms);
        void _process_data(const json& data);
        json _parse_json_string(const std::string& json_string);
        void _send_filter_adjustment(int adjustment);
};

#endif  // PMAC_FILTER_CONTROLLER_H_
