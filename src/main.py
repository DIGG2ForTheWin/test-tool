import argparse
import sys
import time
from testtool.config import load_config, get_test_cases
from payload import create_payload_info, cleanup_payload
from ws_client import DomibusWSClient
from zeep.exceptions import Fault  # Installed via requirements.txt in the Docker image


def main():
    parser = argparse.ArgumentParser(
        description="Domibus Integration & Performance Test Runner"
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    run_parser = subparsers.add_parser(
        "run", help="Execute test cases configured in tests.yaml"
    )
    run_parser.add_argument(
        "test_target",
        metavar="TEST_NAME",
        type=str,
        help="Name of the test case to execute, or 'all'",
    )
    run_parser.add_argument(
        "--config",
        "-c",
        default="tests.yaml",
        help="Path to configuration file (default: tests.yaml)",
    )

    args = parser.parse_args()

    try:
        config = load_config(args.config)
        selected_tests = get_test_cases(config, args.test_target)
        targets = config.get("targets", {})

        print(f"Loaded configuration from: {args.config}")
        print(f"Found {len(selected_tests)} test case(s) to execute.\n")

        for test in selected_tests:
            sender_name = test["from"]
            receiver_name = test["to"]
            target_info = targets.get(sender_name)

            if not target_info:
                raise ValueError(
                    f"Sender node '{sender_name}' is not defined under 'targets' in the configuration."
                )

            print(f"==================================================")
            print(f"Running test: {test['name']}")
            print(f"Route: {sender_name} ({target_info['url']}) -> {receiver_name}")
            print(f"Plan:  {test['count']} message(s) of {test['file_size_kb']} KB")
            print(f"==================================================")

            client = DomibusWSClient(
                base_url=target_info["url"],
                username=target_info.get("user"),
                password=target_info.get("password"),
            )

            for i in range(1, test["count"] + 1):
                payload = create_payload_info(test, sequence_num=i)
                start_time = time.time()

                try:
                    message_id = client.send_message(
                        from_party_id=sender_name,
                        to_party_id=receiver_name,
                        payload_meta=payload,
                    )
                    elapsed = time.time() - start_time
                    print(
                        f"  [{i}/{test['count']}] Sent successfully! "
                        f"MessageId: {message_id} ({elapsed:.2f}s)"
                    )
                except Fault as fault:
                    elapsed = time.time() - start_time
                    print(
                        f"  [{i}/{test['count']}] SOAP Fault ({elapsed:.2f}s): {fault.message}",
                        file=sys.stderr,
                    )
                    if fault.detail is not None:
                        # Print the XML fault details returned by Domibus
                        from lxml import etree

                        detail_str = etree.tostring(fault.detail, pretty_print=True).decode()
                        print(f"  Fault Detail:\n{detail_str}", file=sys.stderr)
                except Exception as ex:
                    elapsed = time.time() - start_time
                    print(
                        f"  [{i}/{test['count']}] Failed: {ex} ({elapsed:.2f}s)",
                        file=sys.stderr,
                    )
                finally:
                    cleanup_payload(payload)

            print(f"Completed test: {test['name']}\n")

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()