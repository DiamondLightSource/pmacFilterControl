import random
import time
from typing import List

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


def main(ports: List[int] = [10001], rate: float = 1, singleshot_length: int = 0):
    context = zmq.Context()

    endpoints = [f"tcp://*:{port}" for port in ports]
    print(f"Publishing on {endpoints}")

    sockets = []
    for endpoint in endpoints:
        socket = context.socket(zmq.PUB)
        socket.bind(endpoint)
        sockets.append(socket)

    delay = 1.0 / rate
    print(f"{rate}Hz -> {delay}s per message")

    frame_number = 0
    while True:
        if singleshot_length > 0 and frame_number > singleshot_length:
            # Do not trigger thresholds
            formatter = dict(
                frame_number=frame_number, high2=0, high1=0, low2=0, low1=0
            )
        else:
            # Random values
            formatter = dict(
                frame_number=frame_number,
                high2=random.randrange(0, THRESHOLD_LEVEL),
                high1=random.randrange(0, THRESHOLD_LEVEL),
                # Make low slightly more likely to balance out precedence
                low1=random.randrange(0, THRESHOLD_LEVEL + 2),
                low2=random.randrange(0, THRESHOLD_LEVEL + 2),
            )

        message = JSON_TEMPLATE.format(**formatter)

        idx = frame_number % len(sockets)
        print(f"{endpoints[idx]} -> ")
        print(message)
        sockets[idx].send_string(message)

        time.sleep(delay)

        frame_number += 1


if __name__ == "__main__":
    typer.run(main)
