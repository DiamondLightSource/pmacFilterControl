import pytest
from mock import Mock
from mock.mock import create_autospec
from softioc import builder

from pmacfiltercontrol.hdfadapter import HDFAdapter
from pmacfiltercontrol.pmacFilterControlWrapper import Wrapper
from pmacfiltercontrol.zmqadapter import ZeroMQAdapter


@pytest.fixture
def mock_zmq_adapter() -> Mock:
    return create_autospec(ZeroMQAdapter)


@pytest.fixture
def mock_hdf_adapter() -> Mock:
    return create_autospec(HDFAdapter)


@pytest.fixture
def pfc_wrapper() -> Wrapper:
    return Wrapper()


def test_pfc_wrapper_constructor(builder_=builder):
    Wrapper(
        "127.0.0.1",
        9000,
        9001,
        builder=builder_,
        device_name="pytest_pfcw",
        filter_set_total=2,
        filters_per_set=4,
        detector="BLXXI-TEST-EXCBR-01",
        motors="BLXXI-TEST-FILT-01",
        autosave_file_path="/tmp/tmp.txt",
        hdf_file_path="/tmp/tmp.h5",
    )
