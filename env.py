# up     - creates the server(s), installs Docker and Domibus, opens the tunnel(s)
# down   - destroys the server(s)
# status - shows what's running
# tunnel - (re)opens the SSH tunnel(s) to the running server(s)
#
# Which Domibus node runs on which machine is read from tests.yaml:
#   infrastructure.machines  -> the servers Terraform creates
#   nodes.<name>.machine     -> which server a node runs on
#   nodes.<name>.port        -> the node's port on that server
#
# Old code is kept below as comments, marked "# OLD:".

import argparse
import subprocess
import time
import json
import os
from pathlib import Path

import yaml   # pip install pyyaml

# Run from the project folder, whatever folder the command is started from,
# so relative paths like "scripts/..." and "Key.pem" always work.
os.chdir(Path(__file__).resolve().parent)

KEY = "Key.pem"

with open("tests.yaml") as f:
    CONFIG = yaml.safe_load(f)

NODES = CONFIG["nodes"]                                   # {"blue": {"port": 8081, "machine": "lab-1"}, ...}
MONITORING = ["prometheus", "grafana", "cadvisor", "node-exporter"]
MONITORING_PORTS = {"grafana": 3000, "prometheus": 9090}


def nodes_on(machine):
    """Names of the Domibus nodes that run on this machine, e.g. ['blue', 'red']."""
    return [node for node, settings in NODES.items() if settings["machine"] == machine]


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

# OLD: def ssh(ip, command, input_file = None):
# OLD:     stdin = open(input_file) if input_file else None
# OLD:     result = subprocess.run(
# OLD:         ssh_command(ip) + [command],
# OLD:         stdin=stdin,
# OLD:         capture_output=True,
# OLD:         text=True,
# OLD:         check=True
# OLD:     )
# OLD:     return result.stdout

def ssh(ip, command, input_file=None):
    """Run a command on the server. Optionally send a local file as its input."""
    if input_file:
        with open(input_file) as f:            # "with" closes the file afterwards
            result = subprocess.run(ssh_command(ip) + [command], stdin=f,
                                    capture_output=True, text=True, check=True)
    else:
        result = subprocess.run(ssh_command(ip) + [command],
                                capture_output=True, text=True, check=True)
    return result.stdout

# OLD: def wait_for_ssh(ip, timeout=180):
# OLD:     print("Waiting for SSH...")
# OLD:     deadline = time.time() + timeout
# OLD:     while time.time() < deadline:
# OLD:         try:
# OLD:             ssh(ip, "true")
# OLD:             return
# OLD:         except subprocess.CalledProcessError:
# OLD:             time.sleep(5)
# OLD:     raise TimeoutError(f"No SSH connection to {ip} after {timeout}s")

def wait_for_ssh(ip, timeout=180):
    print(f"Waiting for SSH on {ip}", end="", flush=True)
    deadline = time.time() + timeout
    last_error = ""
    while time.time() < deadline:
        try:
            ssh(ip, "true")
            print(" ready")
            return
        except subprocess.CalledProcessError as error:
            last_error = (error.stderr or "").strip()
            print(".", end="", flush=True)
            time.sleep(5)
    raise TimeoutError(f"No SSH connection to {ip} after {timeout}s: {last_error}")

def install_docker(ip):
    print("installing docker...")
    ssh(ip, "sudo bash -s", "scripts/install_docker.sh")
    print("Checking docker...")
    print(ssh(ip, "docker --version").strip())
    print(ssh(ip, "docker compose version").strip())

# OLD: def install_domibus(ip):
# OLD:     print("installing Domibus...")
# OLD:     ssh(ip, "sudo bash -s", "scripts/install_domibus.sh")
# OLD:     print("Domibus installed")
# OLD:     print(ssh(ip, "ls /opt/lab").strip())

def install_domibus(ip, nodes):
    """Download Domibus once and unpack one copy per node on this machine."""
    print(f"installing Domibus: {', '.join(nodes)}...")
    # NODES='blue red' is passed to the script as a variable; it installs only those
    ssh(ip, f"sudo NODES='{' '.join(nodes)}' bash -s", "scripts/install_domibus.sh")
    print("Domibus installed")
    print(ssh(ip, "ls /opt/lab").strip())

