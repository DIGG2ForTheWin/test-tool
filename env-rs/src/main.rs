use std::{env, thread};

use crate::{config::Config, infra::*, portal::{open_portal, write_portal}, provisions::setup_machine, tunnels::open_tunnels};

mod infra;
mod remote;
mod provisions;
mod config;
mod monitoring;
mod tunnels;
mod portal;
mod nodefile;
mod root;

fn up() -> Result<(), String> {
    
    let config = Config::load()?;

    println!("Starting...");

    apply_terraform()?;
    let public_ips = get_public_ips()?;
    let private_ips = get_private_ips()?;
    monitoring::write_prometheus_config(&private_ips, &config)?;
    
    let creds = nodefile::Credentials::load()?;

    let config = &config;
    let private_ips_ref = &private_ips;
    let creds = &creds;

    thread::scope(|s| {
        for (name, ip) in &public_ips {
            s.spawn(move || {
                if let Err(e) = setup_machine(name, ip, config, private_ips_ref, creds) {
                    eprintln!("{name} {e}");
                }
            });
        }
    });

    write_portal(&public_ips, &private_ips, config)?;
    open_portal()?;
    open_tunnels()?;

    Ok(())
}

fn tunnel() -> Result<(), String> {
    let config = Config::load()?;
    portal::write_portal(&get_public_ips()?, &get_private_ips()?, &config)?;
    portal::open_portal()?;
    open_tunnels()
    
}

fn down() -> Result<(), String> {
    println!("Destroying...");
    destroy_terraform()?;
    Ok(())
}


fn main() {
    let args: Vec<String> = env::args().skip(1).collect();
    let args: Vec<&str> = args.iter().map(String::as_str).collect();

    // All paths (config.yaml, keys/, terraform/ ...) are relative to the project folder
    let root = match root::find_root() {
        Ok(root) => root,
        Err(e) => {
            eprintln!("Error: {e}");
            std::process::exit(1);
        }
    };
    if let Err(e) = env::set_current_dir(&root) {
        eprintln!("Error: could not enter {}: {e}", root.display());
        std::process::exit(1);
    }

    let result = match args.as_slice() {
        ["up"] => up(),
        ["down"] => down(),
        ["tunnel"] => tunnel(),
        _ => {
            eprintln!("Usage: {} <up|down|tunnel>", env!("CARGO_PKG_NAME"));
            std::process::exit(1);
        }
    };

    if let Err(e) = result {
        eprintln!("Error: {e}");
        std::process::exit(1);
    }

}
