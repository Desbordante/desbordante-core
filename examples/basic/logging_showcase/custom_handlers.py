import logging
import sys
import json
from datetime import datetime
import desbordante

TABLE = 'examples/datasets/ucc_datasets/aucc.csv'
COLOR_CODES = {
    'bold_green': '\033[1;32m',
    'bold_blue': '\033[1;34m',
    'default': '\033[0m',
    'blue_bg': '\033[1;46m',
    'default_bg': '\033[1;49m'
}

def algo_process_aucc():
    print("\n>>> Running a C++ algorithm (PyroUCC)...")
    algo = desbordante.ucc.algorithms.PyroUCC()
    algo.load_data(table=(TABLE, ',', True))
    algo.execute(error=0)
    print("<<< C++ algorithm finished.")

class JsonFormatter(logging.Formatter):
    def format(self, record):
        log_record = {
            "timestamp": datetime.fromtimestamp(record.created).isoformat(),
            "level": record.levelname,
            "message": record.getMessage(),
            "source": record.name
        }
        return json.dumps(log_record)

def prepare_logger_for_scenario(log):
    log.handlers.clear()
    log.propagate = False
    log.setLevel(logging.INFO)

def scenario_log_to_file(log):
    print(f"\n{COLOR_CODES['blue_bg']} SCENARIO 1: Logging to a File {COLOR_CODES['default_bg']}")
    print("First, let's redirect all output from the C++ core to a text file.")
    print("This is useful for creating detailed, human-readable reports of a run.")
    
    prepare_logger_for_scenario(log)
    
    log_filename = "desbordante_run.log"
    file_handler = logging.FileHandler(log_filename, mode='w')
    file_formatter = logging.Formatter(
        '[%(asctime)s] [%(name)s] [%(levelname)-8s] %(message)s'
    )
    file_handler.setFormatter(file_formatter)
    log.addHandler(file_handler)

    print(f"\nA {COLOR_CODES['bold_green']}FileHandler{COLOR_CODES['default']} has been attached. No logs will appear on the console.")
    print(f"All output is now being written to {COLOR_CODES['bold_blue']}{log_filename}{COLOR_CODES['default']}.")

    log.info("This is the first message from Python, written to the file.")
    algo_process_aucc()
    log.info("This is the final message from Python.")
    
    print(f"\nScenario complete. Please inspect the contents of the '{log_filename}' file.")

def scenario_log_as_json(log):
    print(f"\n{COLOR_CODES['blue_bg']} SCENARIO 2: Logging as JSON to Console {COLOR_CODES['default_bg']}")
    print("Next, we'll configure the logger to output messages in JSON format.")
    print("This is ideal for integration with log aggregation systems like Elasticsearch or Splunk.")

    prepare_logger_for_scenario(log)

    json_handler = logging.StreamHandler(sys.stdout)
    json_handler.setFormatter(JsonFormatter())
    log.addHandler(json_handler)

    print(f"\nA {COLOR_CODES['bold_green']}StreamHandler{COLOR_CODES['default']} with a custom "
          f"{COLOR_CODES['bold_blue']}JsonFormatter{COLOR_CODES['default']} is now active.")
    print("All subsequent logs will be printed below in JSON format.")
    
    log.info("This is a info from Python, now formatted as JSON.")
    algo_process_aucc()

def main():
    print(f"{COLOR_CODES['default_bg']}Showcase: Custom Handlers & Formatters{COLOR_CODES['default']}")
    print("This example demonstrates how to take complete control over where to C++ logs " \
    "are sent and how they are formatted using standard Python tools.")

    log = logging.getLogger("desbordante")

    scenario_log_to_file(log)
    scenario_log_as_json(log)

if __name__ == "__main__":
    main()