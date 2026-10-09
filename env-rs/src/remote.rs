use std::{fs::File, io::Write, path::Path, process::{Command, Stdio}, thread::sleep, time::Duration};

pub fn ssh_base(ip: &str) -> Command {
    let key_path: &str = "keys/Key.pem";
    let known_hosts = if cfg!(windows) { "NUL"} else { "/dev/null" };
    
    let mut cmd: Command = Command::new("ssh");
        cmd.args([
            "-i", key_path,
            "-o", "StrictHostKeyChecking=no",
            "-o", &format!("UserKnownHostsFile={}", known_hosts),
            "-o", "ConnectTimeout=5",
            "-o", "BatchMode=yes",
            "-o", "LogLevel=ERROR",
            &format!("ubuntu@{}", ip)
        ]);
        cmd
}


pub fn ssh(ip: &str, args: &[&str], script: Option<&Path>) -> Result<String, String> {
    let stdin = match script {
        Some(path) => {
            let file = File::open(path)
                .map_err(|e| format!("Could not open {}: {e}", path.display()))?;
            Stdio::from(file)
        }
        None => Stdio::null(),
    };

    let output = ssh_base(ip)
        .args(args)
        .stdin(stdin)
        .output()
        .map_err(|e| format!("could not start ssh: {e}"))?; 

    if !output.status.success() {
        return Err(format!(
            "ssh {ip} `{:?}` failed (exit {:?}): {}",
            args,
            output.status.code(),
            String::from_utf8_lossy(&output.stderr).trim()
        ));
    }
    Ok(String::from_utf8_lossy(&output.stdout).into_owned())

}

pub fn wait_for_ssh(ip: &str) -> Result<(), String> {
    print!("Waiting for SSH on {ip}");
    for _ in 0..36 {
        if ssh(ip, &["true"], None).is_ok() {
            println!(" ready");
            return Ok(());
        }
        sleep(Duration::from_secs(5));
    }

    Err(format!("{ip}: no SSH response"))
}

/// Writes `text` to a file on the server without a local copy (used for files with passwords).
/// The file is readable only by the ubuntu user (chmod 600).
pub fn upload_text(ip: &str, text: &str, remote: &str) -> Result<(), String> {
    let mut child = ssh_base(ip)
        .arg(format!("umask 077 && cat > {remote}"))
        .stdin(Stdio::piped())
        .stdout(Stdio::null())
        .stderr(Stdio::piped())
        .spawn()
        .map_err(|e| format!("could not start ssh: {e}"))?;

    // take() moves stdin out of child; when it is dropped at the end of this line, ssh sees end of file
    child.stdin.take().unwrap().write_all(text.as_bytes())
        .map_err(|e| format!("could not send {remote}: {e}"))?;

    let output = child.wait_with_output().map_err(|e| format!("ssh failed: {e}"))?;
    if !output.status.success() {
        return Err(format!("could not write {remote} on {ip}: {}", String::from_utf8_lossy(&output.stderr).trim()));
    }
    Ok(())
}
