#!/usr/bin/env python

import os
from argparse import ArgumentParser

__version__ = "1.0"

def main():

    parser = ArgumentParser(description="Tar")
    parser.add_argument("--version", action="version", version="%(prog)s "  + __version__)
    parser.add_argument("tar", help="the archive file")
    parser.add_argument("file", help="the file to be archived")

    args = parser.parse_args()

    if os.path.isfile(args.tar):
        tar_size = os.path.getsize(args.tar)
    else:
        tar_size = 0

    file_size = os.path.getsize(args.file)
    var_name = os.path.splitext(args.file)[0]

    print("/* %s */" % var_name)
    print("static const uint32_t %s_Start = %d;" % (var_name, tar_size))
    print("static const uint32_t %s_Size = %d;\n" % (var_name, file_size))

    with open(args.tar, "ab+") as tar:
        with open(args.file, "rb") as file:
            tar.write(file.read())

if __name__ == "__main__":
    main()