# OLD: def wait_for_domibus(ip, port, name, timeout = 600):
# OLD:     print(f"Waiting for {name}", end="", flush=True)
# OLD:     deadline = time.time() + timeout
# OLD:     while time.time() < deadline:
# OLD:         try:
# OLD:             ssh(ip, f"curl -fs -o /dev/null http://localhost:{port}/domibus/")
# OLD:             print(" ready")
# OLD:             return
# OLD:         except subprocess.CalledProcessError:
# OLD:             print(".", end="", flush=True)
# OLD:             time.sleep(5)
# OLD:     raise TimeoutError(f"{name} did not start withing {timeout}s")

def wait_for_domibus(ip, port, name, timeout=600):
    """Wait until Domibus answers with any HTTP status (000 = nothing answers yet)."""
    print(f"Waiting for {name}", end="", flush=True)
    deadline = time.time() + timeout
    while time.time() < deadline:
        code = ssh(ip, f"curl -s -o /dev/null -w '%{{http_code}}' http://localhost:{port}/domibus/ || true").strip()
        if code and code != "000":
            print(f" ready (HTTP {code})")
            return
        print(".", end="", flush=True)
        time.sleep(5)
    raise TimeoutError(f"{name} did not start within {timeout}s")

# OLD: def start_domibus(ip):
# OLD:     print("Starting blue and red...")
# OLD:     ssh(ip, "sudo bash -s", "scripts/modify_docker.sh")
# OLD:     ssh(ip, "cat > /opt/lab/compose.yaml", "docker/compose.yaml")
# OLD:     ssh(ip, "cat > /opt/lab/prometheus.yaml", "docker/grafana/prometheus.yaml")
# OLD:     ssh(ip, "cat > /opt/lab/datasource.yaml", "docker/grafana/datasource.yaml")
# OLD:
# OLD:     ssh(ip, "cd /opt/lab && sudo docker compose up -d")
# OLD:     wait_for_domibus(ip, 8081, "blue")
# OLD:     wait_for_domibus(ip, 8082, "red")

def start_domibus(ip, nodes):
    """Upload the compose files and start only this machine's nodes + monitoring."""
    print(f"Starting {', '.join(nodes)}...")
    # unsecureLoginAllowed is now set in install_domibus.sh, per node
    # OLD: ssh(ip, "sudo bash -s", "scripts/modify_docker.sh")
    ssh(ip, "cat > /opt/lab/compose.yaml", "docker/compose.yaml")
    ssh(ip, "cat > /opt/lab/prometheus.yaml", "docker/grafana/prometheus.yaml")
    ssh(ip, "cat > /opt/lab/datasource.yaml", "docker/grafana/datasource.yaml")

    services = " ".join(nodes + MONITORING)
    ssh(ip, f"cd /opt/lab && sudo docker compose up -d --quiet-pull {services}")
    for node in nodes:
        wait_for_domibus(ip, NODES[node]["port"], node)

# OLD: TUNNEL_SOCKET = ".tunnel.sock"
# OLD: TUNNEL_PORTS = [3000, 8081, 8082, 9090]

def tunnel_socket(machine):
    """One control socket per machine, so several tunnels can be open at once."""
    return f".tunnel-{machine}.sock"


def tunnel_forwards(machine, index):
    """Local port -> port on the server, for one machine.

    Domibus ports are unique per node (blue 8081, red 8082), so they can be used
    as they are. Grafana/Prometheus run on every machine, so the 2nd machine gets
    3001/9091, the 3rd 3002/9092, and so on.
    """
    forwards = {}
    for node in nodes_on(machine):
        port = NODES[node]["port"]
        forwards[port] = port
    for service, port in MONITORING_PORTS.items():
        forwards[port + index] = port
    return forwards


# OLD: def open_tunnel(ip):
# OLD:     # close_tunnel()                      # close an old one first, if any
# OLD:     forwards = []
# OLD:     for port in TUNNEL_PORTS:
# OLD:         forwards += ["-L", f"{port}:localhost:{port}"]
# OLD:
# OLD:     subprocess.run(
# OLD:         [
# OLD:             "ssh", "-i", str(KEY),
# OLD:             "-o", "StrictHostKeyChecking=no",
# OLD:             "-o", "UserKnownHostsFile=/dev/null",
# OLD:             "-o", "LogLevel=ERROR",
# OLD:             "-o", "ExitOnForwardFailure=yes",
# OLD:             "-o", "ServerAliveInterval=30",
# OLD:             "-f", "-N",
# OLD:             "-M", "-S", TUNNEL_SOCKET,
# OLD:             *forwards,
# OLD:             f"ubuntu@{ip}",
# OLD:         ],
# OLD:         check=True,
# OLD:     )
# OLD:     print("Tunnel open:")
# OLD:     print("  Grafana     http://localhost:3000")
# OLD:     print("  Blue        http://localhost:8081/domibus")
# OLD:     print("  Red         http://localhost:8082/domibus")
# OLD:     print("  Prometheus  http://localhost:9090")

