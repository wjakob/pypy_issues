from setuptools import Extension, setup

setup(
    py_modules=[],
    ext_modules=[
        Extension(
            name="pypy_issues",
            sources=["pypy_issues.c"],
        ),
    ]
)
