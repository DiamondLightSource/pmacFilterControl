import json
import os
from shutil import which
import subprocess
from pathlib import Path
from typing import Iterator, List

import pytest
import zmq


HERE = Path(__file__).parent
PMAC_FILTER_CONTROL = os.getenv(
    "PMAC_FILTER_CONTROL", str(HERE / "../vscode_prefix/bin/pmacFilterControl")
)
assert which(PMAC_FILTER_CONTROL) is not None, "Bad pmacFilterControl executable"


class PMACFilterControlWrapper:
    def __init__(self):
        self.process = None
        self.control_socket = 9000

        context = zmq.Context()
        self.socket = context.socket(zmq.REQ)
        self.poller = zmq.Poller()
        self.poller.register(self.socket, zmq.POLLIN)

    def start(self):
        cmd = [
            PMAC_FILTER_CONTROL,
            str(self.control_socket),
            "9001",
            "127.0.0.1:10009,127.0.0.1:10019",
        ]
        print(f"Running pmacFilterControl\n{cmd}")
        self.process = subprocess.Popen(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )

        self.socket.connect(f"tcp://127.0.0.1:{self.control_socket}")

    def request(self, message: dict) -> dict:
        self.socket.send(json.dumps(message).encode())

        if not self.poller.poll(timeout=1000):
            assert False, "Did not get a response from the application"

        response = json.loads(self.socket.recv(zmq.NOBLOCK))
        assert response, "Response invalid"
        return response

    def stdout(self) -> List[bytes]:
        output = self.process.stdout.readlines()
        print(output)
        return output

    def stop(self):
        print("Stopping pmacFilterControl")
        self.process.kill()


@pytest.fixture()
def pfc() -> Iterator[PMACFilterControlWrapper]:
    pfc = PMACFilterControlWrapper()
    pfc.start()
    yield pfc
    pfc.stop()


def test_cli_help():
    cmd = [PMAC_FILTER_CONTROL, "--help"]
    assert (
        "pmacFilterControl 9000 9001 127.0.0.1:10009,127.0.0.1:10019"
        in subprocess.check_output(cmd).decode().strip()
    )


def test_initial_status(pfc: PMACFilterControlWrapper):
    response = pfc.request({"command": "status"})

    assert "status" in response


def test_shutdown(pfc: PMACFilterControlWrapper):
    response = pfc.request({"command": "shutdown"})
    assert response["success"]
