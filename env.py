import argparse
import subprocess
import time
import json
import os
import yaml
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


os.chdir(Path(__file__).resolve().parent)

KEY = "Key.pem"

with open("tests.yaml") as f:
    CONFIG = yaml.safe_load(f)

NODES = CONFIG["nodes"]                                   
EXPORTERS = ["cadvisor", "node-exporter"] 
MONITOR = ["prometheus", "grafana"]          
MONITORING_PORTS = {"grafana": 3000, "prometheus": 9090}
MONITOR_MACHINE = CONFIG.get("monitoring", {}).get("machine")   # e.g. "monitor"
PROMETHEUS_CONFIG = "grafana/prometheus.generated.yaml"   # written by env.py on every up


def nodes_on(machine):
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

def ssh(ip, command, input_file=None):
    if input_file:
        with open(input_file) as f:            
            result = subprocess.run(ssh_command(ip) + [command], stdin=f,
                                    capture_output=True, text=True, check=True)
    else:
        result = subprocess.run(ssh_command(ip) + [command],
                                capture_output=True, text=True, check=True)
    return result.stdout

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

def install_domibus(ip, nodes):
    print(f"installing Domibus: {', '.join(nodes)}...")
    ssh(ip, f"sudo NODES='{' '.join(nodes)}' bash -s", "scripts/install_domibus.sh")
    print("Domibus installed")
    print(ssh(ip, "ls /opt/lab").strip())

def wait_for_domibus(ip, port, name, timeout=600):
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

def start_domibus(name, ip, nodes):
    print(f"Starting {', '.join(nodes)}...")
    ssh(ip, "cat > /opt/lab/compose.yaml", "docker/compose.yaml")

    services = " ".join(nodes + EXPORTERS)
    ssh(ip, f"cd /opt/lab && sudo MACHINE={name} docker compose up -d --quiet-pull {services}")
    for node in nodes:
        wait_for_domibus(ip, NODES[node]["port"], node)

def write_prometheus_config(private_ips):
    """Prometheus on the monitor machine scrapes the exporters on every Domibus machine."""
    node_targets = []
    cadvisor_targets = []
    for name, ip in private_ips.items():
        if nodes_on(name):
            # OLD: node_targets.append({"targets": [f"{ip}:9100"], "labels": {"machine": name}})
            # OLD: cadvisor_targets.append({"targets": [f"{ip}:8080"], "labels": {"machine": name}})
            node_targets.append({"targets": [f"{ip}:9100"], "labels": {"instance": name}})
            cadvisor_targets.append({"targets": [f"{ip}:8080"], "labels": {"instance": name}})
    config = {"scrape_configs": [
        {"job_name": "node", "static_configs": node_targets},
        {"job_name": "cadvisor", "static_configs": cadvisor_targets},
    ]}
    with open(PROMETHEUS_CONFIG, "w") as f:
        yaml.safe_dump(config, f, sort_keys=False)

def start_monitoring(ip):
    print("Starting Prometheus and Grafana...")
    ssh(ip, "sudo mkdir -p /opt/lab && sudo chown ubuntu:ubuntu /opt/lab")
    ssh(ip, "cat > /opt/lab/compose.yaml", "docker/compose.yaml")
    ssh(ip, "cat > /opt/lab/prometheus.yaml", PROMETHEUS_CONFIG)
    ssh(ip, "cat > /opt/lab/datasource.yaml", "grafana/datasource.yaml")
    ssh(ip, f"cd /opt/lab && sudo docker compose up -d --quiet-pull {' '.join(MONITOR)}")

def tunnel_socket(machine):
    """One control socket per machine, so several tunnels can be open at once."""
    return f".tunnel-{machine}.sock"


def tunnel_forwards(machine):

    forwards = {}
    for node in nodes_on(machine):
        port = NODES[node]["port"]
        forwards[port] = port
    if machine == MONITOR_MACHINE:
        for service, port in MONITORING_PORTS.items():
            forwards[port] = port
    return forwards

def open_tunnel(machine, ip):
    close_tunnel(machine)
    forwards = tunnel_forwards(machine)
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
    if machine == MONITOR_MACHINE:
        print(f"  {'Grafana':<11} http://localhost:{MONITORING_PORTS['grafana']}")
        print(f"  {'Prometheus':<11} http://localhost:{MONITORING_PORTS['prometheus']}")

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


def get_private_ips():
    """{"lab-1": "172.31.x.x", ...} - addresses the machines use to reach each other."""
    outputs = json.loads(terraform("output", "-json"))
    if "private_ips" not in outputs:
        return {}
    return outputs["private_ips"]["value"]


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
        # OLD: start_domibus(ip, nodes)
        start_domibus(name, ip, nodes)
    if name == MONITOR_MACHINE:
        start_monitoring(ip)


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
