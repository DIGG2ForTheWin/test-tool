"""Prometheus + Grafana on the monitor machine."""
import yaml

from testtool.config import MONITOR, PROMETHEUS_CONFIG, nodes_on
from testtool.remote import ssh


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
    config = {
    "global": {
        "scrape_interval": "5s",
        "evaluation_interval": "5s",
    },
    "scrape_configs": [
        {"job_name": "node", "static_configs": node_targets},
        {"job_name": "cadvisor", "static_configs": cadvisor_targets},
    ],}
    with open(PROMETHEUS_CONFIG, "w") as f:
        yaml.safe_dump(config, f, sort_keys=False)


def start_monitoring(ip):
    print("Starting Prometheus and Grafana...")
    ssh(ip, "sudo mkdir -p /opt/lab && sudo chown ubuntu:ubuntu /opt/lab")
    ssh(ip, "cat > /opt/lab/compose.yaml", "docker/compose.yaml")
    ssh(ip, "cat > /opt/lab/prometheus.yaml", PROMETHEUS_CONFIG)
    ssh(ip, "cat > /opt/lab/datasource.yaml", "grafana/datasource.yaml")
    ssh(ip, f"cd /opt/lab && sudo docker compose up -d --quiet-pull {' '.join(MONITOR)}")
