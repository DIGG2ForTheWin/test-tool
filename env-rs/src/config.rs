use std::{collections::BTreeMap};

use serde::Deserialize;


#[derive(Debug, Deserialize)]
pub struct Config {
    pub nodes: BTreeMap<String, Node>,
    pub monitoring: Option<Monitoring>,
}
#[derive(Debug, Deserialize)]
pub struct Node {
    pub machine: String,
    pub port: u16,
}

#[derive(Debug, Deserialize)]
pub struct Monitoring {
    pub machine: String,
}


impl Config {
    pub fn load() -> Result<Config, String> {
        let text = std::fs::read_to_string("config.yaml")
            .map_err(|e| format!("could not read config.yaml: {e}"))?;
        serde_yaml::from_str(&text).map_err(|e| format!("config.yaml: {e}"))

    }

    pub fn nodes_on(&self, machine: &str) -> Vec<&str> {
        self.nodes
            .iter()
            .filter(|(_, node)| node.machine == machine)
            .map(|(name, _)| name.as_str())
            .collect()
    }

    pub fn is_monitor(&self, machine: &str) -> bool {
    self.monitoring.as_ref().is_some_and(|m| m.machine == machine)
}
}
