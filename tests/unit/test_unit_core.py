import unittest

class TesteAIUnit(unittest.TestCase):
    def test_tensor_gemm_quantization(self):
        # Simulate INT8 matrix multiplication (GEMM) for NPU
        import numpy as np
        # Use int16/int32 for accumulation to avoid 8-bit overflow
        weights = np.array([[127, -127], [-64, 64]], dtype=np.int8)
        inputs = np.array([[127, 127]], dtype=np.int8)
        res = np.dot(inputs.astype(np.int32), weights.astype(np.int32))
        assert res[0][0] == 8001, f"Quantized GEMM expected 8001, got {res[0][0]}"
