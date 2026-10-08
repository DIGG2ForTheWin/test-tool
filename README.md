# dset – Domibus test environment

`dset` creates AWS machines with Terraform, starts Domibus (blue/red) and monitoring
(Prometheus + Grafana) with Docker, opens SSH tunnels and a web portal.

## Requirements

- [Terraform](https://developer.hashicorp.com/terraform/install)
- AWS credentials (`aws configure`, or `AWS_ACCESS_KEY_ID` / `AWS_SECRET_ACCESS_KEY`)
- `ssh` (built into Mac, Linux and Windows 10+)

## First time

1. Download the zip for your OS from **Releases** and unpack it.
2. Edit `config.yaml`: set `user:` to your name (used in AWS resource names).
3. Create your own SSH key in `keys/`:
   ```bash
   ssh-keygen -t ed25519 -f keys/Key -N ""
   mv keys/Key keys/Key.pem
   ```
4. Optional: `keys/domibus.yaml` with the Domibus plugin user (written into `node.yaml` on each gateway):
   ```yaml
   username: your_plugin_user
   password: 'your password'
   ```
5. Mac only, if it says the developer cannot be verified: `xattr -d com.apple.quarantine dset`

## Usage

```bash
./dset up       # create machines, install everything, open portal + tunnels (Ctrl+C closes tunnels)
./dset tunnel   # reopen portal + tunnels for machines that are already up
./dset down     # destroy all machines
```

`dset` finds its folder by looking for `config.yaml` (in `DSET_HOME`, the current folder and its parents,
or next to the binary), so you can also put it on your PATH and set `DSET_HOME` to this folder.

## Building from source

```bash
cd env-rs
cargo install --path .      # installs dset to ~/.cargo/bin
```

## Releasing

```bash
git tag v0.1.0
git push --tags             # GitHub Actions builds the zips and attaches them to a Release
```
