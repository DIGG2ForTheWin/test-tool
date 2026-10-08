//! node.yaml: one per Domibus node, for the colleague's tool. Never committed (contains a password).

use std::{collections::HashMap, fs};

use serde::Deserialize;

use crate::{config::Config, provisions::LAB, remote::upload_text};

const CREDENTIALS: &str = "keys/domibus.yaml"; // gitignored, next to Key.pem

#[derive(Debug, Deserialize)]
pub struct Credentials {
    pub username: String,
    pub password: String,
}

impl Credentials {
    /// Reads keys/domibus.yaml. If it is missing, node.yaml gets CHANGE_ME instead.
    pub fn load() -> Result<Credentials, String> {
        match fs::read_to_string(CREDENTIALS) {
            Ok(text) => serde_yaml::from_str(&text).map_err(|e| format!("{CREDENTIALS}: {e}")),
            Err(_) => {
                println!("{CREDENTIALS} not found, node.yaml will say CHANGE_ME");
                Ok(Credentials { username: "CHANGE_ME".into(), password: "CHANGE_ME".into() })
            }
        }
    }
}

/// Domibus folder on the server for this node (used as data_root).
pub fn data_root(node: &str) -> String {
    format!("{LAB}/{node}/domibus")
}

/// Builds node.yaml for `node`, which runs on `machine`.
/// Nodes on the same machine use localhost, nodes on other machines use their private IP.
fn node_yaml(node: &str, machine: &str, config: &Config, private_ips: &HashMap<String, String>, creds: &Credentials) -> String {
    let mut yaml = format!(
        "# node.yaml – per machine, never commit (written by env-rs)\n\
         node: {node}\n\
         data_root: {}\n\
         nodes:\n",
        data_root(node)
    );

    for (name, other) in &config.nodes {
        let host = if other.machine == machine {
            "localhost".to_string()
        } else {
            private_ips.get(&other.machine).cloned().unwrap_or_else(|| "UNKNOWN".to_string())
        };
        yaml.push_str(&format!(
            "  {name}:\n    url: http://{host}:{}/domibus\n    username: {}\n    password: {}\n",
            other.port,
            quote(&creds.username),
            quote(&creds.password),
        ));
    }
    yaml
}

/// YAML single quotes: safe for any special characters; a ' inside is written as ''.
fn quote(value: &str) -> String {
    format!("'{}'", value.replace('\'', "''"))
}

/// Writes /opt/lab/<node>/node.yaml for every node on this machine.
pub fn write_node_files(ip: &str, machine: &str, config: &Config, private_ips: &HashMap<String, String>, creds: &Credentials) -> Result<(), String> {
    for node in config.nodes_on(machine) {
        let yaml = node_yaml(node, machine, config, private_ips, creds);
        upload_text(ip, &yaml, &format!("{LAB}/{node}/node.yaml"))?;
    }
    Ok(())
}
