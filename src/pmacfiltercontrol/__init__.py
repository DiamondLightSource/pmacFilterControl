from importlib_metadata import version

__version__ = version("pmacfiltercontrol")
del version

__all__ = ["__version__"]
