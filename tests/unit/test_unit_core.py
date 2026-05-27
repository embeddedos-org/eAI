import unittest
import numpy as np
class TestEAIUnit(unittest.TestCase):
    def test_tensor_gemm(self):
        A = np.array([[1, 2], [3, 4]], dtype=np.int32)
        B = np.array([[5, 6], [7, 8]], dtype=np.int32)
        C = np.dot(A, B)
        self.assertTrue(np.array_equal(C, [[19, 22], [43, 50]]))
    def test_int8_quantization(self):
        weights = np.array([-1.5, 0.0, 2.3], dtype=np.float32)
        scale = 127.0 / max(abs(weights))
        quantized = np.round(weights * scale).astype(np.int8)
        self.assertEqual(quantized[2], 127)
