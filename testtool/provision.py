"""Setting up one server: Docker, Domibus containers, monitoring."""
import time

from testtool.config import EXPORTERS, MONITOR_MACHINE, NODES, nodes_on
from testtool.monitoring import start_monitoring
from testtool.remote import ssh, wait_for_ssh


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


def install_jmx_exporter(ip):
    """Stage the JMX Exporter agent and its config in /opt/lab/jmx (mounted into Domibus)."""
    print("staging JMX exporter...")
    print(ssh(ip, "sudo bash -s", "scripts/install_jmx_exporter.sh").strip())
    ssh(ip, "cat > /opt/lab/jmx/config.yaml", "grafana/jmx-exporter.yaml")


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
    # /opt/lab used to be created by install_domibus.sh
    ssh(ip, "sudo mkdir -p /opt/lab && sudo chown ubuntu:ubuntu /opt/lab")
    ssh(ip, "cat > /opt/lab/compose.yaml", "docker/compose.yaml")
    install_jmx_exporter(ip)   # must be there before the Domibus JVM starts

    services = " ".join(nodes + EXPORTERS)
    ssh(ip, f"cd /opt/lab && sudo MACHINE={name} docker compose up -d --quiet-pull {services}")
    for node in nodes:
        wait_for_domibus(ip, NODES[node]["port"], node)


def setup_machine(name, ip):
    """Everything one server needs: Docker, its Domibus nodes, compose up."""
    nodes = nodes_on(name)
    print(f"\n===== {name}: {ip} ({', '.join(nodes) or 'no Domibus nodes'}) =====")
    wait_for_ssh(ip)
    install_docker(ip)
    if nodes:
        # OLD: install_domibus(ip, nodes)   # Domibus is now inside the Docker image
        # OLD: start_domibus(ip, nodes)
        start_domibus(name, ip, nodes)
    if name == MONITOR_MACHINE:
        start_monitoring(ip)
