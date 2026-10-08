use std::collections::HashMap;

use crate::{config::Config, provisions::{compose_up, create_folders, upload, LAB}};

pub const PROMETHEUS_CONFIG: &str = "grafana/prometheus.generated.yaml";

/// Prometheus scrapes node-exporter (9100) and cadvisor (8080) on every machine that runs Domibus.
pub fn write_prometheus_config(private_ips: &HashMap<String, String>, config: &Config) -> Result<(), String> {
    let mut node = String::new();
    let mut cadvisor = String::new();

    for (name, ip) in private_ips {
        if config.nodes_on(name).is_empty() {
            continue; // skip the monitor machine
        }
        node.push_str(&format!("      - targets: [\"{ip}:9100\"]\n        labels: {{instance: {name}}}\n"));
        cadvisor.push_str(&format!("      - targets: [\"{ip}:8080\"]\n        labels: {{instance: {name}}}\n"));
    }

    let yaml = format!(
        "scrape_configs:\n  - job_name: node\n    static_configs:\n{node}  - job_name: cadvisor\n    static_configs:\n{cadvisor}"
    );
    std::fs::write(PROMETHEUS_CONFIG, yaml).map_err(|e| format!("could not write {PROMETHEUS_CONFIG}: {e}"))
}

pub fn start_monitoring(ip: &str) -> Result<(), String> {
    create_folders(ip, &[])?;
    upload(ip, "docker/compose.yaml", &format!("{LAB}/compose.yaml"))?;
    upload(ip, PROMETHEUS_CONFIG, &format!("{LAB}/prometheus.yaml"))?;
    upload(ip, "grafana/datasource.yaml", &format!("{LAB}/datasource.yaml"))?;
    compose_up(ip, "monitor", &["prometheus", "grafana"])
}