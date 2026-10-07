"""Terraform: creating/destroying servers and reading their IPs."""
import json
import subprocess


def terraform(*args):
    result = subprocess.run(
        ["terraform", "-chdir=terraform" , *args],
        capture_output=True,
        text=True,
        check=True
    )
    return result.stdout


def get_ips():
    """{"lab-1": "13.48.x.x", ...} from Terraform's state."""
    outputs = json.loads(terraform("output", "-json"))
    if "public_ips" not in outputs:
        return {}
    return outputs["public_ips"]["value"]


def get_private_ips():
    """{"lab-1": "172.31.x.x", ...} - addresses the machines use to reach each other."""
    outputs = json.loads(terraform("output", "-json"))
    if "private_ips" not in outputs:
        return {}
    return outputs["private_ips"]["value"]
