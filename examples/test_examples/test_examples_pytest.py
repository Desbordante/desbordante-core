import os
import re
import runpy
import subprocess
import sys
from pathlib import Path

import pytest
import snapshottest

EXAMPLES_DIR = Path("examples")
INPUTS_DIR = EXAMPLES_DIR / "test_examples" / "inputs"
TEST_EXAMPLES_SUBDIR = EXAMPLES_DIR / "test_examples"
EXCLUDED_FILES = {
    "basic/mining_sfd.py",
}

def generate_test_cases():
    """Dynamically discovers and generates test cases for Python example scripts."""
    test_cases = []

    for script_path in EXAMPLES_DIR.rglob("*.py"):
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

@pytest.mark.usefixtures("snapshot")
@pytest.mark.parametrize(("script", "input_file", "output"), TEST_CASES)
def test_example(script, input_file, output):
    cmd = [sys.executable, f"examples/{script}"]
    stdin = None
    if input_file:
        with open(f"examples/test_examples/inputs/{input_file}") as f:
            stdin = f.read()
    env = os.environ.copy()
    # To skip plt.show()
    env["MPLBACKEND"] = "Agg"

    # The test intentionally executes examples discovered in the repository.
    result = subprocess.run(  # noqa: S603
        cmd,
        input=stdin,
        text=True,
        capture_output=True,
        check=False,
        env=env
    )

    if result.returncode != 0:
        error_msg = (
            f"Script {script} exited with code {result.returncode}.\n"
            f"Stderr:\n{result.stderr}"
        )
        raise AssertionError(error_msg)

    result_output = result.stdout
    if script == "basic/mining_afd.py":
        result_output = re.sub(
            r"(\|.+(?:Pyro|Tane).+\|.+\|.+)\d+\.\d{3} s",
            r"\1<runtime>",
            result_output,
        )
        result_output = re.sub(
            r"was \d+\.\d{2} times faster",
            "was <speedup> times faster",
            result_output,
        )

    snapshottest.assert_match_snapshot(result_output, output)


@pytest.mark.parametrize("script", ["dynamic_verifying_fd.py", "dynamic_verifying_afd.py"])
def test_dynamic_indices_after_deletion(script):
    example = runpy.run_path(f"examples/basic/{script}")
    pd = example["pd"]
    update = example["update_and_print_data_table"]
    data = pd.DataFrame({"key": ["a", "a", "a"], "value": [10, 10, 10]})
    verifier = example["desbordante"].dynamic_fd_verification.algorithms.Default()
    verifier.load_data(table=data, lhs_indices=[0], rhs_indices=[1])
    assert verifier.fd_holds()

    deleted_index = 2
    verifier.execute(delete={deleted_index})
    data, last_index = update(data, deleted_index, delete_set={deleted_index})
    assert data.index.tolist() == [0, 1]
    assert last_index == deleted_index

    insert_df = pd.DataFrame({"key": ["a"], "value": [40]})
    verifier.execute(insert=insert_df)
    data, last_index = update(data, last_index, insert_df=insert_df)
    assert data.index.tolist() == [0, 1, 3]
    assert last_index == deleted_index + 1
    assert not verifier.fd_holds()
    highlights = verifier.get_highlights()
    assert len(highlights) == 1
    assert set(highlights[0].cluster) == set(data.index)
