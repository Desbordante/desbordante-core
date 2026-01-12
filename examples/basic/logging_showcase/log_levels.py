import logging
import desbordante
import sys

TABLE = 'examples/datasets/ucc_datasets/aucc.csv'
COLOR_CODES = {
    'bold_green': '\033[1;32m',
    'bold_yellow': '\033[1;33m',
    'bold_blue': '\033[1;34m',
    'default': '\033[0m',
    'blue_bg': '\033[1;46m',
    'default_bg': '\033[1;49m'
}

def algo_process_aucc():
    print(">>> Running a C++ algorithm (PyroUCC)...")
    algo = desbordante.ucc.algorithms.PyroUCC()
    algo.load_data(table=(TABLE, ',', True))
    algo.execute(error=0)
    print("<<< C++ algorithm finished.")

def setup_logging():
    log = logging.getLogger("desbordante")
    handler = logging.StreamHandler(sys.stderr)
    log.addHandler(handler)
    log.setLevel(logging.INFO)
    return log


def scenario_info_level(log):
    print(f"\n{COLOR_CODES['blue_bg']} SCENARIO 1: INFO Level (Default Verbosity) {COLOR_CODES['default_bg']}")
    print("By default, the library is configured to show logs at the INFO level and higher.")
    print(f"We'll explicitly set it to {COLOR_CODES['bold_green']}INFO{COLOR_CODES['default']}{COLOR_CODES['default_bg']} to demonstrate.{COLOR_CODES['default']}")

    log.setLevel(logging.INFO)
    algo_process_aucc()

    print("\nAs you can see, only INFO, WARNING, ERROR, and CRITICAL messages (if any) are displayed.")


def scenario_debug_level(log):
    print(f"\n{COLOR_CODES['blue_bg']} SCENARIO 2: DEBUG Level (Detailed Verbosity) {COLOR_CODES['default_bg']}")
    print("To see more detailed messages from the C++ core, we can lower the log level.")
    print(f"Let's set it to {COLOR_CODES['bold_yellow']}DEBUG{COLOR_CODES['default']}.")

    log.setLevel(logging.DEBUG)
    algo_process_aucc()

    print("\nNow, the output includes verbose DEBUG messages, which are essential for troubleshooting.")


def scenario_trace_level(log):
    print(f"\n{COLOR_CODES['blue_bg']} SCENARIO 3: TRACE Level (Maximum Verbosity) {COLOR_CODES['default_bg']}")
    print(f"For deep debugging, the C++ core provides a {COLOR_CODES['bold_blue']}TRACE{COLOR_CODES['default']}{COLOR_CODES['default_bg']} level, which is even more")
    print("detailed than DEBUG. The desbordante library automatically registers this level")
    print(f"with Python's `logging` module for your convenience{COLOR_CODES['default']}.")

    log.setLevel(logging.TRACE)
    algo_process_aucc()

    print("\nWith TRACE enabled, you get the most granular view of the C++ core's execution.")


def main():
    print(f"{COLOR_CODES['default_bg']}Showcase: Controlling C++ Log Levels from Python\n{COLOR_CODES['default']}")
    print("This example demonstrates how to use Python's standard `logging` module " \
    "to control the log verbosity of the desbordante C++ core. " \
    f"The key is to use {COLOR_CODES['bold_green']}logging.getLogger('desbordante'){COLOR_CODES['default']}.")

    # Setup logging and get the logger object
    log = setup_logging()

    # Run through the scenarios
    scenario_info_level(log)
    scenario_debug_level(log)
    scenario_trace_level(log)

    handlers_list = log.handlers
    print(handlers_list)
    for handler in handlers_list:
        handler.flush()
if __name__ == "__main__":
    main()