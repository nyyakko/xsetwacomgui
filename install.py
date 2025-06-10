import os
import sys

def main(arguments):
    prefix = "~/.local"

    if len(arguments):
        prefix = arguments[0]

    os.system(f'cmake --install build --prefix {prefix}')

if __name__ == "__main__":
    sys.argv.pop(0)
    main(sys.argv)
