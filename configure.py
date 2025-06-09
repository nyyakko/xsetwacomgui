import os
import sys

def main(arguments):
    preset = "debug"

    if len(arguments) and not arguments[0].startswith("-D"):
        preset = arguments[0].lower()
        arguments.pop(0)

    if preset != "debug" and preset != "release":
        print(f'The preset {preset} is invalid.')
        return

    os.system(f'cmake --preset { preset } { " ".join(sys.argv) }')

if __name__ == "__main__":
    sys.argv.pop(0)
    main(sys.argv)


