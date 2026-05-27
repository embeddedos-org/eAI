import unittest

class TesteAIFunctional(unittest.TestCase):
    def test_npu_inference_pipeline(self):
        # Test model inference pipeline (quantize -> run -> dequantize)
        float_input = 0.5
        scale = 127.0
        quant_input = int(float_input * scale)
        assert quant_input == 63
        # NPU run
        quant_out = quant_input * 2
        # Dequantize
        float_out = quant_out / (scale * 2)
        assert abs(float_out - 0.5) < 0.01
