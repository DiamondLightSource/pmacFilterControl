import asyncio
import codecs
import logging
from typing import Any, Dict, List, Optional, Tuple

import cothread
import numpy as np
import json
from cothread.catools import caput
from softioc import builder, asyncio_dispatcher

from .zmqadapter import ZeroMQAdapter

STATES =[
    "Idle",
    "Waiting",
    "Active",
    "Timeout",
    "Singleshot Complete"
]

MODE = [
    "Disable",
    "Continuous",
    "Single-shot",
]

FILTER_SET = [
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    13,
    14,
    15,
]

FILTER_MODE = [
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

    def __init__(self, ip: str, port: int, builder: builder, device_name: str):

        self._log = logging.getLogger(self.__class__.__name__)

        self.ip = ip
        self.port = port
        self.zmq_stream = ZeroMQAdapter(ip, port)

        self.device_name = device_name

        self.version = builder.stringIn("VERSION")
        self.state = builder.stringIn("STATE")

        self.mode = builder.mbbOut("MODE", *MODE, on_update=self._set_mode)
        self.mode_rbv = builder.mbbIn("MODE_RBV", *MODE)

        self.reset = builder.boolOut("RESET", on_update=self._reset)

        self.timeout = builder.aOut("TIMEOUT", on_update=self._set_timeout)
        self.timeout_rbv = builder.aIn("TIMEOUT_RBV")
        self.clear_timeout = builder.boolOut(
            "TIMEOUT:CLEAR", on_update=self._clear_timeout
        )

        self.singleshot_start = builder.boolOut(
            "SINGLESHOT:START", on_update=self._start_singleshot
        )

        self.upper_high_threshold = builder.aOut(
            "HIGH:THRESHOLD:UPPER", on_update=self._set_upper_high_threshold
        )
        self.lower_high_threshold = builder.aOut(
            "HIGH:THRESHOLD:LOWER", on_update=self._set_lower_high_threshold
        )
        self.upper_low_threshold = builder.aOut(
            "LOW:THRESHOLD:UPPER", on_update=self._set_upper_low_threshold
        )
        self.lower_low_threshold = builder.aOut(
            "LOW:THRESHOLD:LOWER", on_update=self._set_lower_low_threshold
        )

        self.filter_set = builder.mbbOut(
            "FILTER_SET", *FILTER_SET, on_update=self._set_filter_set
        )
        self.filter_set_rbv = builder.mbbIn("FILTER_SET_RBV", *FILTER_SET)

        self.filter_mode = builder.mbbOut(
            "FILTER_MODE", *FILTER_MODE, on_update=self._set_filter_mode
        )
        self.filter_mode_rbv = builder.mbbIn("FILTER_MODE_RBV", *FILTER_MODE)

        self.file_path = builder.stringOut("FILE:PATH", on_update=self._set_file_path)
        self.file_name = builder.stringOut("FILE:NAME", on_update=self._set_file_name)
        self.file_full_name = builder.stringIn("FILE:FULL_NAME")

        self.process_duration = builder.aIn("PROCESS:DURATION")
        self.process_period = builder.aIn("PROCESS:PERIOD")

        self.last_frame_received = builder.aIn("FRAME:RECEIVED")
        self.last_frame_processed = builder.aIn("FRAME:PROCESSED")
        self.time_since_last_frame = builder.aIn("FRAME:LAST_TIME")

        self.current_attenuation = builder.aIn("ATTENUATION_RBV")

    async def run_forever(self) -> None:

        print("Connecting to ZMQ stream...")

        # asyncio.ensure_future(self.zmq_stream.run_forever())

        req_status = b"{\"command\":\"status\"}"

        self._send_message(req_status)
        # resp = await self.zmq_stream.get_response()

        # if resp:
        #     print("Connected.")

        # await self.monitor_responses()
        await self.zmq_stream.run_forever()
        print("Done")

    async def monitor_responses(self) -> None:
        print("Monitoring responses...")
        print("F")
        while self.zmq_stream.running:
            print("D")
            resp = await self.zmq_stream.get_response()
            print("E")
            print(resp)

    def _send_message(self, message: bytes) -> bytes:
        print(f"Sending message: {message}")
        self.zmq_stream.send_message([message])

        #return await self.zmq_stream.get_response()

    def _set_mode(self, mode: int) -> None:

        # Set mode for PFC
        mode_config = json.dumps({"command":"configure","params":{"mode":mode}})
        self._send_message(codecs.encode(mode_config, "utf-8"))

        self.mode_rbv.set(mode)

    def _reset(self, _) -> None:
        pass

    def _set_timeout(self, timeout: int) -> None:

        # Set timeout for PFC

        self.timeout_rbv.set(timeout)

    def _clear_timeout(self, _) -> None:
        
        clear_timeout = json.dumps({"command":"clear_timeout"})
        self._send_message(codecs.encode(clear_timeout, "utf-8"))

    def _start_singleshot(self, _) -> None:
        
        if self.state == "WAITING" and self.mode_rbv.get() == 2:
            start_singleshot = json.dumps({"command":"singleshot"})
            self._send_message(codecs.encode(start_singleshot, "utf-8"))
        else:
            print("WARNING: Must be in SINGLESHOT mode and WAITING state.")

    def _set_upper_high_threshold(self, threshold: int) -> None:

        # Set upper high threshold for PFC
        set_upper_high = json.dumps({"command": "configure", "params": {"high2": threshold}})
        self._send_message(codecs.encode(set_upper_high, "utf-8"))

    def _set_lower_high_threshold(self, threshold: int) -> None:

        # Set lower high threshold for PFC
        set_lower_high = json.dumps({"command": "configure", "params": {"high1": threshold}})
        self._send_message(codecs.encode(set_lower_high, "utf-8"))

    def _set_upper_low_threshold(self, threshold: int) -> None:

        # Set upper low threshold for PFC
        set_upper_low = json.dumps({"command": "configure", "params": {"low2": threshold}})
        self._send_message(codecs.encode(set_upper_low, "utf-8"))

    def _set_lower_low_threshold(self, threshold: int) -> None:

        # Set lower low threshold for PFC
        set_lower_low = json.dumps({"command": "configure", "params": {"low1": threshold}})
        self._send_message(codecs.encode(set_lower_low, "utf-8"))

    def _set_filter_set(self, filter_set: int) -> None:

        # Set filter set for PFC

        self.filter_set_rbv.set(filter_set)

    def _set_filter_mode(self, filter_mode: int) -> None:

        # Set filter set for PFC

        self.filter_mode_rbv.set(filter_mode)

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
