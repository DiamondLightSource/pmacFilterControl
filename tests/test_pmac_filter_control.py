import json
import os
from shutil import which
import subprocess
from pathlib import Path
from time import sleep
from typing import Any, Dict, Iterator

import pytest
import zmq

from detector_sim import DetectorSim
from event_subscriber import EventSubscriber


DEFAULT_TIMEOUT_MS = 1000


HERE = Path(__file__).parent
PMAC_FILTER_CONTROL = os.getenv(
    "PMAC_FILTER_CONTROL", str(HERE / "../vscode_prefix/bin/pmacFilterControl")
)
assert which(PMAC_FILTER_CONTROL) is not None, "Bad pmacFilterControl executable"


class PMACFilterControlWrapper:
    """A class to run a pmacFilterControl application and interact with it"""

    def __init__(self):
        self.process = None
        self.control_socket = 9000

        context = zmq.Context()
        self.socket = context.socket(zmq.REQ)
        self.poller = zmq.Poller()
        self.poller.register(self.socket, zmq.POLLIN)

    def start(self):
        """Start the application, give it some time to start and test a status request"""
        cmd = [
            PMAC_FILTER_CONTROL,
            str(self.control_socket),
            "9001",
            "127.0.0.1:10009,127.0.0.1:10019",
        ]
        print(f"Running pmacFilterControl\n{cmd}")
        self.process = subprocess.Popen(cmd)
        self.socket.connect(f"tcp://127.0.0.1:{self.control_socket}")

        sleep(0.1)  # Give things a chance to connect
        self.assert_status_equal({"state": 0}, timeout=3)

    def request(self, request: dict, timeout_ms: int = DEFAULT_TIMEOUT_MS) -> dict:
        """Encode the given dict and send it as a request to the application

        Args:
            request: The request dictionary to send
            timeout_ms: Timeout in milliseconds to wait for response

        """
        request_str = json.dumps(request)
        print(f"Sending request: {request_str}")

        self.socket.send(request_str.encode())

        if not self.poller.poll(timeout=timeout_ms):
            assert False, "Did not get a response from the application"

        response = json.loads(self.socket.recv(zmq.NOBLOCK))
        assert response, "Response invalid"

        print(f"Received response: {response}")
        return response

    def request_status(self, timeout_ms: int = DEFAULT_TIMEOUT_MS) -> Dict[str, Any]:
        """Request status from the application

        Args:
            timeout_ms: Timeout in milliseconds to wait for response

        """
        response = self.request({"command": "status"}, timeout_ms)
        assert "success" in response and response["success"]
        assert "status" in response

        return response["status"]

    def assert_status_equal(self, expected_status: Dict[str, Any], timeout: int = 1):
        """Poll for status until the timeout elapses or the status matches

        Args:
            expected_status: Status items expected in status - can be a subset
            timeout: Timeout in seconds to wait for status to match

        """
        delay = 0.2
        elapsed = 0
        while elapsed < timeout:
            sleep(delay)
            elapsed += delay

            status = self.request_status(timeout * 1000)
            if self._status_equal(status, expected_status):
                return

        assert self._status_equal(
            status, expected_status
        ), f"Status not as expected after timeout elapsed:\n{status}"

    @staticmethod
    def _status_equal(status: Dict[str, Any], expected_status: Dict[str, Any]) -> bool:
        """Check if the given status dictionary matches the expected status dictionary

        Any entries in expected must match, but entries in status but not in expected
        do not matter.

        Args:
            status: Full status dictionary
            expected_status: Status items to check in `status`

        """
        for k in expected_status:
            if expected_status[k] != status.get(k, None):
                return False

        return True

    def configure(self, config: Dict[str, Any], timeout_ms: int = DEFAULT_TIMEOUT_MS):
        """Encode the given dict and send it as a request to the application

        Args:
            config: The config dictionary to send
            timeout_ms: Timeout in milliseconds to wait for response

        """
        response = self.request({"command": "configure", "params": config}, timeout_ms)
        assert "success" in response and response["success"]

        sleep(0.1)  # Allow the application state to update

    def stop(self):
        print("Stopping pmacFilterControl")
        self.process.kill()


@pytest.fixture
def detector_sim() -> Iterator[DetectorSim]:
    sim = DetectorSim([10009, 10019])
    yield sim
    sim.stop()


# pfc takes detector_sim to ensure detector_sim is instantiated first
@pytest.fixture
def pfc(detector_sim) -> Iterator[PMACFilterControlWrapper]:
    wrapper = PMACFilterControlWrapper()
    wrapper.start()
    yield wrapper
    wrapper.stop()


