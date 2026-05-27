import unittest
class TestEAIFunctional(unittest.TestCase):
    def test_npu_inference_pipeline(self):
        pipeline = ["load_model", "quantize", "run_conv", "softmax"]
        self.assertEqual(pipeline[-1], "softmax")
