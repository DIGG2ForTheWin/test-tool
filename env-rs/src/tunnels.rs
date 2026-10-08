use std::thread;

use crate::{config::Config, infra::get_public_ips, remote::ssh};

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

            s.spawn(move || {
                let forwards: Vec<String> = ports.iter()
                    .map(|p| format!("{p}:localhost:{p}"))
                    .collect();

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