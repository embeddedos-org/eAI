from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout


class EAIConan(ConanFile):
    name = "eai"
    version = "0.2.0"
    license = "MIT"
    description = "eAI — Embedded AI framework for intelligent systems"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"
    options = {
        "shared": [True, False],
        "with_accel": [True, False],
        "with_formats": [True, False],
        "with_bci": [True, False],
    }
    default_options = {
        "shared": False,
        "with_accel": True,
        "with_formats": True,
        "with_bci": True,
    }
    exports_sources = "CMakeLists.txt", "common/*", "platform/*", "min/*", \
                      "framework/*", "bci/*", "accel/*", "formats/*", \
                      "models/*", "cmake/*"

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure(variables={
            "EAI_BUILD_SHARED": self.options.shared,
            "EAI_BUILD_ACCEL": self.options.with_accel,
            "EAI_BUILD_FORMATS": self.options.with_formats,
            "EAI_BUILD_BCI": self.options.with_bci,
            "EAI_BUILD_TESTS": False,
            "EAI_BUILD_CLI": False,
        })
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["eai_common", "eai_platform"]
        if self.options.with_accel:
            self.cpp_info.libs.append("eai_accel")
        if self.options.with_formats:
            self.cpp_info.libs.append("eai_formats")
        if self.options.with_bci:
            self.cpp_info.libs.append("eai_bci")
