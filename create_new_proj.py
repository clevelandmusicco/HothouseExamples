#!/usr/bin/env python
"""
Helper script to create a new, blank Hothouse effect project.
"""

import argparse
import os
import re
from pathlib import Path


def copy_and_replace_directory(
    src_dir, dst_dir, tokens_to_replace, filenames_to_replace
):
    """
    Recursively copy a directory, replacing specified tokens in all files
    and renaming files as needed.

    Args:
        src_dir (Path): Path to the source directory.
        dst_dir (Path): Path to the destination directory.
        tokens_to_replace (dict): Dictionary where keys are the tokens to
                                  be replaced in file content
                                  and values are the replacement strings.
        filenames_to_replace (dict): Dictionary where keys are the tokens to
                                     be replaced in file names
                                     and values are the replacement strings.
    """
    content_pattern = re.compile("|".join(re.escape(key) for key in tokens_to_replace))
    filename_pattern = re.compile(
        "|".join(re.escape(key) for key in filenames_to_replace)
    )

    for root, _, files in sorted(os.walk(src_dir)):
        rel_path = Path(root).relative_to(src_dir)
        new_dir = dst_dir / rel_path
        new_dir.mkdir(parents=True, exist_ok=True)

        for file in sorted(files):
            src_file = Path(root) / file
            new_filename = filename_pattern.sub(
                lambda match: filenames_to_replace[match.group(0)], file
            )
            dst_file = new_dir / new_filename
            print(f"Creating {dst_file} ...")

            with src_file.open("r", encoding="utf-8") as src, dst_file.open(
                "w", encoding="utf-8"
            ) as dst:
                for line in src:
                    new_line = content_pattern.sub(
                        lambda match: tokens_to_replace[match.group(0)], line
                    )
                    dst.write(new_line)


def camel_to_snake(camel_str):
    """
    Helper function to convert CamelCase or PascalCase strings to snake_case.
    """
    return re.sub(r"(?<!^)(?=[A-Z])", "_", camel_str).lower()


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Create a new Hothouse effect project."
    )
    parser.add_argument(
        "--proj_name",
        type=str,
        required=True,
        help="Name of the new project in camelCase or PascalCase.",
    )
    parser.add_argument(
        "--your_name",
        type=str,
        default="Your Name",
        help="Your name for use in the license and README.",
    )
    parser.add_argument(
        "--your_email",
        type=str,
        default="your@email",
        help="Your email address for use in the license and README.",
    )
    return parser.parse_args()


def main():
    args = parse_arguments()

    src_dir = Path("resources/_template")
    dest_dir = Path(f"src/{args.proj_name}")
    tokens = {
        "@@template_uc": args.proj_name,
        "@@template_sc": camel_to_snake(args.proj_name),
        "@@your_name": args.your_name,
        "@@your_email": args.your_email,
    }

    filenames = {
        "_template.cpp": f"{camel_to_snake(args.proj_name)}.cpp",
    }

    copy_and_replace_directory(src_dir, dest_dir, tokens, filenames)
    print("Done!")


if __name__ == "__main__":
    main()
