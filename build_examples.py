#!/usr/bin/env python

"""
Builds all of the examples in the src/ dir. 
A Makefile is required in each example subdir, or the commands will fail.
"""

import os


def ex_subdirs(src_dir):
    """
    Gets the subdirs of src_dir
    """
    return [
        name
        for name in os.listdir(src_dir)
        if os.path.isdir(os.path.join(src_dir, name))
    ]


for example in ex_subdirs("./src"):
    cwd = os.path.join("./src", example)
    os.system(f"echo Building: {example} ...")
    exit_code = os.system(f"make -C {cwd} clean")
    exit_code = os.system(f"make -C {cwd}")

os.system("echo Done!")
