"""Prometheus + Grafana on the monitor machine."""
import json
import time
from pathlib import Path
import yaml

from testtool.config import JMX_EXPORTER_PORT, MONITOR, PROMETHEUS_CONFIG, nodes_on
from testtool.remote import ssh


def write_prometheus_config(private_ips):
    """Prometheus on the monitor machine scrapes the exporters on every Domibus machine."""
    node_targets = []
    cadvisor_targets = []
    jmx_targets = []
    for name, ip in private_ips.items():
        if nodes_on(name):
            # OLD: node_targets.append({"targets": [f"{ip}:9100"], "labels": {"machine": name}})
            # OLD: cadvisor_targets.append({"targets": [f"{ip}:8080"], "labels": {"machine": name}})
            node_targets.append({"targets": [f"{ip}:9100"], "labels": {"instance": name}})
            cadvisor_targets.append({"targets": [f"{ip}:8080"], "labels": {"instance": name}})
            jmx_targets.append({"targets": [f"{ip}:{JMX_EXPORTER_PORT}"], "labels": {"instance": name}})
    config = {
    "global": {
        "scrape_interval": "5s",
        "evaluation_interval": "5s",
    },
    "scrape_configs": [
        {"job_name": "node", "static_configs": node_targets},
        {"job_name": "cadvisor", "static_configs": cadvisor_targets},
        {"job_name": "jmx", "static_configs": jmx_targets},
    ],}
    with open(PROMETHEUS_CONFIG, "w") as f:
        yaml.safe_dump(config, f, sort_keys=False)

def wait_for_grafana(ip, timeout=120):
    """Wait until Grafana is ready to accept dashboard API requests."""
    print("Waiting for Grafana", end="", flush=True)

    deadline = time.time() + timeout

    while time.time() < deadline:
        result = ssh(
            ip,
            "curl -fsS http://localhost:3000/api/health >/dev/null 2>&1 "
            "&& echo ready || true",
        ).strip()

        if result == "ready":
            print(" ready")
            return

        print(".", end="", flush=True)
        time.sleep(2)

    raise TimeoutError("Grafana did not become ready")


def provision_dashboards(ip):
    """Create or update all Grafana V2 dashboards stored in grafana/dashboards."""
    dashboard_dir = Path("grafana/dashboards")

    if not dashboard_dir.exists():
        return

    base_url = (
        "http://localhost:3000"
        "/apis/dashboard.grafana.app/v2"
        "/namespaces/default/dashboards"
    )

    for dashboard_path in sorted(dashboard_dir.glob("*.json")):
        with open(dashboard_path, encoding="utf-8") as f:
            dashboard = json.load(f)

        dashboard_name = dashboard.get("metadata", {}).get("name")

        if not dashboard_name:
            raise ValueError(
                f"{dashboard_path} has no metadata.name. "
                "Provisioned V2 dashboards require a stable identifier."
            )

        status = ssh(
            ip,
            "curl -s "
            "-u admin:admin "
            "-o /dev/null "
            "-w '%{http_code}' "
            f"'{base_url}/{dashboard_name}'",
        ).strip()

        if status == "200":
            method = "PUT"
            url = f"{base_url}/{dashboard_name}"
            action = "Updating"
        elif status == "404":
            method = "POST"
            url = base_url
            action = "Creating"
        else:
            raise RuntimeError(
                f"Unexpected Grafana response for {dashboard_name}: HTTP {status}"
            )

        print(f"{action} Grafana dashboard: {dashboard_name}")

        ssh(
            ip,
            "curl -fsS "
            "-u admin:admin "
            "-H 'Content-Type: application/json' "
            f"-X {method} "
            f"'{url}' "
            "--data-binary @-",
            str(dashboard_path),
        )

def start_monitoring(ip):
    print("Starting Prometheus and Grafana...")

    ssh(
        ip,
        "sudo mkdir -p /opt/lab "
        "&& sudo chown ubuntu:ubuntu /opt/lab"
    )

    ssh(ip, "cat > /opt/lab/compose.yaml", "docker/compose.yaml")
    ssh(ip, "cat > /opt/lab/prometheus.yaml", PROMETHEUS_CONFIG)
    ssh(ip, "cat > /opt/lab/datasource.yaml", "grafana/datasource.yaml")

    ssh(
        ip,
        f"cd /opt/lab && sudo docker compose up -d --quiet-pull "
        f"{' '.join(MONITOR)}",
    )

    wait_for_grafana(ip)
    provision_dashboards(ip)
