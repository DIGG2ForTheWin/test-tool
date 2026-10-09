use std::{collections::HashMap, thread};

use crate::{config::Config, infra::get_public_ips, remote::ssh};

pub const TERMINAL_PORT: u16 = 7681;

pub fn terminal_port(machine: &str, ips: &HashMap<String, String>) -> u16 {
    let mut names: Vec<&String> = ips.keys().collect();
    names.sort();
    let index = names.iter().position(|n| n.as_str() == machine).unwrap_or(0);
    TERMINAL_PORT + index as u16
}

pub fn open_tunnels() -> Result<(), String> {
    let config = Config::load()?;
    let ips = get_public_ips()?;

    thread::scope(|s| {
        for (name, ip) in &ips {
            let mut ports: Vec<u16> = config.nodes.values()
                .filter(|n| n.machine == *name)
                .map(|n| n.port)
                .collect();
            if config.is_monitor(name) {
                ports.extend([3000, 9090]);
            }

            let terminal = terminal_port(name, &ips);

            s.spawn(move || {
                let mut forwards: Vec<String> = ports.iter()
                    .map(|p| format!("{p}:localhost:{p}"))
                    .collect();
                // ttyd only listens on IPv4 localhost, so 127.0.0.1 instead of localhost
                forwards.push(format!("{terminal}:127.0.0.1:{TERMINAL_PORT}"));

                let mut args = vec!["-N", "-o", "ExitOnForwardFailure=yes"];
                for f in &forwards {
                    args.extend(["-L", f.as_str()]);
                }

                if let Err(e) = ssh(ip, &args, None) {
                    eprintln!("{name}: {e}");
                }
            });
        }
        println!("Tunnels open. Ctrl+C to close.");
    });
    Ok(())
}