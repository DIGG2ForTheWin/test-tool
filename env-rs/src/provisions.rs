use std::{collections::HashMap, path::Path};

use crate::{config::Config, monitoring::start_monitoring, nodefile::{data_root, write_node_files, Credentials}, remote::{ssh, wait_for_ssh}};

pub const LAB: &str = "/opt/lab";

pub fn create_folders(ip: &str, nodes: &[&str]) -> Result<(), String> {
    let mut folders = vec![LAB.to_string()];
    for node in nodes {
        folders.push(format!("{LAB}/{node}/fs"));
        folders.push(data_root(node));
    }
    let cmd = format!("sudo mkdir -p {} && sudo chown -R ubuntu:ubuntu {LAB}", folders.join(" "));
    ssh(ip, &[&cmd], None)?;
    Ok(())
}

pub fn upload(ip: &str, local: &str, remote: &str) -> Result<(), String> {
    ssh(ip, &[&format!("cat > {remote}")], Some(Path::new(local)))?;
    Ok(())
}

pub fn compose_up(ip: &str, machine: &str, services: &[&str]) -> Result<(), String> {
    let cmd = format!("cd {LAB} && sudo MACHINE={machine} docker compose up -d --quiet-pull {}", services.join(" "));
    ssh(ip, &[&cmd], None)?;
    Ok(())
}

pub fn _install_docker(ip: &str) -> Result<(), String> {
    let docker_script = Some(Path::new("scripts/install_docker.sh"));
    ssh(ip, &["sudo bash -s"], docker_script)?;
    Ok(())
}

pub fn _install_terminal(ip: &str) -> Result<(), String> {
    let script = Some(Path::new("scripts/install_terminal.sh"));
    ssh(ip, &["sudo bash -s"], script)?;
    Ok(())
}

pub fn wait_for_boot_script(ip: &str) -> Result<(), String> {
    ssh(ip, &["cloud-init status --wait > /dev/null"], None)?;
    Ok(())
}


pub fn setup_machine(name: &str, ip: &str, config: &Config, private_ips: &HashMap<String, String>, creds: &Credentials) -> Result<(), String> {
    println!("{name}, waiting for SSH on {ip}");
    wait_for_ssh(ip)?;

    // println!("{name} installing Docker");
    // install_docker(ip)?;

    wait_for_boot_script(ip)?;

    let version = ssh(ip, &["docker --version && docker compose version"], None)?;
    println!("[{name}] {}", version.trim());

    let nodes = config.nodes_on(name);
    if !nodes.is_empty() {
        println!("[{name}] creating folders");
        create_folders(ip, &nodes)?;
        write_node_files(ip, name, config, private_ips, creds)?;
        upload(ip, "docker/compose.yaml", &format!("{LAB}/compose.yaml"))?;

        let mut services = nodes.clone();
        services.extend(["cadvisor", "node-exporter"]);
        println!("[{name}] starting {}", services.join(", "));
        compose_up(ip, name, &services)?;
    }

    if config.is_monitor(name) {
        println!("[{name}] starting Prometheus and Grafana");
        start_monitoring(ip)?;
    }

    // println!("[{name}] installing web terminal");
    // install_terminal(ip)?;

    println!("{name} done");
    Ok(())
}