def open_tunnel(machine, ip, index):
    close_tunnel(machine)                       # close an old one first, if any
    forwards = tunnel_forwards(machine, index)
    args = []
    for local, remote in forwards.items():
        args += ["-L", f"{local}:localhost:{remote}"]

    subprocess.run(
        [
            "ssh", "-i", str(KEY),
            "-o", "StrictHostKeyChecking=no",
            "-o", "UserKnownHostsFile=/dev/null",
            "-o", "LogLevel=ERROR",
            "-o", "ExitOnForwardFailure=yes",
            "-o", "ServerAliveInterval=30",
            "-f", "-N",
            "-M", "-S", tunnel_socket(machine),
            *args,
            f"ubuntu@{ip}",
        ],
        check=True,
    )
    print(f"Tunnel open to {machine} ({ip}):")
    for node in nodes_on(machine):
        print(f"  {node:<11} http://localhost:{NODES[node]['port']}/domibus")
    print(f"  {'Grafana':<11} http://localhost:{MONITORING_PORTS['grafana'] + index}")
    print(f"  {'Prometheus':<11} http://localhost:{MONITORING_PORTS['prometheus'] + index}")


# OLD: def close_tunnel():
# OLD:     subprocess.run(
# OLD:         ["ssh", "-S", TUNNEL_SOCKET, "-O", "exit", "tunnel"],
# OLD:         capture_output=True,            # quiet if no tunnel is open
# OLD:     )

def close_tunnel(machine):
    subprocess.run(
        ["ssh", "-S", tunnel_socket(machine), "-O", "exit", "tunnel"],
        capture_output=True,            # quiet if no tunnel is open
    )


def get_ips():
    """{"lab-1": "13.48.x.x", ...} from Terraform's state."""
    outputs = json.loads(terraform("output", "-json"))
    if "public_ips" not in outputs:
        return {}
    return outputs["public_ips"]["value"]


def open_all_tunnels(ips):
    for index, (name, ip) in enumerate(ips.items()):
        open_tunnel(name, ip, index)


def setup_machine(name, ip):
    """Everything one server needs: Docker, its Domibus nodes, compose up."""
    nodes = nodes_on(name)
    print(f"\n===== {name}: {ip} ({', '.join(nodes) or 'no Domibus nodes'}) =====")
    wait_for_ssh(ip)
    install_docker(ip)
    if nodes:
        install_domibus(ip, nodes)
        start_domibus(ip, nodes)


def up():
    try:
        print("Starting server...")

        terraform("validate")
        terraform("apply", "-auto-approve")

        # OLD: outputs = json.loads(terraform("output", "-json"))
        # OLD: ips = outputs["public_ips"]["value"]
        ips = get_ips()

        print("Server(s) is up at:")
        for name in ips:
            print(f"{name}: {ips[name]}")

        # OLD: for name in ips:
        # OLD:     print(f"===== {name}: {ips[name]} =====")
        # OLD:     wait_for_ssh(ips[name])
        # OLD:     install_docker(ips[name])
        # OLD:     install_domibus(ips[name])
        # OLD:
        # OLD:     start_domibus(ips[name])
        # OLD:     open_tunnel(ips[name])
        for name in ips:
            setup_machine(name, ips[name])

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
        # Keep the server(s) so you can look at what went wrong
        # OLD: terraform("destroy", "-auto-approve")
        print("Server(s) left running for debugging. Run: python env.py down")


def down():
    print("Shutting down...")
    # OLD: close_tunnel()
    for machine in [m["name"] for m in CONFIG["infrastructure"]["machines"]]:
        close_tunnel(machine)
    terraform("destroy", "-auto-approve")

# OLD: def status():
# OLD:     print("Checking status...")

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


parser = argparse.ArgumentParser(description="Manage the Domibus test environment")
sub = parser.add_subparsers(dest="command", required=True)

sub.add_parser("up", help="Create the server and start Domibus")
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
