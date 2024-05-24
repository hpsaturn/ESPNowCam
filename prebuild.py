# pre-build script for symbolic lynks generation

import os
import os.path
import shutil

example_lib_dir = "examples/lib/espcamlib"
dst = "examples/lib/espcamlib/src"
src = "../../../src"
lib = "src"

os.makedirs(example_lib_dir, 0o755, True)

if not os.path.exists(dst) and os.name != 'nt':
    os.symlink(src, dst)
    print("Symbolic link created successfully")
elif not os.path.exists(dst) and os.name == 'nt':
    shutil.copytree(lib, dst)
    print("Source lib for examples created successfully")