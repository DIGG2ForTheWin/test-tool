#!/bin/bash
set -e

VERSION="5.2.1.3-JEE10"
URL="https://ec.europa.eu/digital-building-blocks/artifact/repository/eDelivery/eu/domibus/domibus-msh-distribution/${VERSION}/domibus-msh-distribution-${VERSION}-tomcat-full.zip"
LAB="/opt/lab"

# unzip is not installed on Ubuntu by default
sudo apt-get install -y -o DPkg::Lock::Timeout=300 unzip

# Download once
curl -fL -o /tmp/domibus.zip "$URL"

# Unpack a separate copy for each node
for NODE in blue red; do
  sudo rm -rf "$LAB/$NODE"
  sudo mkdir -p "$LAB/$NODE"
  sudo unzip -q /tmp/domibus.zip -d "$LAB/$NODE"
done

rm /tmp/domibus.zip

# Let the ubuntu user own the files, and make the Tomcat scripts runnable
sudo chown -R ubuntu:ubuntu "$LAB"
sudo find "$LAB" -name "*.sh" -path "*/bin/*" -exec chmod +x {} \;