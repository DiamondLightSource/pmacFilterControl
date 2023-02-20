import pytest

from pmacfiltercontrol.zmqadapter import ZeroMQAdapter


@pytest.fixture
def zmq_adapter() -> ZeroMQAdapter:
    return ZeroMQAdapter()


def test_zmq_adapter_constructor():
    ZeroMQAdapter()
