#!/usr/bin/env python3

import os
import sys
import subprocess
from pathlib import Path

from pybind11.setup_helpers import Pybind11Extension, build_ext
from pybind11 import get_cmake_dir
import pybind11

from setuptools import setup, Extension, find_packages

# Define extension module
ext_modules = [
    Pybind11Extension(
        "ucra",
        [
            "src/main.cpp",
            "src/engine_wrapper.cpp",
            "src/curves_wrapper.cpp",
            "src/manifest_wrapper.cpp",
            "src/types_wrapper.cpp",
        ],
        include_dirs=[
            # Path to pybind11 headers
            pybind11.get_include(),
            # Path to UCRA headers
            str(Path(__file__).parent.parent.parent / "include"),
        ],
        libraries=["ucra_impl"],
        library_dirs=[
            # Path to UCRA shared library
            str(Path(__file__).parent.parent.parent / "build"),
        ],
        language='c++'
    ),
]

# Ensure runtime linker can find libucra_impl at runtime on Linux
if sys.platform.startswith("linux"):
    for ext in ext_modules:
        # Add rpath to $ORIGIN (so placing lib next to extension works)
        # and to top-level build dir
        ext.runtime_library_dirs = [
            "$ORIGIN",
            str(Path(__file__).parent.parent.parent / "build"),
        ]

setup(
    name="ucra-python",
    version="1.0.0",
    author="UCRA Team",
    author_email="team@ucra.dev",
    url="https://github.com/ucra/ucra",
    description="Python bindings for UCRA audio synthesis library",
    long_description="""
UCRA Python bindings provide a Pythonic interface to the UCRA audio synthesis
library.
Features include:
- Easy-to-use engine for audio rendering
- NumPy integration for efficient data handling
- Support for F0 and envelope curves
- Manifest-based voicebank management
- Automatic memory management through Python's garbage collector
    """.strip(),
    ext_modules=ext_modules,
    extras_require={"test": "pytest"},
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.6",
    install_requires=[
        "numpy>=1.15.0",
    ],
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Topic :: Multimedia :: Sound/Audio",
        "Topic :: Software Development :: Libraries :: Python Modules",
    ],
)
