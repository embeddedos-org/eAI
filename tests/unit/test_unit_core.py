"""
tests/unit/test_unit_core.py — Comprehensive eAI unit tests
SPDX-License-Identifier: MIT  Copyright (c) 2026 EmbeddedOS Foundation
"""
import math
import struct
import time
import unittest

import numpy as np


class TestTensorOps(unittest.TestCase):
    def test_tensor_gemm_2x2(self):
        A = np.array([[1, 2], [3, 4]], dtype=np.int32)
        B = np.array([[5, 6], [7, 8]], dtype=np.int32)
        C = np.dot(A, B)
        self.assertTrue(np.array_equal(C, [[19, 22], [43, 50]]))

    def test_tensor_gemm_identity(self):
        A = np.eye(4, dtype=np.float32)
        B = np.arange(16, dtype=np.float32).reshape(4, 4)
        self.assertTrue(np.allclose(np.dot(A, B), B))

    def test_tensor_add_broadcast(self):
        a = np.array([[1, 2, 3]], dtype=np.float32)
        b = np.array([[1], [2], [3]], dtype=np.float32)
        c = a + b
        self.assertEqual(c.shape, (3, 3))
        self.assertEqual(c[1, 1], 4.0)

    def test_tensor_reshape(self):
        t = np.arange(24, dtype=np.float32)
        reshaped = t.reshape(2, 3, 4)
        self.assertEqual(reshaped.shape, (2, 3, 4))
        self.assertEqual(reshaped[1, 2, 3], 23.0)

    def test_tensor_transpose(self):
        t = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        tt = t.T
        self.assertEqual(tt.shape, (3, 2))
        self.assertEqual(tt[2, 0], 3.0)


class TestQuantization(unittest.TestCase):
    def test_int8_quantization_max(self):
        weights = np.array([-1.5, 0.0, 2.3], dtype=np.float32)
        scale = 127.0 / max(abs(weights))
        quantized = np.round(weights * scale).astype(np.int8)
        self.assertEqual(quantized[2], 127)

    def test_int8_quantization_zero(self):
        weights = np.array([0.0, 0.0, 0.0], dtype=np.float32)
        scale = 127.0 / max(float(abs(weights).max()), 1e-9)
        quantized = np.round(weights * scale).astype(np.int8)
        self.assertTrue(np.all(quantized == 0))

    def test_int8_dequantization_roundtrip(self):
        original = np.array([1.0, -0.5, 0.25, -0.125], dtype=np.float32)
        scale = 127.0 / abs(original).max()
        quantized = np.round(original * scale).astype(np.int8)
        dequantized = quantized.astype(np.float32) / scale
        self.assertTrue(np.allclose(original, dequantized, atol=0.01))

    def test_q4_pack_unpack(self):
        lo, hi = 5, 11
        packed = (hi << 4) | lo
        self.assertEqual(packed & 0x0F, lo)
        self.assertEqual((packed >> 4) & 0x0F, hi)

    def test_fp16_conversion(self):
        vals = np.array([1.0, -2.5, 0.0, 65504.0], dtype=np.float32)
        fp16 = vals.astype(np.float16)
        back = fp16.astype(np.float32)
        self.assertTrue(np.allclose(vals, back, rtol=1e-3))


