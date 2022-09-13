import random
import time

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

THRESHOLD_LEVEL = 3


def main(port: int = 10000, rate: float = 1):
    endpoint = f"tcp://*:{port}"
    print(f"Publishing on {endpoint}")
    delay = 1.0 / rate
    print(f"{rate}Hz -> {delay}s per message")

    context = zmq.Context()
    socket = context.socket(zmq.PUB)
    socket.bind(endpoint)

    frame_number = 1
    while True:
        formatter = dict(
            frame_number=frame_number,
            high2=random.randrange(0, THRESHOLD_LEVEL),
            high1=random.randrange(0, THRESHOLD_LEVEL),
            # Make low slightly more likely to balance out precedence
            low1=random.randrange(0, THRESHOLD_LEVEL + 2),
            low2=random.randrange(0, THRESHOLD_LEVEL + 2),
        )

        message = JSON_TEMPLATE.format(**formatter)
        print(message)
        socket.send_string(message)

        time.sleep(delay)

        frame_number += 1


if __name__ == "__main__":
    typer.run(main)
