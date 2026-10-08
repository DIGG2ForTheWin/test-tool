use std::{env, path::PathBuf};

pub fn find_root() -> Result<PathBuf, String> {
    if let Ok(home) = env::var("DSET_HOME") {
        let home = PathBuf::from(home);
        if home.join("config.yaml").exists() {
            return Ok(home);
        }
        return Err(format!("DSET_HOME is set to {}, but there is no config.yaml there", home.display()));
    }

    if let Ok(cwd) = env::current_dir() {
        if let Some(root) = search_up(cwd) {
            return Ok(root);
        }
    }

    if let Ok(exe) = env::current_exe() {
        if let Some(root) = search_up(exe) {
            return Ok(root);
        }
    }

    Err("could not find config.yaml. Run inside the project folder, or set DSET_HOME to it".into())
}

fn search_up(mut dir: PathBuf) -> Option<PathBuf> {
    loop {
        if dir.join("config.yaml").exists() {
            return Some(dir);
        }
        if !dir.pop() {
            return None;
        }
    }
}
