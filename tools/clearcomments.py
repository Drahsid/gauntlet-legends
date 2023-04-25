#!/usr/bin/python3

import re
import sys

def remove_multiline_comments(input_file):
    with open(input_file, 'r') as file:
        text = file.read()

    pattern = r'/\*[\s\S]*?\*/'
    result = re.sub(pattern, '', text)

    # Remove empty lines from the result
    lines = result.splitlines()
    non_empty_lines = [line for line in lines if line.strip()]

    with open(input_file, 'w') as file:
        file.write('\n'.join(non_empty_lines))

if __name__ == "__main__":
    input_file = sys.argv[1]
    remove_multiline_comments(input_file)
