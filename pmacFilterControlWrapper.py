import asyncio
import codecs
import logging

import cothread
import numpy as np
import json
import zmq
from softioc import builder

from .zmqadapter import ZeroMQAdapter

STATES = ["IDLE", "WAITING", "ACTIVE", "TIMEOUT", "SINGLESHOT COMPLETE"]

MODE = [
    "DISABLE",
    "CONTINUOUS",
    "SINGLE-SHOT",
]

FILTER_SET = [
    "Cu",
    "Mo 1",
    "Mo 2",
    "Mo 3",
    "Ag 1",
    "Ag 2",
]


class Wrapper:

    POLL_PERIOD = 0.1
    RETRY_PERIOD = 5

    def __init__(
        self,
        ip: str,
        port: int,
        event_stream_port: int,
        builder: builder,
        device_name: str,
        filter_set_total: int,
        filters_per_set: int,
    ):

        self._log = logging.getLogger(self.__class__.__name__)

        self.ip = ip
        self.port = port
        self.zmq_stream = ZeroMQAdapter(ip, port)
        self.event_stream = ZeroMQAdapter(ip, event_stream_port, zmq_type=zmq.SUB)

        self.pixel_count_thresholds = {"high1": 2, "high2": 2, "low1": 2, "low2": 2}

        self.device_name = device_name

        self.version = builder.stringIn("VERSION")
        self.state = builder.mbbIn("STATE", *STATES)

        self.mode = builder.mbbOut("MODE", *MODE, on_update=self._set_mode)
        self.mode_rbv = builder.mbbIn("MODE_RBV", *MODE)

        self.reset = builder.boolOut("RESET", on_update=self._reset)

        self.timeout = builder.aOut(
            "TIMEOUT", initial_value=3, on_update=self._set_timeout
        )
        self.timeout_rbv = builder.aIn("TIMEOUT_RBV", initial_value=3)
        self.clear_timeout = builder.boolOut(
            "TIMEOUT:CLEAR", on_update=self._clear_timeout
        )

        self.singleshot_start = builder.boolOut(
            "SINGLESHOT:START", on_update=self._start_singleshot
        )

        self.upper_high_threshold = builder.aOut(
            "HIGH:THRESHOLD:UPPER",
            initial_value=2,
            on_update=self._set_upper_high_threshold,
        )
        self.lower_high_threshold = builder.aOut(
            "HIGH:THRESHOLD:LOWER",
            initial_value=2,
            on_update=self._set_lower_high_threshold,
        )
        self.upper_low_threshold = builder.aOut(
            "LOW:THRESHOLD:UPPER",
            initial_value=2,
            on_update=self._set_upper_low_threshold,
        )
        self.lower_low_threshold = builder.aOut(
            "LOW:THRESHOLD:LOWER",
            initial_value=2,
            on_update=self._set_lower_low_threshold,
        )

        self.filter_set = builder.mbbOut(
            "FILTER_SET", *FILTER_SET, initial_value=0, on_update=self._set_filter_set
        )
        self.filter_set_rbv = builder.mbbIn(
            "FILTER_SET_RBV", *FILTER_SET, initial_value=0
        )

        self.file_path = builder.stringOut("FILE:PATH", on_update=self._set_file_path)
        self.file_name = builder.stringOut("FILE:NAME", on_update=self._set_file_name)
        self.file_full_name = builder.stringIn("FILE:FULL_NAME")

        self.process_duration = builder.aIn("PROCESS:DURATION")
        self.process_period = builder.aIn("PROCESS:PERIOD")

        self.last_frame_received = builder.aIn("FRAME:RECEIVED")
        self.last_frame_processed = builder.aIn("FRAME:PROCESSED")
        self.time_since_last_frame = builder.aIn("FRAME:LAST_TIME")

        self.current_attenuation = builder.aIn("ATTENUATION_RBV")

        self.filter_sets_in = {}
        self.filter_sets_out = {}
        for i in range(1, filter_set_total + 1):
            filter_set_key = f"filter_set_{i}"
            self.filter_sets_in[filter_set_key] = {}
            self.filter_sets_out[filter_set_key] = {}

            for j in range(1, filters_per_set + 1):

                in_key = f"filter_set_{i}_in_pos_{j}"
                in_value = builder.aOut(
                    f"FILTER_SET:{i}:IN:{j}",
                    initial_value=100,
                    on_update=lambda _, i=i: self._set_in_pos(i),
                )
                self.filter_sets_in[filter_set_key][in_key] = in_value

                out_key = f"filter_set_{i}_out_pos_{j}"
                out_value = builder.aOut(
                    f"FILTER_SET:{i}:OUT:{j}",
                    initial_value=0,
                    on_update=lambda _, i=i: self._set_out_pos(i),
                )
                self.filter_sets_out[filter_set_key][out_key] = out_value

    async def run_forever(self) -> None:

        print("Connecting to ZMQ stream...")

        req_status = b'{"command":"status"}'

        self._send_message(req_status)
        # resp = await self.zmq_stream.get_response()

        # if resp:
        #     print("Connected.")

        await asyncio.gather(
            *[
                self.monitor_responses(self.zmq_stream),
                self.monitor_responses(self.event_stream),
                self.zmq_stream.run_forever(),
                self.event_stream.run_forever(),
                self._query_status(),
            ]
        )

    async def monitor_responses(self, zmq_stream: ZeroMQAdapter) -> None:
        running = True
        while running:
            resp = await zmq_stream.get_response()
            resp_json = json.loads(resp[0])

            if "status" in resp_json:
                status = resp_json["status"]

                self._handle_status(status)

            running = zmq_stream.check_if_running()

    async def _query_status(self) -> None:
        running = True
        while running:
            if self.zmq_stream.running:
                req_status = b'{"command":"status"}'
                self._send_message(req_status)
                running = self.zmq_stream.check_if_running()
            await asyncio.sleep(0.1)

    def _handle_status(self, status: json) -> None:

        state = status["state"]
        self.state.set(state)

        version = status["version"]
        self.version.set(str(version))

        process_duration = status["process_duration"]
        self.process_duration.set(process_duration)

        process_period = status["process_period"]
        self.process_period.set(process_period)

        last_received_frame = status["last_received_frame"]
        self.last_frame_received.set(last_received_frame)

        last_processed_frame = status["last_processed_frame"]
        self.last_frame_processed.set(last_processed_frame)

        time_since_last_frame = status["time_since_last_message"]
        self.time_since_last_frame.set(time_since_last_frame)

        current_attenuation = status["current_attenuation"]
        self.current_attenuation.set(current_attenuation)

    def _send_message(self, message: bytes) -> bytes:
        self.zmq_stream.send_message([message])

    def _set_mode(self, mode: int) -> None:

        # Set mode for PFC
        mode_config = json.dumps({"command": "configure", "params": {"mode": mode}})
        self._send_message(codecs.encode(mode_config, "utf-8"))

        self.mode_rbv.set(mode)

    def _reset(self, _) -> None:
        if _ == 1:
            reset = b'{"command":"reset"}'
            self._send_message(reset)

    def _set_timeout(self, timeout: int) -> None:

        # Set timeout for PFC

        self.timeout_rbv.set(timeout)

    def _clear_timeout(self, _) -> None:

        if _ == 1:
            clear_timeout = json.dumps({"command": "clear_timeout"})
            self._send_message(codecs.encode(clear_timeout, "utf-8"))

    def _start_singleshot(self, _) -> None:

        if _ == 1:

            if self.state.get() == 1 and self.mode_rbv.get() == 2:
                start_singleshot = json.dumps({"command": "singleshot"})
                self._send_message(codecs.encode(start_singleshot, "utf-8"))
            else:
                print("WARNING: Must be in SINGLESHOT mode and WAITING state.")

    def _set_thresholds(self) -> None:

        set_thresholds = json.dumps(
            {
                "command": "configure",
                "params": {"pixel_count_thresholds": self.pixel_count_thresholds},
            }
        )
        self._send_message(codecs.encode(set_thresholds, "utf-8"))

    def _set_upper_high_threshold(self, threshold: int) -> None:

        if threshold != self.pixel_count_thresholds["high2"]:
            self.pixel_count_thresholds["high2"] = threshold

            # Set upper high threshold for PFC
            self._set_thresholds()

        else:
            print(f"High 2 is already at value {threshold}.")

    def _set_lower_high_threshold(self, threshold: int) -> None:

        if threshold != self.pixel_count_thresholds["high1"]:
            self.pixel_count_thresholds["high1"] = threshold

            # Set lower high threshold for PFC
            self._set_thresholds()

        else:
            print(f"High 1 is already at value {threshold}.")

    def _set_upper_low_threshold(self, threshold: int) -> None:

        if threshold != self.pixel_count_thresholds["low2"]:
            self.pixel_count_thresholds["low2"] = threshold

            # Set upper high threshold for PFC
            self._set_thresholds()

        else:
            print(f"Low 2 is already at value {threshold}.")

    def _set_lower_low_threshold(self, threshold: int) -> None:

        if threshold != self.pixel_count_thresholds["low1"]:
            self.pixel_count_thresholds["low1"] = threshold

            # Set upper high threshold for PFC
            self._set_thresholds()

        else:
            print(f"Low 1 is already at value {threshold}.")

    def _set_filter_set(self, filter_set_num: int) -> None:

        in_positions = [
            x.get()
            for x in self.filter_sets_in[f"filter_set_{filter_set_num+1}"].values()
        ]
        out_positions = [
            x.get()
            for x in self.filter_sets_out[f"filter_set_{filter_set_num+1}"].values()
        ]

        in_pos = {}
        for id, pos in enumerate(in_positions):
            in_pos[f"filter{id+1}"] = pos

        out_pos = {}
        for id, pos in enumerate(out_positions):
            out_pos[f"filter{id+1}"] = pos

        # Set filter set positions for PFC
        set_filter_set = json.dumps(
            {
                "command": "configure",
                "params": {"in_positions": in_pos, "out_positions": out_pos},
            }
        )

        self._send_message(codecs.encode(set_filter_set, "utf-8"))

        self.filter_set_rbv.set(filter_set_num)

    def _set_in_pos(self, filter_set: int) -> None:

        if self.filter_set_rbv.get() == filter_set - 1:
            self._set_filter_set(filter_set - 1)

    def _set_out_pos(self, filter_set: int) -> None:

        if self.filter_set.get() == filter_set - 1:
            self._set_filter_set(filter_set - 1)

    def _set_file_path(self, path: str) -> None:

        # Set file path for PFC

        self._combine_file_path_and_name()

    def _set_file_name(self, name: str) -> None:

        # Set file name for PFC

        self._combine_file_path_and_name()

    def _combine_file_path_and_name(self) -> None:

        path: str = self.file_path.get()
        name: str = self.file_name.get()

        full_path: str = "/".join([path, name])

        self.file_full_name.set(full_path)
