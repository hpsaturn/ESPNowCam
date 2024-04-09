# pre-build script for symbolic lynks generation

import os
import os.path

example_lib_dir = "examples/espcamlib"
dst = "examples/espcamlib/src"
src = "../../src"

os.makedirs(example_lib_dir, 0o755, True)

if not os.path.exists(dst):
    os.symlink(src, dst)

print("Symbolic link created successfully")
