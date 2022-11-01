import typer
import zmq
import json


class EventSubscriber:
    """Class to receive events from pmacFilterControl publish socket"""

    def __init__(self, endpoint: str = "127.0.0.1:9001"):
        context = zmq.Context()

        endpoint = f"tcp://{endpoint}"
        print(f"Subscribing to {endpoint}")

        self.socket = context.socket(zmq.SUB)
        self.socket.setsockopt_string(zmq.SUBSCRIBE, "")
        self.poller = zmq.Poller()
        self.poller.register(self.socket, zmq.POLLIN)
        self.socket.connect(endpoint)

    def recv(self, timeout: int = 1000):
        """Blocking recv on socket"""
        if timeout:
            if not self.poller.poll(timeout):
                raise IOError("Did not receive event within timeout")

        return json.loads(self.socket.recv())

    def stop(self):
        """Close socket"""
        self.socket.close()


def main(endpoint: str = "127.0.0.1:9001"):
    sub = EventSubscriber(endpoint)

    try:
        while True:
            print(sub.recv(timeout=0))
    except KeyboardInterrupt:
        print("Shutting down")

    sub.stop()


if __name__ == "__main__":
    typer.run(main)
