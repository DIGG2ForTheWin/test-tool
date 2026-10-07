"""SSH tunnels from this computer to the servers (works on Mac, Linux and Windows)."""
import os
import signal
import subprocess
import time
from pathlib import Path

from testtool.config import MONITOR_MACHINE, MONITORING_PORTS, NODES, nodes_on
from testtool.remote import KEY


def pid_file(machine):
    """Remembers which ssh process is the tunnel for this machine."""
    return Path(f".tunnel-{machine}.pid")


def tunnel_forwards(machine):          # unchanged
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
    args = []
    for local, remote in tunnel_forwards(machine).items():
        args += ["-L", f"{local}:localhost:{remote}"]

    if os.name == "nt":   # Windows: run without a console window, separate from env.py
        extra = {"creationflags": subprocess.CREATE_NEW_PROCESS_GROUP | subprocess.DETACHED_PROCESS}
    else:                 # Mac/Linux: own session, so closing the terminal doesn't kill it
        extra = {"start_new_session": True}

    process = subprocess.Popen(
        ["ssh", "-i", KEY, "-N",
         "-o", "StrictHostKeyChecking=no",
         "-o", f"UserKnownHostsFile={os.devnull}",
         "-o", "BatchMode=yes",
         "-o", "LogLevel=ERROR",
         "-o", "ExitOnForwardFailure=yes",
         "-o", "ServerAliveInterval=30",
         *args, f"ubuntu@{ip}"],
        stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        **extra,
    )

    time.sleep(2)   # give ssh a moment; if it already exited, the tunnel failed
    if process.poll() is not None:
        raise RuntimeError(f"Tunnel to {machine} failed (port already in use, or SSH refused)")
    pid_file(machine).write_text(str(process.pid))

    print(f"Tunnel open to {machine} ({ip}):")
    for node in nodes_on(machine):
        print(f"  {node:<11} http://localhost:{NODES[node]['port']}/domibus")
    if machine == MONITOR_MACHINE:
        print(f"  {'Grafana':<11} http://localhost:{MONITORING_PORTS['grafana']}")
        print(f"  {'Prometheus':<11} http://localhost:{MONITORING_PORTS['prometheus']}")


def close_tunnel(machine):
    f = pid_file(machine)
    if not f.exists():
        return
    try:
        os.kill(int(f.read_text()), signal.SIGTERM)
    except (OSError, ValueError):
        pass                                          
    f.unlink()


def open_all_tunnels(ips):
    for name, ip in ips.items():
        open_tunnel(name, ip)