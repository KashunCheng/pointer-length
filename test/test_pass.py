import os
import subprocess
from glob import glob

import pytest

test_dir = os.path.dirname(os.path.realpath(__file__))
resource_dir = os.path.join(test_dir, "resources")


def compile_c(filename):
    ir_filename = os.path.splitext(filename)[0] + '.ll'
    if os.path.exists(os.path.join(resource_dir, ir_filename)):
        return
    # Run clang -S -fno-discard-value-names -emit-llvm test.c -o test.ll for the source file
    subprocess.run(
        ['clang', '-O0', '-S', '-fno-discard-value-names', '-emit-llvm', os.path.join(resource_dir, filename), '-o',
         os.path.join(resource_dir, ir_filename)])


def find_pass_plugin():
    # Find the path to the pass plugin
    # It will be in the build directory (PROJECT_ROOT/*/lib/*.{so,dylib})
    path_wo_ext = os.path.join(test_dir, "..", "*", "lib", "libHelloWorld")
    pass_plugin = glob(path_wo_ext + ".so") + glob(path_wo_ext + ".dylib")
    assert len(pass_plugin) == 1
    return pass_plugin[0]


@pytest.mark.parametrize("filename", filter(lambda x: x.endswith('.c'), os.listdir(resource_dir)))
def test_pass(filename):
    compile_c(filename)
    expected = os.path.splitext(filename)[0] + '.out'
    assert os.path.exists(os.path.join(resource_dir, expected))
    with open(os.path.join(resource_dir, expected), 'r') as f:
        expected = f.read()
    # Run opt -load-pass-plugin PATH_TO_THE_PLUGIN -passes=hello-world -disable-output demo.ll
    pass_plugin = find_pass_plugin()
    # Check if the result from stderr matches the expected result
    result = subprocess.run(['opt', '-load-pass-plugin', pass_plugin, '-passes=hello-world', '-disable-output',
                             os.path.join(resource_dir, os.path.splitext(filename)[0] + '.ll')], capture_output=True)
    assert result.stderr.decode('utf-8') == expected
