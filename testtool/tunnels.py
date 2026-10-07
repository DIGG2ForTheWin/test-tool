"""SSH tunnels from this computer to the servers."""
import subprocess

from testtool.config import MONITOR_MACHINE, MONITORING_PORTS, NODES, nodes_on
from testtool.remote import KEY


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


def open_all_tunnels(ips):
    for index, (name, ip) in enumerate(ips.items()):
        open_tunnel(name, ip)