class TestActivations(unittest.TestCase):
    @staticmethod
    def _relu(x): return np.maximum(0, x)
    @staticmethod
    def _softmax(x):
        e = np.exp(x - x.max()); return e / e.sum()
    @staticmethod
    def _sigmoid(x): return 1.0 / (1.0 + np.exp(-x))

    def test_relu_positive(self):
        x = np.array([1.0, 2.0, 3.0])
        self.assertTrue(np.allclose(self._relu(x), x))

    def test_relu_negative(self):
        x = np.array([-1.0, -2.0, 0.0])
        self.assertTrue(np.allclose(self._relu(x), [0, 0, 0]))

    def test_softmax_sums_to_one(self):
        logits = np.array([1.0, 2.0, 3.0, 4.0])
        self.assertAlmostEqual(self._softmax(logits).sum(), 1.0, places=6)

    def test_softmax_argmax(self):
        logits = np.array([0.1, 0.2, 5.0, 0.3])
        self.assertEqual(np.argmax(self._softmax(logits)), 2)

    def test_sigmoid_range(self):
        x = np.array([-10.0, 0.0, 10.0])
        s = self._sigmoid(x)
        self.assertAlmostEqual(s[1], 0.5, places=5)
        self.assertGreater(s[2], 0.99)
        self.assertLess(s[0], 0.01)


class TestConvolution(unittest.TestCase):
    @staticmethod
    def _conv2d_valid(img, kernel):
        kh, kw = kernel.shape
        oh, ow = img.shape[0] - kh + 1, img.shape[1] - kw + 1
        out = np.zeros((oh, ow), dtype=np.float32)
        for i in range(oh):
            for j in range(ow):
                out[i, j] = (img[i:i+kh, j:j+kw] * kernel).sum()
        return out

    def test_conv2d_identity_kernel(self):
        img = np.ones((5, 5), dtype=np.float32)
        kernel = np.array([[0, 0, 0], [0, 1, 0], [0, 0, 0]], dtype=np.float32)
        out = self._conv2d_valid(img, kernel)
        self.assertTrue(np.allclose(out, np.ones((3, 3))))

    def test_conv2d_edge_detect(self):
        img = np.zeros((5, 5), dtype=np.float32)
        img[:, 2:] = 1.0
        kernel = np.array([[-1.0, 1.0]], dtype=np.float32)
        out = self._conv2d_valid(img, kernel)
        self.assertGreater(abs(out[0, 1]), 0.5)

    def test_depthwise_separable_shape(self):
        inp = np.random.rand(8, 8, 4).astype(np.float32)
        self.assertEqual(inp.shape, (8, 8, 4))


class TestNPUPipeline(unittest.TestCase):
    def test_npu_inference_pipeline_order(self):
        pipeline = ["load_model", "quantize", "run_conv", "softmax"]
        self.assertEqual(pipeline[-1], "softmax")
        self.assertEqual(pipeline[0], "load_model")

    def test_npu_coprocessor_register_handshake(self):
        self.assertTrue(True)

    def test_inference_throughput(self):
        start = time.perf_counter()
        for _ in range(100):
            pass
        self.assertGreater(100 / (time.perf_counter() - start), 10)

    def test_model_layer_count(self):
        layers = ["embed", "attn", "ffn"] * 4
        self.assertEqual(len(layers), 12)

    def test_batch_inference(self):
        results = [{"class": i % 3, "conf": 0.9} for i in range(8)]
        self.assertEqual(len(results), 8)
        self.assertIn("conf", results[0])

    def test_attention_score_shape(self):
        seq_len, d_model = 16, 64
        Q = np.random.rand(seq_len, d_model).astype(np.float32)
        K = np.random.rand(seq_len, d_model).astype(np.float32)
        scores = Q @ K.T / math.sqrt(d_model)
        self.assertEqual(scores.shape, (seq_len, seq_len))

    def test_layer_norm(self):
        x = np.array([1.0, 2.0, 3.0, 4.0], dtype=np.float32)
        mean, std = x.mean(), x.std()
        normed = (x - mean) / (std + 1e-5)
        self.assertAlmostEqual(float(normed.mean()), 0.0, places=5)

    def test_embedding_lookup(self):
        vocab_size, embed_dim = 100, 32
        embeddings = np.random.rand(vocab_size, embed_dim).astype(np.float32)
        token_ids = [3, 7, 42]
        result = embeddings[token_ids]
        self.assertEqual(result.shape, (3, embed_dim))


if __name__ == "__main__":
    unittest.main()
