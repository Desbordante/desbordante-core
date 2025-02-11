import os
from pathlib import Path
import subprocess
import pytest
import snapshottest

EXAMPLES_DIR = Path('examples')
INPUTS_DIR = EXAMPLES_DIR / 'test_examples' / 'inputs'
TEST_EXAMPLES_SUBDIR = EXAMPLES_DIR / 'test_examples'
EXCLUDED_FILES = {
    'basic/mining_sfd.py',
}

def generate_test_cases():
    """Dynamically discovers and generates test cases for Python example scripts."""
    test_cases = []

    for script_path in EXAMPLES_DIR.rglob('*.py'):
        if script_path.is_relative_to(TEST_EXAMPLES_SUBDIR):
            continue
        relative_path_str = script_path.relative_to(EXAMPLES_DIR).as_posix()
        if relative_path_str in EXCLUDED_FILES:
            continue
        output_name = f"{script_path.stem}_output"
        potential_input_file = INPUTS_DIR / f"{script_path.stem}_input.txt"
        input_file = potential_input_file.name if potential_input_file.exists() else None
        test_cases.append((relative_path_str, input_file, output_name))

    test_cases.sort()
    return test_cases

TEST_CASES = generate_test_cases()

@pytest.mark.parametrize('script, input_file, output', TEST_CASES)
def test_example(snapshot, script, input_file, output):
    cmd = ['python3', f'examples/{script}']
    stdin = None
    if input_file:
        with open(f'examples/test_examples/inputs/{input_file}') as f:
            stdin = f.read()
    env = os.environ.copy()
    # To skip plt.show()
    env["MPLBACKEND"] = "Agg"

    result = subprocess.run(
        cmd,
        input=stdin,
        text=True,
        capture_output=True,
        env=env
    )

    if result.returncode != 0:
        error_msg = (
            f"Script {script} exited with code {result.returncode}.\n"
            f"Stderr:\n{result.stderr}"
        )
        raise AssertionError(error_msg)

    result_output = result.stdout

    snapshottest.assert_match_snapshot(result_output, output)
