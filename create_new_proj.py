"""Helper script to create a new, blank Hothouse effect project."""

import argparse
import os
import re

def copy_and_replace_directory(src_dir, dst_dir, tokens_to_replace, filenames_to_replace):
    """
    Recursively copy a directory, replacing specified tokens in all files
    and renaming files as needed.

    Args:
        src_dir (str): Path to the source directory.
        dst_dir (str): Path to the destination directory.
        tokens_to_replace (dict): Dictionary where keys are the tokens to
                                     be replaced in file content
                                     and values are the replacement strings.
        filenames_to_replace (dict): Dictionary where keys are the tokens to
                                      be replaced in file names
                                      and values are the replacement strings.
    """
    # Create a regex pattern to match any of the tokens to be replaced in content
    content_pattern = re.compile('|'.join(re.escape(key) for key in tokens_to_replace.keys()))

    # Create a regex pattern to match any of the tokens to be replaced in file names
    filename_pattern = re.compile('|'.join(re.escape(key) for key in filenames_to_replace.keys()))

    # Walk through the source directory
    for root, dirs, files in os.walk(src_dir):
        # Determine the destination directory
        rel_path = os.path.relpath(root, src_dir)
        new_dir = os.path.join(dst_dir, rel_path)
        os.makedirs(new_dir, exist_ok=True)

        for file in files:
            src_file = os.path.join(root, file)

            # Rename the file if needed
            new_filename = filename_pattern.sub(
                lambda match: filenames_to_replace[match.group(0)],
                file)
            dst_file = os.path.join(new_dir, new_filename)

            # Copy and replace tokens in the file content
            with open(src_file, 'r', encoding='utf-8') as src, \
                open(dst_file, 'w', encoding='utf-8') as dst:
                for line in src:
                    new_line = content_pattern.sub(
                        lambda match: tokens_to_replace[match.group(0)],
                        line)
                    dst.write(new_line)

def camel_to_snake(camel_str):
    """
    Helper function to convert CamelCase or pascalCase strings to snake_case.
    """
    snake_str = re.sub('([A-Z])', r'_\1', camel_str).lower()
    return snake_str.lstrip('_')

# Create the parser
parser = argparse.ArgumentParser()

# Add arguments
parser.add_argument('--proj_name',
                    type=str,
                    required=True,
                    help='Name of the new project in camelCase or PascalCase.')
parser.add_argument('--your_name',
                    type=str,
                    required=False,
                    help='Your name for use in the license and README.',
                    default="Your Name")
parser.add_argument('--your_email',
                    type=str,
                    required=False,
                    help='Your email address for use in the license and README.',
                    default="your@email")

# Parse the arguments
args = parser.parse_args()

# Copy the _template project into a new project directory,
# replacing various string tokens along the way
SOURCE_DIR = 'resources/_template'
dest_dir = f'src/{args.proj_name}'
tokens = {
    '@@template_uc': f'{args.proj_name}',
    '@@template_sc': f'{camel_to_snake(args.proj_name)}',
    '@@your_name': f'{args.your_name}',
    '@@your_email': f'{args.your_email}',
}

filenames = {
    '_template.cpp': f'{camel_to_snake(args.proj_name)}.cpp',
}

copy_and_replace_directory(SOURCE_DIR, dest_dir, tokens, filenames)
