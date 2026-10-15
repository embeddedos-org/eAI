"""
tests/unit/test_unit_conanfile.py — Unit tests for the Conan recipe
SPDX-License-Identifier: MIT  Copyright (c) 2026 EmbeddedOS Foundation

conanfile.py was the suite's one uncovered module (0% while the
total sat at 82.71% — see #48): the Python Tests job does not
install Conan, so the recipe could not even be imported there.
These tests stub the small slice of the `conan` API the recipe
uses (ConanFile, CMake, cmake_layout) in sys.modules, then load
the recipe by path and exercise it for real — attributes, the
layout/build/package calls, and every package_info branch.
"""
import importlib.util
import sys
import types
import unittest
from pathlib import Path
from types import SimpleNamespace


class _FakeCMake:
    """Records what the recipe asks CMake to do."""

    instances = []

    def __init__(self, conanfile):
        self.conanfile = conanfile
        self.configure_variables = None
        self.build_calls = 0
        self.install_calls = 0
        _FakeCMake.instances.append(self)

    def configure(self, variables=None):
        self.configure_variables = variables

    def build(self):
        self.build_calls += 1

    def install(self):
        self.install_calls += 1


def _install_conan_stubs():
    layout_calls = []

    conan = types.ModuleType("conan")

    class ConanFile:
        pass

    conan.ConanFile = ConanFile

    tools = types.ModuleType("conan.tools")
    cmake_mod = types.ModuleType("conan.tools.cmake")
    cmake_mod.CMake = _FakeCMake

    def cmake_layout(conanfile):
        layout_calls.append(conanfile)

    cmake_mod.cmake_layout = cmake_layout
    tools.cmake = cmake_mod
    conan.tools = tools

    sys.modules["conan"] = conan
    sys.modules["conan.tools"] = tools
    sys.modules["conan.tools.cmake"] = cmake_mod
    return layout_calls


_LAYOUT_CALLS = _install_conan_stubs()

_RECIPE_PATH = Path(__file__).resolve().parents[2] / "conanfile.py"
_spec = importlib.util.spec_from_file_location("eai_conanfile", _RECIPE_PATH)
_module = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_module)
EAIConan = _module.EAIConan


def _recipe(**options):
    recipe = EAIConan()
    merged = {"shared": False, "with_accel": True,
              "with_formats": True, "with_bci": True}
    merged.update(options)
    recipe.options = SimpleNamespace(**merged)
    recipe.cpp_info = SimpleNamespace(libs=[])
    return recipe


class TestRecipeAttributes(unittest.TestCase):
    def test_identity(self):
        self.assertEqual(EAIConan.name, "eai")
        self.assertEqual(EAIConan.version, "0.2.0")
        self.assertEqual(EAIConan.license, "MIT")
        self.assertIn("Embedded AI", EAIConan.description)

    def test_settings_and_generators(self):
        self.assertEqual(EAIConan.settings,
                         ("os", "compiler", "build_type", "arch"))
        self.assertEqual(EAIConan.generators, ("CMakeToolchain", "CMakeDeps"))

    def test_options_and_defaults(self):
        self.assertEqual(set(EAIConan.options),
                         {"shared", "with_accel", "with_formats", "with_bci"})
        for allowed in EAIConan.options.values():
            self.assertEqual(allowed, [True, False])
        self.assertEqual(EAIConan.default_options["shared"], False)
        for key in ("with_accel", "with_formats", "with_bci"):
            self.assertEqual(EAIConan.default_options[key], True)

    def test_exports_sources_cover_the_tree(self):
        self.assertIn("CMakeLists.txt", EAIConan.exports_sources)
        for area in ("common/*", "accel/*", "formats/*", "bci/*"):
            self.assertIn(area, EAIConan.exports_sources)


class TestRecipeMethods(unittest.TestCase):
    def setUp(self):
        _FakeCMake.instances.clear()
        _LAYOUT_CALLS.clear()

    def test_layout_uses_cmake_layout(self):
        recipe = _recipe()
        recipe.layout()
        self.assertEqual(_LAYOUT_CALLS, [recipe])

    def test_build_configures_options_and_builds(self):
        recipe = _recipe(shared=True, with_accel=False)
        recipe.build()
        (cmake,) = _FakeCMake.instances
        self.assertIs(cmake.conanfile, recipe)
        self.assertEqual(cmake.configure_variables, {
            "EAI_BUILD_SHARED": True,
            "EAI_BUILD_ACCEL": False,
            "EAI_BUILD_FORMATS": True,
            "EAI_BUILD_BCI": True,
            "EAI_BUILD_TESTS": False,
            "EAI_BUILD_CLI": False,
        })
        self.assertEqual(cmake.build_calls, 1)

    def test_package_installs(self):
        recipe = _recipe()
        recipe.package()
        (cmake,) = _FakeCMake.instances
        self.assertEqual(cmake.install_calls, 1)

    def test_package_info_all_components(self):
        recipe = _recipe()
        recipe.package_info()
        self.assertEqual(recipe.cpp_info.libs,
                         ["eai_common", "eai_platform", "eai_accel",
                          "eai_formats", "eai_bci"])

    def test_package_info_core_only(self):
        recipe = _recipe(with_accel=False, with_formats=False, with_bci=False)
        recipe.package_info()
        self.assertEqual(recipe.cpp_info.libs, ["eai_common", "eai_platform"])

    def test_package_info_mixed_components(self):
        recipe = _recipe(with_accel=True, with_formats=False, with_bci=True)
        recipe.package_info()
        self.assertEqual(recipe.cpp_info.libs,
                         ["eai_common", "eai_platform", "eai_accel", "eai_bci"])


if __name__ == "__main__":
    unittest.main()
