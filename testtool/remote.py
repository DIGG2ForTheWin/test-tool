import os
import subprocess
import time

KEY = "Key.pem"

def ssh_command(ip):
    return [
        "ssh",
        "-i", KEY,
        "-o", "StrictHostKeyChecking=no",
        "-o", f"UserKnownHostsFile={os.devnull}",   
        "-o", "ConnectTimeout=5",
        "-o", "BatchMode=yes",                     
        "-o", "LogLevel=ERROR",
        f"ubuntu@{ip}",
    ]

def ssh(ip, command, input_file=None):
    if input_file:
        with open(input_file) as f:
            result = subprocess.run(ssh_command(ip) + [command], stdin=f,
                                    capture_output=True, text=True, check=True)
    else:
        result = subprocess.run(ssh_command(ip) + [command],
                                stdin=subprocess.DEVNULL,          
                                capture_output=True, text=True, check=True)
    return result.stdout


def wait_for_ssh(ip, timeout=180):
    print(f"Waiting for SSH on {ip}", end="", flush=True)
    deadline = time.time() + timeout
    last_error = ""
    while time.time() < deadline:
        try:
            ssh(ip, "true")
            print(" ready")
            return
        except subprocess.CalledProcessError as error:
            last_error = (error.stderr or "").strip()
            print(".", end="", flush=True)
            time.sleep(5)
    raise TimeoutError(f"No SSH connection to {ip} after {timeout}s: {last_error}")
