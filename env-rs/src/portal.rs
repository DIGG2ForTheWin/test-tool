use std::{collections::HashMap, fs, process::{Command, Stdio}};

use serde_json::json;

use crate::{config::Config, tunnels::terminal_port};

const LAB_JS: &str = "portal/lab.js";
const PORTAL: &str = "portal/portal.html";

pub fn write_portal(
    public_ips: &HashMap<String, String>,
    private_ips: &HashMap<String, String>,
    config: &Config,
) -> Result<(), String> {
    let mut names: Vec<&String> = public_ips.keys().collect();
    names.sort();

    let mut machines = Vec::new();
    for name in names {
        let mut links = Vec::new();
        for (node, n) in &config.nodes {
            if n.machine == *name {
                links.push(json!({
                    "label": format!("Domibus {node}"),
                    "url": format!("http://localhost:{}/domibus", n.port),
                }));
            }
        }
        if config.is_monitor(name) {
            links.push(json!({ "label": "Grafana", "url": "http://localhost:3000" }));
            links.push(json!({ "label": "Prometheus", "url": "http://localhost:9090" }));
        }

        // links.push(json!({
        //     "label": "Terminal",
        //     "url": format!("http://localhost:{}", terminal_port(name, public_ips)),
        // }));

        machines.push(json!({
            "name": name,
            "public_ip": public_ips[name],
            "private_ip": private_ips.get(name).cloned().unwrap_or_default(),
            "links": links,
        }));
    }

    let lab = serde_json::to_string_pretty(&json!({ "machines": machines }))
        .map_err(|e| format!("could not build lab.js: {e}"))?;
    fs::write(LAB_JS, format!("const LAB = {lab};\n"))
        .map_err(|e| format!("could not write {LAB_JS}: {e}"))
}

pub fn open_portal() -> Result<(), String> {
    let path = fs::canonicalize(PORTAL).map_err(|e| format!("could not find {PORTAL}: {e}"))?;
    let path = path.display().to_string().replace(r"\\?\", "");

    let mut cmd = if cfg!(target_os = "windows") {
        let mut c = Command::new("cmd");
        c.args(["/C", "start", ""]);
        c
    } else if cfg!(target_os = "macos") {
        Command::new("open")
    } else {
        Command::new("xdg-open")
    };

    cmd.arg(&path)
        .stdin(Stdio::null())
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .status()
        .map_err(|e| format!("could not open browser: {e}"))?;
    Ok(())
}
