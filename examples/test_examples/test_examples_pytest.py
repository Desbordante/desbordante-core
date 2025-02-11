import os
import subprocess
import pytest
import snapshottest


TEST_CASES = [
    # (example_path, input_file, output_name)
    ('advanced/afd_multiple_error_thresholds.py', None, 'afd_multiple_error_thresholds_output'),
    ('advanced/aind_typos.py', None, 'aind_typos_output'),
    ('advanced/comparison_mining_fd_approximate.py', None, 'comparison_mining_fd_approximate_output'),
    ('advanced/comparison_pfd_vs_afd.py', None, 'comparison_pfd_vs_afd_output'),
    ('advanced/comparison_ucc_and_aucc_1.py', None, 'comparison_ucc_and_aucc_1_output'),
    ('advanced/comparison_ucc_and_aucc_2.py', None, 'comparison_ucc_and_aucc_2_output'),
    ('advanced/md_semantic_checks.py', None, 'md_semantic_checks_output'),
    ('basic/mining_gfd/mining_gfd1.py', None, 'mining_gfd1_output'),
    ('basic/mining_gfd/mining_gfd2.py', None, 'mining_gfd2_output'),
    ('basic/verifying_gfd/verifying_gfd1.py', None, 'verifying_gfd1_output'),
    ('basic/verifying_gfd/verifying_gfd2.py', None, 'verifying_gfd2_output'),
    ('basic/verifying_gfd/verifying_gfd3.py', None, 'verifying_gfd3_output'),
    ('basic/verifying_nd/verifying_nd_1.py', None, 'verifying_nd_1_output'),
    ('basic/verifying_nd/verifying_nd_2.py', None, 'verifying_nd_2_output'),
    ('basic/verifying_nd/verifying_nd_3.py', None, 'verifying_nd_3_output'),
    ('basic/data_stats.py', None, 'data_stats_output'),
    ('basic/dynamic_verifying_afd.py', None, 'dynamic_verifying_afd_output'),
    ('basic/dynamic_verifying_fd.py', None, 'dynamic_verifying_fd_output'),
    ('basic/mining_ac.py', None, 'mining_ac_output'),
    ('basic/mining_adc.py', None, 'mining_adc_output'),
    ('basic/mining_afd.py', None, 'mining_afd_output'),
    ('basic/mining_aind.py', None, 'mining_aind_output'),
    ('basic/mining_ar.py', None, 'mining_ar_output'),
    ('basic/mining_aucc.py', None, 'mining_aucc_output'),
    ('basic/mining_cfd.py', None, 'mining_cfd_output'),
    ('basic/mining_dd.py', None, 'mining_dd_output'),
    ('basic/mining_fd.py', None, 'mining_fd_output'),
    ('basic/mining_fd_approximate.py', None, 'mining_fd_approximate_output'),
    ('basic/mining_ind.py', None, 'mining_ind_output'),
    ('basic/mining_list_od.py', None, 'mining_list_od_output'),
    ('basic/mining_md.py', None, 'mining_md_output'),
    ('basic/mining_nar.py', None, 'mining_nar_output'),
    ('basic/mining_pfd.py', None, 'mining_pfd_output'),
    ('basic/mining_set_od_1.py', None, 'mining_set_od_1_output'),
    ('basic/mining_set_od_2.py', None, 'mining_set_od_2_output'),
    # Output is inconstant
    # ('basic/mining_sfd.py', None, 'mining_sfd_output'),
    ('basic/mining_ucc.py', None, 'mining_ucc_output'),
    ('basic/verifying_aucc.py', None, 'verifying_aucc_output'),
    ('basic/verifying_dc.py', None, 'verifying_dcoutput'),
    ('basic/verifying_fd_afd.py', None, 'verifying_fd_afd_output'),
    ('basic/verifying_ind_aind.py', None, 'verifying_ind_aind_output'),
    ('basic/verifying_mfd.py', None, 'verifying_mfd_output'),
    ('basic/verifying_pfd.py', None, 'verifying_pfd_output'),
    ('basic/verifying_ucc.py', None, 'verifying_ucc_output'),
    ('expert/anomaly_detection.py', None, 'anomaly_detection_output'),
    ('expert/data_cleaning_dc.py', None, 'data_cleaning_dc_output'),
    ('expert/mine_typos.py', None, 'mine_typos_output'),
    ('expert/dedupe.py', 'dedupe_input.txt', 'dedupe_output')
]


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
