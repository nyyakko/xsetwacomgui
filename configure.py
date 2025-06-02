import os
import sys

sys.argv.pop(0)

os.system(f'cmake --preset debug { " ".join(sys.argv) }')

