from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

ext_modules = [
    Pybind11Extension(
        "html_parser",
        ["html_pybind.cpp", "HTMLFile.cpp"],  # Add source files here
        include_dirs=["."],  # Path to the header files
    ),
]

setup(
    name="html_parser",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
)
