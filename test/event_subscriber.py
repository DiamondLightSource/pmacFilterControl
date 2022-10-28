import typer
import zmq


def main(endpoint: str = "127.0.0.1:9001"):
    context = zmq.Context()

    endpoint = f"tcp://{endpoint}"
    print(f"Subscribing to {endpoint}")

    socket = context.socket(zmq.SUB)
    socket.setsockopt_string(zmq.SUBSCRIBE, "")
    socket.connect(endpoint)

    while True:
        print(socket.recv())


if __name__ == "__main__":
    typer.run(main)
