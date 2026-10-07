"""Reads tests.yaml. Everything else imports its settings from here."""
import os
from pathlib import Path

import yaml

# Project root (the folder with tests.yaml), so relative paths like "Key.pem" work
ROOT = Path(__file__).resolve().parent.parent
os.chdir(ROOT)

with open("tests.yaml") as f:
    CONFIG = yaml.safe_load(f)

NODES = CONFIG["nodes"]
EXPORTERS = ["cadvisor", "node-exporter"]
MONITOR = ["prometheus", "grafana"]
MONITORING_PORTS = {"grafana": 3000, "prometheus": 9090}
JMX_EXPORTER_PORT = 9404   # Java agent inside each Domibus JVM (see docker/compose.yaml)
MONITOR_MACHINE = CONFIG.get("monitoring", {}).get("machine")   # e.g. "monitor"
PROMETHEUS_CONFIG = "grafana/prometheus.generated.yaml"   # written on every up


def nodes_on(machine):
    return [node for node, settings in NODES.items() if settings["machine"] == machine]
