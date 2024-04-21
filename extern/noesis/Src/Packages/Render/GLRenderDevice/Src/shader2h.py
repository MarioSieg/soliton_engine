#!/usr/bin/env python

from argparse import ArgumentParser
import sys

__version__ = "1.0"

def main():

    parser = ArgumentParser(description="OpenGL shader to C header converter")
    parser.add_argument("--version", action="version", version="%(prog)s " + __version__)
    parser.add_argument("file", help="the file to be converted")
    parser.add_argument("name", help="variable name in header file")

    args = parser.parse_args()

    # Line with #version is removed because at runtime extra defines are prepended
    sys.stdout.write('static const char* %s = R"(' % args.name)

    with open(args.file) as f:
        lines = f.readlines()
        for line in lines:
            if line.startswith('#version'):
                sys.stdout.write('\n')
            else:
                sys.stdout.write(line)

    sys.stdout.write('\n)";')

if __name__ == "__main__":
    main()
