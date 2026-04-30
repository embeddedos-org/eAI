"""Core ctypes FFI wrapper for libeai C API."""

import ctypes
import os
import platform as _platform
from pathlib import Path


def _find_library():
    """Locate the eAI shared library."""
    system = _platform.system()
    if system == "Windows":
        name = "eai.dll"
    elif system == "Darwin":
        name = "libeai.dylib"
    else:
        name = "libeai.so"

    search_paths = [
        Path(__file__).parent.parent.parent.parent / "build",
        Path(__file__).parent.parent.parent.parent / "build" / "common",
        Path(os.environ.get("EAI_LIB_PATH", "")),
    ]

    for p in search_paths:
        lib_path = p / name
        if lib_path.exists():
            return str(lib_path)

    return name


class EAILib:
    """Wrapper around the eAI C shared library."""

    def __init__(self, lib_path=None):
        path = lib_path or _find_library()
        self._lib = ctypes.CDLL(path)
        self._setup_signatures()

    def _setup_signatures(self):
        lib = self._lib

        # Version
        lib.eai_version.argtypes = []
        lib.eai_version.restype = ctypes.c_char_p

        # Status
        lib.eai_api_status_str.argtypes = [ctypes.c_int]
        lib.eai_api_status_str.restype = ctypes.c_char_p


def eai_version(lib_path=None):
    """Get eAI version string."""
    lib = EAILib(lib_path)
    return lib._lib.eai_version().decode()


def eai_status_str(status_code):
    """Convert status code to string."""
    lib = EAILib()
    return lib._lib.eai_api_status_str(status_code).decode()


class EAIPlatform:
    """Platform detection and info."""

    def __init__(self, lib=None):
        self._lib = lib or EAILib()
        # Platform struct is stack-allocated in C — use a byte buffer
        self._plat = ctypes.create_string_buffer(256)  # generous for eai_platform_t

    def detect(self):
        """Auto-detect the current platform."""
        rc = self._lib._lib.eai_api_platform_detect(self._plat)
        if rc != 0:
            raise RuntimeError(f"Platform detection failed: {rc}")
        return self

    def get_info(self):
        """Get device info string."""
        buf = ctypes.create_string_buffer(256)
        self._lib._lib.eai_api_platform_get_info(self._plat, buf, 256)
        return buf.value.decode()

    def get_memory(self):
        """Get total and available memory in bytes."""
        total = ctypes.c_uint64()
        avail = ctypes.c_uint64()
        self._lib._lib.eai_api_platform_get_memory(
            self._plat, ctypes.byref(total), ctypes.byref(avail)
        )
        return total.value, avail.value

    def shutdown(self):
        """Shutdown the platform."""
        self._lib._lib.eai_api_platform_shutdown(self._plat)

    def __del__(self):
        try:
            self.shutdown()
        except Exception:
            pass


class EAIRuntime:
    """Runtime for model loading and inference."""

    def __init__(self, lib=None):
        self._lib = lib or EAILib()
        self._rt = ctypes.c_void_p()

    def create(self):
        rc = self._lib._lib.eai_api_runtime_create(ctypes.byref(self._rt))
        if rc != 0:
            raise RuntimeError(f"Runtime creation failed: {rc}")
        return self

    def load_model(self, path):
        rc = self._lib._lib.eai_api_runtime_load_model(self._rt, path.encode())
        if rc != 0:
            raise RuntimeError(f"Model loading failed: {rc}")

    def destroy(self):
        if self._rt:
            self._lib._lib.eai_api_runtime_destroy(self._rt)
            self._rt = None

    def __del__(self):
        try:
            self.destroy()
        except Exception:
            pass
