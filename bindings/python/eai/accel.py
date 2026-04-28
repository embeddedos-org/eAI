"""Accelerator backend Python API for eAI."""

from .core import EAILib


class Accelerator:
    """Query available accelerator backends."""

    def __init__(self, lib_path=None):
        self._lib = EAILib(lib_path) if lib_path else EAILib()

    @property
    def count(self):
        """Number of registered accelerator backends."""
        return self._lib._lib.eai_api_accel_count()

    def list_backends(self):
        """List all registered backend names."""
        import ctypes
        names = []
        for i in range(self.count):
            buf = ctypes.create_string_buffer(64)
            self._lib._lib.eai_api_accel_get_name(i, buf, 64)
            names.append(buf.value.decode())
        return names

    def __repr__(self):
        return f"Accelerator(backends={self.list_backends()})"
