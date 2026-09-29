# up - creates the server, install Docker and Domibus
# down - destroys server
# status

# modules
# ==============================
# create server
# install Docker
# install Domibus
# set up file structure (red/blue)
# docker compose
# ==============================

import argparse
import subprocess
import time
import json

KEY = "Key.pem"

def terraform(*args):
    result = subprocess.run(
        ["terraform", "-chdir=terraform" , *args],
        capture_output=True,
        text=True,
        check=True
    )
    return result.stdout

def ssh_command(ip):
    return [
        "ssh",
        "-i", KEY,
        "-o", "StrictHostKeyChecking=no",
        "-o", "UserKnownHostsFile=/dev/null",
        "-o", "ConnectTimeout=5",
        "-o", "LogLevel=ERROR",
        f"ubuntu@{ip}",
    ]

def ssh(ip, command, input_file = None):
    stdin = open(input_file) if input_file else None
    result = subprocess.run(
        ssh_command(ip) + [command],
        stdin=stdin,
        capture_output=True,
        text=True,
        check=True
    )
    return result.stdout

def wait_for_ssh(ip, timeout=180):
    print("Waiting for SSH...")
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            ssh(ip, "true")
            return
        except subprocess.CalledProcessError:
            time.sleep(5)
    raise TimeoutError(f"No SSH connection to {ip} after {timeout}s")

def install_docker(ip):
    print("installing docker...")
    ssh(ip, "sudo bash -s", "scripts/install_docker.sh")
    print("Checking docker...")
    print(ssh(ip, "docker --version").strip())
    print(ssh(ip, "docker compose version").strip())

def install_domibus(ip):
    print("installing Domibus...")
    ssh(ip, "sudo bash -s", "scripts/install_domibus.sh")
    print("Domibus installed")
    print(ssh(ip, "ls /opt/lab").strip())

def wait_for_domibus(ip, port, name, timeout = 600):
    print(f"Waiting for {name}", end="", flush=True)
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            ssh(ip, f"curl -fs -o /dev/null http://localhost:{port}/domibus/")
            print(" ready")
            return
        except subprocess.CalledProcessError:
            print(".", end="", flush=True)
            time.sleep(10)
    raise TimeoutError(f"{name} did not start withing {timeout}s")

def start_domibus(ip):
    print("Starting blue and red...")
    ssh(ip, "cat > /opt/lab/compose.yaml", "docker/compose.yaml")
    ssh(ip, "cat > /opt/lab/prometheus.yaml", "docker/grafana/prometheus.yaml")
    ssh(ip, "cat > /opt/lab/datasource.yaml", "docker/grafana/datasource.yaml")

    ssh(ip, "cd /opt/lab && sudo docker compose up -d")
    wait_for_domibus(ip, 8081, "blue")
    wait_for_domibus(ip, 8082, "red")

TUNNEL_SOCKET = ".tunnel.sock"
TUNNEL_PORTS = [3000, 8081, 8082, 9090]

def open_tunnel(ip):
    close_tunnel()                      # close an old one first, if any
    forwards = []
    for port in TUNNEL_PORTS:
        forwards += ["-L", f"{port}:localhost:{port}"]

    subprocess.run(
        [
            "ssh", "-i", str(KEY),
            "-o", "StrictHostKeyChecking=no",
            "-o", "UserKnownHostsFile=/dev/null",
            "-o", "LogLevel=ERROR",
            "-o", "ExitOnForwardFailure=yes",
            "-o", "ServerAliveInterval=30",
            "-f", "-N",
            "-M", "-S", TUNNEL_SOCKET,
            *forwards,
            f"ubuntu@{ip}",
        ],
        check=True,
    )
    print("Tunnel open:")
    print("  Grafana     http://localhost:3000")
    print("  Blue        http://localhost:8081/domibus")
    print("  Red         http://localhost:8082/domibus")
    print("  Prometheus  http://localhost:9090")


def close_tunnel():
    subprocess.run(
        ["ssh", "-S", TUNNEL_SOCKET, "-O", "exit", "tunnel"],
        capture_output=True,            # quiet if no tunnel is open
    )

    
def up():
    try:
        print("Starting server...")

        # terraform("init")
        terraform("plan")
        # terraform("validate")
        terraform("apply", "-auto-approve")

        outputs = json.loads(terraform("output", "-json"))
        ip = outputs["public_ip"]["value"]
        print(f"Server is up at {ip}")

        wait_for_ssh(ip)
        install_docker(ip)
        install_domibus(ip)
        start_domibus(ip)
        open_tunnel(ip)
    
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
        terraform("destroy", "-auto-approve")

    
def down():
    print("Shutting down...")
    close_tunnel()
    terraform("destroy", "-auto-approve")

def status():
    print("Checking status...")

parser = argparse.ArgumentParser(description="Manage the Domibus test environment")
sub = parser.add_subparsers(dest="command", required=True)

sub.add_parser("up", help="Create the server and start Domibus")
sub.add_parser("down", help="Destroy server")
sub.add_parser("status", help="Show whats running")

args = parser.parse_args()

if args.command == "up":
    up()
elif args.command == "down":
    down()
