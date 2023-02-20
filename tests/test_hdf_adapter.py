import pytest

from pmacfiltercontrol.hdfadapter import HDFAdapter


@pytest.fixture
def hdf_adapter() -> HDFAdapter:
    return HDFAdapter()


def test_hdf_adapter_constructor():
    HDFAdapter()
