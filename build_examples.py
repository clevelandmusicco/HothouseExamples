#!/usr/bin/env python

"""
Builds all of the examples in the src/ dir.
A Makefile is required in each example subdir, or the commands will fail.
"""
import argparse
import subprocess
from pathlib import Path


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Build examples and optionally publish the .bin files."
    )
    parser.add_argument(
        "--publish_dir",
        type=Path,
        required=False,
        help="Location to copy *.bin files after building.",
    )
    return parser.parse_args()


def get_example_subdirs(src_dir):
    """
    Gets the subdirs of src_dir
    """
    return [p for p in src_dir.iterdir() if p.is_dir()]


def run_command(command, cwd=None):
    """
    Run a shell command and raise an error if it fails
    """
    result = subprocess.run(command, cwd=cwd, shell=True, check=True)
    return result.returncode


def build_examples(src_dir, publish_dir=None):
    for example in get_example_subdirs(src_dir):
        print(f"Building: {example.name} ...")
        try:
            run_command("make clean", cwd=example)
            run_command("make", cwd=example)
            if publish_dir:
                run_command(f"make publish PUBLISH_DIR={publish_dir}", cwd=example)
        except subprocess.CalledProcessError as e:
            print(f"Error building {example.name}: {e}")
            continue


def main():
    args = parse_arguments()
    src_dir = Path("./src")
    build_examples(src_dir, args.publish_dir)
    print("Done!")


if __name__ == "__main__":
    main()