@pytest.fixture
def event_subscriber(pfc) -> Iterator[EventSubscriber]:
    sub = EventSubscriber("127.0.0.1:9001")
    yield sub
    sub.stop()


def test_cli_help():
    cmd = [PMAC_FILTER_CONTROL, "--help"]
    assert (
        "pmacFilterControl 9000 9001 127.0.0.1:10009,127.0.0.1:10019"
        in subprocess.check_output(cmd).decode().strip()
    )


def test_initial_status(pfc: PMACFilterControlWrapper):
    status = pfc.request_status()

    assert status["mode"] == 0  # DISABLE
    assert status["state"] == 0  # IDLE
    assert status["current_attenuation"] == 0


def test_shutdown(pfc: PMACFilterControlWrapper):
    response = pfc.request({"command": "shutdown"})
    assert response["success"]


def test_configure_positions(pfc: PMACFilterControlWrapper):
    pfc.configure(
        {
            "in_positions": {
                "filter1": 100,
                "filter2": 300,
                "filter3": 500,
                "filter4": 700,
            },
            "out_positions": {
                "filter1": 0,
                "filter2": 200,
                "filter3": 400,
                "filter4": 600,
            },
        }
    )

    status = pfc.request_status()
    assert status["in_positions"] == [100, 300, 500, 700]
    assert status["out_positions"] == [0, 200, 400, 600]


@pytest.mark.xfail
def test_configure_positions_bad(pfc: PMACFilterControlWrapper):
    in_message = {
        "command": "configure",
        "params": {
            "in_positions": {
                "filter1": 100,
                "filter2": 300,
                "filter3": 500,
                "filter4": 700,
            },
        },
    }
    out_message = {
        "command": "configure",
        "params": {
            "out_positions": {
                "filter1": 0,
                "filter2": 200,
                "filter3": 400,
                "filter4": 600,
            },
        },
    }
    pfc.socket.send(json.dumps(in_message).encode())
    pfc.socket.send(json.dumps(out_message).encode())


def test_configure_change_position(pfc: PMACFilterControlWrapper):
    pfc.configure({"in_positions": {"filter1": 100}})
    pfc.configure({"in_positions": {"filter1": 200}})

    status = pfc.request_status()

    assert status["in_positions"] == [200, 0, 0, 0]


def test_configure_pixel_count_thresholds(pfc: PMACFilterControlWrapper):
    pfc.configure(
        {
            "pixel_count_thresholds": {
                "low2": 10,
                "low1": 50,
                "high1": 1000,
                "high2": 5000,
                "high3": 50000,
            }
        }
    )

    status = pfc.request_status()
    assert status["pixel_count_thresholds"] == {
        "low2": 10,
        "low1": 50,
        "high1": 1000,
        "high2": 5000,
        "high3": 50000,
    }


def test_configure_mode(pfc: PMACFilterControlWrapper):
    # Changing to CONTINUOUS changes state to WAITING
    pfc.configure({"mode": 1})
    status = pfc.request_status()
    assert status["mode"] == 1
    assert status["state"] == 1

    # Changing to DISABLE changes state to IDLE
    pfc.configure({"mode": 0})
    status = pfc.request_status()
    assert status["mode"] == 0
    assert status["state"] == 0

    # Changing to SINGLESHOT changes state to WAITING
    pfc.configure({"mode": 2})
    status = pfc.request_status()
    assert status["mode"] == 2
    assert status["state"] == 1


def test_continuous_timeout(detector_sim: DetectorSim, pfc: PMACFilterControlWrapper):
    pfc.configure({"mode": 1})
    pfc.assert_status_equal({"state": 1, "current_attenuation": 15})

    # Force trigger low2 threshold
    detector_sim.send_frame({"high2": 0, "high1": 0, "low2": 0})

    # Process frame 0, reduce attenuation by 2 and change to ACTIVE
    pfc.assert_status_equal(
        {
            "state": 2,
            "last_processed_frame": 0,
            "last_received_frame": 0,
            "current_attenuation": 13,
        }
    )
    # Then the application should timeout after 3 seconds and set max attenuation
    pfc.assert_status_equal({"state": 3, "current_attenuation": 15}, timeout=4)


def test_event(
    detector_sim: DetectorSim,
    pfc: PMACFilterControlWrapper,
    event_subscriber: EventSubscriber,
):
    pfc.configure({"mode": 1})
    pfc.assert_status_equal({"state": 1, "current_attenuation": 15})

    detector_sim.send_frame()

    # Check an event was published
    event = event_subscriber.recv()
    assert event["frame_number"] == 0
