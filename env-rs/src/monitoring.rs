use std::collections::HashMap;

use crate::{config::Config, provisions::{compose_up, create_folders, upload, LAB}, remote::ssh};

pub const PROMETHEUS_CONFIG: &str = "grafana/prometheus.generated.yaml";

/// Prometheus scrapes node-exporter (9100), cadvisor (8088) and the JMX exporter (9404)
/// on every machine that runs Domibus, every 5 seconds.
pub fn write_prometheus_config(private_ips: &HashMap<String, String>, config: &Config) -> Result<(), String> {
    let mut node = String::new();
    let mut cadvisor = String::new();
    let mut jmx = String::new();

    for (name, ip) in private_ips {
        if config.nodes_on(name).is_empty() {
            continue; // skip the monitor machine
        }
        let target = |port: u16| format!("      - targets: [\"{ip}:{port}\"]\n        labels: {{instance: {name}}}\n");
        node.push_str(&target(9100));
        cadvisor.push_str(&target(8088));
        jmx.push_str(&target(9404));
    }

    let yaml = format!(
        "global:\n  scrape_interval: 5s\n\
         scrape_configs:\n\
         \x20 - job_name: node\n    static_configs:\n{node}\
         \x20 - job_name: cadvisor\n    static_configs:\n{cadvisor}\
         \x20 - job_name: jmx\n    static_configs:\n{jmx}"
    );
    std::fs::write(PROMETHEUS_CONFIG, yaml).map_err(|e| format!("could not write {PROMETHEUS_CONFIG}: {e}"))
}

pub fn start_monitoring(ip: &str) -> Result<(), String> {
    create_folders(ip, &[])?;
    upload(ip, "docker/compose.yaml", &format!("{LAB}/compose.yaml"))?;
    upload(ip, PROMETHEUS_CONFIG, &format!("{LAB}/prometheus.yaml"))?;
    upload(ip, "grafana/datasource.yaml", &format!("{LAB}/datasource.yaml"))?;
    upload(ip, "grafana/dashboards.yaml", &format!("{LAB}/dashboards.yaml"))?;
    upload_dashboards(ip)?;
    compose_up(ip, "monitor", &["prometheus", "grafana"])
}

/// Uploads every grafana/dashboards/*.json to /opt/lab/dashboards (Grafana loads them at start).
fn upload_dashboards(ip: &str) -> Result<(), String> {
    ssh(ip, &[&format!("mkdir -p {LAB}/dashboards")], None)?;
    let Ok(entries) = std::fs::read_dir("grafana/dashboards") else {
        return Ok(()); // no dashboards folder, nothing to upload
    };
    for entry in entries {
        let path = entry.map_err(|e| e.to_string())?.path();
        if path.extension().is_some_and(|ext| ext == "json") {
            let file = path.file_name().unwrap().to_string_lossy();
            upload(ip, &path.to_string_lossy(), &format!("{LAB}/dashboards/{file}"))?;
        }
    }
    Ok(())
}
