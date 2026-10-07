"""Command line: python env.py up | down | status | tunnel"""
import argparse
import subprocess
from concurrent.futures import ThreadPoolExecutor

from testtool.config import CONFIG, NODES, nodes_on
from testtool.infra import get_ips, get_private_ips, terraform
from testtool.monitoring import write_prometheus_config
from testtool.provision import setup_machine
from testtool.remote import ssh
from testtool.tunnels import close_tunnel, open_all_tunnels


def up():
    try:
        print("Starting server...")

        terraform("validate")
        terraform("apply", "-auto-approve")

        ips = get_ips()
        write_prometheus_config(get_private_ips())

        print("Server(s) is up at:")
        for name in ips:
            print(f"{name}: {ips[name]}")

        with ThreadPoolExecutor() as pool:
            list(pool.map(setup_machine, ips.keys(), ips.values()))

        open_all_tunnels(ips)


    except subprocess.CalledProcessError as error:
        print(f"\nCommand failed (exit {error.returncode}): {' '.join(error.cmd[-1:])}")
        if error.stdout:
            print("--- output ---")
            print(error.stdout)
        if error.stderr:
            print("--- error ---")
            print(error.stderr)

    except Exception as error:
        print(f"Error: {error}")
        print("Server(s) left running for debugging. Run: python env.py down")


def down():
    print("Shutting down...")
    for machine in [m["name"] for m in CONFIG["infrastructure"]["machines"]]:
        close_tunnel(machine)
    terraform("destroy", "-auto-approve")


def status():
    print("Checking status...")
    ips = get_ips()
    if not ips:
        print("No servers running (according to Terraform).")
        return
    for name, ip in ips.items():
        print(f"\n{name}: {ip}")
        for node in nodes_on(name):
            port = NODES[node]["port"]
            try:
                code = ssh(ip, f"curl -s -o /dev/null -w '%{{http_code}}' http://localhost:{port}/domibus/ || true").strip()
            except subprocess.CalledProcessError:
                code = "no SSH"
            state = "up" if code not in ("", "000", "no SSH") else "not answering"
            print(f"  {node:<6} port {port}: {state} ({code})")


def main():
    parser = argparse.ArgumentParser(description="Manage the Domibus test environment")
    sub = parser.add_subparsers(dest="command", required=True)

    sub.add_parser("up", help="Create servers and start Domibus")
    sub.add_parser("down", help="Destroy server")
    sub.add_parser("status", help="Show whats running")
    sub.add_parser("tunnel", help="(Re)open the SSH tunnel(s) to the running server(s)")

    args = parser.parse_args()

    if args.command == "up":
        up()
    elif args.command == "down":
        down()
    elif args.command == "status":
        status()
    elif args.command == "tunnel":
        open_all_tunnels(get_ips())
