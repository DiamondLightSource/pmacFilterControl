import asyncio
import logging
from dataclasses import dataclass#, field
from typing import Iterable

import aiozmq
import zmq

# LOGGER = logging.getLogger("ZmqAdapter")


@dataclass
class ZeroMQAdapter:
    """An adapter for a ZeroMQ data stream."""

    zmq_host: str = "127.0.0.1"
    zmq_port: int = 5555
    zmq_type: int = zmq.REQ
    running: bool = False
    # _send_message_queue: asyncio.Queue = field(default_factory=asyncio.Queue)
    # _recv_message_queue: asyncio.Queue = field(default_factory=asyncio.Queue)

    async def start_stream(self) -> None:
        """Start the ZeroMQ stream."""
        # LOGGER.debug("Starting stream...")
        print("starting stream...")

        self._socket = await aiozmq.create_zmq_stream(
            self.zmq_type, connect=f"tcp://{self.zmq_host}:{self.zmq_port}"
        )
        if self.zmq_type == zmq.SUB:
                self._socket.transport.setsockopt(zmq.SUBSCRIBE, b"")
        self._socket.transport.setsockopt(zmq.LINGER, 0)
        # LOGGER.debug(f"Stream started. {self._socket}")
        print(f"Stream started. {self._socket}")

    async def close_stream(self) -> None:
        """Close the ZeroMQ stream."""
        self._socket.close()

        self.running = False

    def send_message(self, message: bytes) -> None:
        """Send a message down the ZeroMQ stream.

        Sets up an asyncio task to put the message on the message queue, before
        being processed.

        Args:
            message (str): The message to send down the ZeroMQ stream.
        """
        self._send_message_queue.put_nowait(message)

    async def _read_response(self) -> bytes:
        resp = await self._socket.read()
        return resp

    async def get_response(self) -> bytes:
        return await self._recv_message_queue.get()

    async def run_forever(self) -> None:
        """Runs the ZeroMQ adapter continuously."""

        self._send_message_queue: asyncio.Queue = asyncio.Queue()
        self._recv_message_queue: asyncio.Queue = asyncio.Queue()

        try:
            if getattr(self, "_socket", None) is None:
                await self.start_stream()
        except Exception as e:
            print("Exception when starting stream:", e)

        self.running = True

        if self.zmq_type == zmq.REQ:
            await asyncio.gather(
                *[
                    self._process_message_queue(),
                    self._process_response_queue(),
                ]
            )
        elif self.zmq_type == zmq.SUB:
            await asyncio.gather(
                *[
                    self._process_response_queue(),
                ]
            )

    def check_if_running(self):
        """Returns the running state of the adapter."""
        return self.running

    async def _process_message_queue(self) -> None:
        print("Processing message queue...")
        running = True
        while running:
            message = await self._send_message_queue.get()
            await self._process_message(message)
            running = self.check_if_running()

    async def _process_message(self, message: Iterable[bytes]) -> None:
        if message is not None:
            if not self._socket._closing:
                try:
                    self._socket.write(message)
                except zmq.error.ZMQError as e:
                    print("ZMQ Error", e)
                    await asyncio.sleep(1)
                except Exception as e:
                    print(f"Error, {e}")
                    print("Unable to write to ZMQ stream, trying again...")
                    await asyncio.sleep(1)
            else:
                print("Socket closed...")
                await asyncio.sleep(5)
        else:
            # LOGGER.debug("No message")
            print("No message")

    async def _process_response_queue(self) -> None:
        print("Processing response queue...")
        running = True
        while running:
            resp = await self._read_response()
            self._recv_message_queue.put_nowait(resp)
            running = self.check_if_running()
