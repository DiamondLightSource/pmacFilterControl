import random
from time import sleep
from typing import Dict, List

import typer
import zmq


JSON_TEMPLATE = """
{{
    "frame_number": {frame_number},
    "parameters": {{
        "high2": {high2},
        "high1": {high1},
        "low1": {low1},
        "low2": {low2}
    }}
}}
"""

THRESHOLD_LEVEL = 4


class DetectorSim:
    def __init__(self, ports: List[int]) -> None:
        context = zmq.Context()

        self.endpoints = [f"tcp://*:{port}" for port in ports]
        print(f"Publishing on {self.endpoints}")

        self.sockets = []
        for endpoint in self.endpoints:
            socket = context.socket(zmq.PUB)
            socket.bind(endpoint)
            self.sockets.append(socket)

        self.frame_number = 0

    def run(self, rate: float, frame_count: int, singleshot_length: int):
        """Send frames according to the given parameters

        Send random frames until `frame_count` is reached. If `singleshot_length` is
        reached, send blank frames until `frame_count` reached.

        Args:
            rate: Rate in Hz to send frames at
            frame_count: Total number of frames to send before stopping. 0 -> unlimited.
            singleshot_length: Number of random frames to send before sending blank
                frames. 0 -> only send random frames.

        """
        delay = 1.0 / min(rate, 100)
        print(f"{rate}Hz -> {delay}s per message")

        send_fn = self.send_frame
        while True:
            if frame_count > 0 and self.frame_number + 1 > frame_count:
                break

            if singleshot_length > 0 and self.frame_number + 1 > singleshot_length:
                print("Singleshot length reached")
                send_fn = self.send_blank

            send_fn()
            sleep(delay)

        print("Frame count reached")
        self.stop()

    def send_frame(self, user_data: Dict[str, int] = {}):
        """Send a random frame, polpulated with the given data if given

        Args:
            user_data: Values to explicitly set in frame. Used to force certain
                processing in the application.

        """
        # Random values
        data = dict(
            frame_number=self.frame_number,
            high2=random.randrange(0, THRESHOLD_LEVEL),
            high1=random.randrange(0, THRESHOLD_LEVEL),
            low1=random.randrange(0, THRESHOLD_LEVEL),
            low2=random.randrange(0, THRESHOLD_LEVEL),
        )
        data.update(user_data)
        self._send_frame(data)

    def send_blank(self):
        """Send a "blank" frame - i.e. a frame that causes no processing to take place"""
        # Do not trigger thresholds
        data = dict(
            frame_number=self.frame_number, high2=0, high1=0, low2=10000, low1=10000
        )
        self._send_frame(data)

    def _send_frame(self, data):
        """Encode data as a message and publish on a socket based on frame number

        Args:
            data: Dictionary of data to publish

        """
        message = JSON_TEMPLATE.format(**data)

        idx = self.frame_number % len(self.sockets)
        print(f"{self.endpoints[idx]} -> ")
        print(message)
        self.sockets[idx].send_string(message)

        self.frame_number += 1

    def stop(self):
        """Close sockets"""
        for socket in self.sockets:
            socket.close()


def main(
    ports: List[int] = [10009],
    rate: float = 1,
    frame_count: int = 0,
    singleshot_length: int = 0,
):
    DetectorSim(ports).run(rate, frame_count, singleshot_length)


if __name__ == "__main__":
    typer.run(main)
