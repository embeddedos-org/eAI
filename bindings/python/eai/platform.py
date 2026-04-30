"""High-level Platform API for eAI."""

from .core import EAIPlatform, EAILib


class Platform:
    """Pythonic interface for eAI platform detection."""

    def __init__(self, lib_path=None):
        self._lib = EAILib(lib_path) if lib_path else EAILib()
        self._plat = EAIPlatform(self._lib)
        self._plat.detect()

    @staticmethod
    def detect(lib_path=None):
        """Auto-detect the current platform and return a Platform instance."""
        return Platform(lib_path)

    @property
    def info(self):
        """Get device info string."""
        return self._plat.get_info()

    @property
    def memory(self):
        """Get (total_bytes, available_bytes) tuple."""
        return self._plat.get_memory()

    @property
    def total_memory_mb(self):
        """Get total memory in MB."""
        total, _ = self.memory
        return total // (1024 * 1024)

    def __repr__(self):
        return f"Platform({self.info})"

    def __del__(self):
        try:
            self._plat.shutdown()
        except Exception:
            pass
