"""
tests/unit/test_unit_bci_pipeline.py — Unit tests for eai_bci.BciPipeline
SPDX-License-Identifier: MIT  Copyright (c) 2026 EmbeddedOS Foundation

BciPipeline (bindings/python/eai_bci/pipeline.py) wraps BciLib and
BciHandle, which load the compiled eai_bci shared library through
ctypes. The Python Tests job builds no C code, so these tests put a
stub eai_bci.core in sys.modules that records what the pipeline asks
for, then load pipeline.py by path, the same way the Conan recipe test
stubs `conan`.
"""
import importlib.util
import sys
import types
import unittest
import unittest.mock
from pathlib import Path

PKG_DIR = Path(__file__).resolve().parents[2] / "bindings" / "python" / "eai_bci"


class _FakeLib:
    def __init__(self, lib_path=None):
        self.lib_path = lib_path


class _FakeHandle:
    instances = []

    def __init__(self, lib, device="simulator", decoder="threshold", output="log"):
        self.lib = lib
        self.device = device
        self.decoder = decoder
        self.output = output
        self.calls = []
        self.channel_count = 8
        self.sample_rate = 250
        self.samples_processed = 42
        _FakeHandle.instances.append(self)

    def start(self):
        self.calls.append("start")

    def stop(self):
        self.calls.append("stop")

    def destroy(self):
        self.calls.append("destroy")

    # BciHandle.poll() returns a bool, so the fake returns one too: with an
    # int 0 an assertEqual(..., 0) would also accept False and the test could
    # not tell a successful poll from an unsuccessful one.
    def poll(self):
        self.calls.append("poll")
        return True

    def get_intent(self):
        return {"label": "rest", "confidence": 0.5}

    def get_signal(self, max_samples=256):
        return [max_samples]


def _load_pipeline():
    package = types.ModuleType("eai_bci")
    package.__path__ = [str(PKG_DIR)]
    core = types.ModuleType("eai_bci.core")
    core.BciLib = _FakeLib
    core.BciHandle = _FakeHandle
    with unittest.mock.patch.dict(sys.modules, {"eai_bci": package, "eai_bci.core": core}):
        spec = importlib.util.spec_from_file_location(
            "eai_bci.pipeline", PKG_DIR / "pipeline.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
    return module


class TestBciPipeline(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.BciPipeline = _load_pipeline().BciPipeline

    def setUp(self):
        _FakeHandle.instances.clear()

    def _handle(self):
        self.assertEqual(len(_FakeHandle.instances), 1)
        return _FakeHandle.instances[0]

    def test_default_is_the_simulator(self):
        self.BciPipeline()
        self.assertEqual(self._handle().device, "simulator")

    def test_named_device_is_used(self):
        self.BciPipeline(device="openbci")
        self.assertEqual(self._handle().device, "openbci")

    def test_simulator_flag_overrides_the_device(self):
        self.BciPipeline(device="openbci", simulator=True)
        self.assertEqual(self._handle().device, "simulator")

    def test_decoder_output_and_lib_path_are_passed_on(self):
        self.BciPipeline(decoder="snn", output="gpio", lib_path="/opt/libeai_bci.so")
        handle = self._handle()
        self.assertEqual((handle.decoder, handle.output), ("snn", "gpio"))
        self.assertEqual(handle.lib.lib_path, "/opt/libeai_bci.so")

    def test_context_manager_starts_stops_and_destroys(self):
        with self.BciPipeline() as bci:
            self.assertTrue(bci.poll())
            self.assertEqual(bci.get_intent()["label"], "rest")
            self.assertEqual(bci.get_signal(16), [16])
            self.assertEqual(
                (bci.channel_count, bci.sample_rate, bci.samples_processed),
                (8, 250, 42))
        self.assertEqual(self._handle().calls, ["start", "poll", "stop", "destroy"])


if __name__ == "__main__":  # pragma: no cover
    unittest.main()
