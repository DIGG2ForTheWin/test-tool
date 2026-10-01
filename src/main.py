import argparse
import sys
from config import get_test_cases, load_config


def main():
    parser = argparse.ArgumentParser(
        description="Domibus Integration & Performance Tester"
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    # run-kommandot
    run_parser = subparsers.add_parser(
        "run", help="Running test configurations from tests.yaml"
    )
    run_parser.add_argument(
        "test_target",
        metavar="TEST_NAME",
        type=str,
        help="Name for specific test, or 'all'",
    )
    run_parser.add_argument(
        "--config",
        "-c",
        default="tests.yaml",
        help="Path to config-file (default: tests.yaml)",
    )

    args = parser.parse_args()

    try:
        config = load_config(args.config)
        selected_tests = get_test_cases(config, args.test_target)

        print(f"Loading configuration from {args.config}")
        print(f"Found {len(selected_tests)} testcases to run:\n")

        for test in selected_tests:
            print(f"  * Test: {test['name']}")
            print(f"    Route: {test['from']} -> {test['to']}")
            print(
                f"    File: {test['count']} qt {test['file_size_kb']} KB"
            )
            print("-" * 30)

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()