use std::{collections::HashMap, path::Path, process::{Command, Stdio}};


fn terraform_base() -> Command {
    let mut cmd = Command::new("terraform");
    cmd.current_dir("terraform")
        .stdin(Stdio::null());
    
    cmd
} 

fn terraform(args: &[&str]) -> Result<String, String> {
    let output = terraform_base()
        .args(args)
        .output()
        .map_err(|e| format!("could not start terraform: {e}"))?;

    if !output.status.success() {
        return Err(format!(
            "terraform {} failed: {}",
            args.join(" "),
            String::from_utf8_lossy(&output.stderr).trim()
        ));
    }
    Ok(String::from_utf8_lossy(&output.stdout).into_owned())
}

fn ensure_init() -> Result<(), String> {
    if !Path::new("terraform/.terraform").exists() {
        println!("Running terraform init (first time only)...");
        terraform(&["init", "-input=false"])?;
    }
    Ok(())
}

pub fn destroy_terraform() -> Result<String, String> {
    ensure_init()?;
    terraform(&["destroy", "--auto-approve"])
}

pub fn apply_terraform() -> Result<String, String> {
    ensure_init()?;
    terraform(&["apply", "--auto-approve"])
}

fn get_ips(output: &str) -> Result<HashMap<String, String>, String> {
    let json = terraform(&["output", "-json", output])?;
    serde_json::from_str(&json).map_err(|e| format!("could not read {output} from terraform: {e}"))
}

pub fn get_public_ips() -> Result<HashMap<String, String>, String> { get_ips("public_ips") }
pub fn get_private_ips() -> Result<HashMap<String, String>, String> { get_ips("private_ips") }

// pub fn plan_terraform() {
//     terraform("plan");
// }