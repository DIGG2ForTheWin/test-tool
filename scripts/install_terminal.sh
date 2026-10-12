#!/bin/bash
# Web terminal (ttyd) on port 7681, bound to localhost only.
# Reached from your computer through the SSH tunnel, so Key.pem is the only way in.
set -e

export DEBIAN_FRONTEND=noninteractive
apt-get install -y -qq ttyd

tee /etc/systemd/system/ttyd.service > /dev/null <<'UNIT'
[Unit]
Description=ttyd web terminal
After=network.target

[Service]
User=ubuntu
WorkingDirectory=/home/ubuntu
# -W writable (read-only by default), -i lo = only localhost, bash -l = normal login shell
ExecStart=/usr/bin/ttyd -W -i lo -p 7681 bash -l
Restart=always

[Install]
WantedBy=multi-user.target
UNIT

systemctl daemon-reload
systemctl enable --now ttyd
systemctl restart ttyd
