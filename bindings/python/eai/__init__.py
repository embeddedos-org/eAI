# eAI Python Bindings
from .core import EAIPlatform, EAIRuntime, eai_version, eai_status_str
from .platform import Platform
from .accel import Accelerator

__version__ = "0.2.0"
__all__ = [
    "Platform",
    "EAIPlatform",
    "EAIRuntime",
    "Accelerator",
    "eai_version",
    "eai_status_str",
]
