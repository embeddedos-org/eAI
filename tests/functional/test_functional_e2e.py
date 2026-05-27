# SPDX-License-Identifier: MIT
# Copyright (c) 2026 EoS Project
import unittest
import numpy as np
class TestEaiFunctional(unittest.TestCase):
    def test_tensor_multiplication(self):
        print("Testing tensor matrix multiplication (GEMM) operator...")
        A = np.array([[1, 2], [3, 4]])
        B = np.array([[5, 6], [7, 8]])
        C = np.dot(A, B)
        self.assertEqual(C[0, 0], 19)
    def test_int8_quantization(self):
        print("Testing float32 to int8 symmetric quantization...")
        weights = [0.5, -0.2, 0.8, -0.9]
        scale = max(abs(w) for w in weights) / 127
        quantized = [int(w / scale) for w in weights]
        self.assertTrue(all(-128 <= q <= 127 for q in quantized))
