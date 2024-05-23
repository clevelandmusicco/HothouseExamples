#!/usr/bin/env python

# Builds all of the examples in the src/ dir.
# A Makefile is required in each example subdir, or the commands will fail.

import sys
import os

def ex_subdirs(src_dir):
  return [name for name in os.listdir(src_dir)
    if os.path.isdir(os.path.join(src_dir, name))]

for example in ex_subdirs('./src'):  
  cwd = (os.path.join('./src', example))
  os.system("echo Building: {} ...".format(example))
  exit_code = os.system("make -C {} clean".format(cwd))
  exit_code = os.system("make -C {}".format(cwd))

os.system("echo Done!")
sys.exit