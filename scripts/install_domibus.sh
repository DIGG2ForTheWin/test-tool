#!/bin/bash
set -e

# Installs the Domibus nodes listed in NODES on this machine.
# env.py runs it as: sudo NODES='blue red' bash -s   (or just 'blue', or just 'red')
# Old lines are kept as comments, marked "# OLD:".

VERSION="5.2.1.3-JEE10"
URL="https://ec.europa.eu/digital-building-blocks/artifact/repository/eDelivery/eu/domibus/domibus-msh-distribution/${VERSION}/domibus-msh-distribution-${VERSION}-tomcat-full.zip"
LAB="/opt/lab"
NODES="${NODES:?NODES must be set, e.g. NODES='blue red'}"

# unzip is not installed on Ubuntu by default
# OLD: sudo apt-get install -y -o DPkg::Lock::Timeout=300 unzip
apt-get install -y -o DPkg::Lock::Timeout=300 unzip

# Download once
curl -fL -o /tmp/domibus.zip "$URL"

# Unpack a separate copy for each node
# OLD: for NODE in blue red; do
# OLD:   sudo rm -rf "$LAB/$NODE"
# OLD:   sudo mkdir -p "$LAB/$NODE"
# OLD:   sudo unzip -q /tmp/domibus.zip -d "$LAB/$NODE"
# OLD: done
for NODE in $NODES; do
  rm -rf "$LAB/$NODE"
  mkdir -p "$LAB/$NODE"
  unzip -q /tmp/domibus.zip -d "$LAB/$NODE"

  # Memory is set in compose.yaml, so remove Domibus's own 4 GB setting
  sed -i -E 's/ -Xms[0-9]+[mg]//; s/ -Xmx[0-9]+[mg]//' "$LAB/$NODE/domibus/bin/setenv.sh"

  # REST/WS access without login (test lab only) - was scripts/modify_docker.sh
  PROPS="$LAB/$NODE/domibus/conf/domibus/domibus.properties"
  sed -i '/domibus.auth.unsecureLoginAllowed/d' "$PROPS"
  echo "domibus.auth.unsecureLoginAllowed=true" >> "$PROPS"

  echo "Installed $NODE"
done

rm /tmp/domibus.zip

# Let the ubuntu user own the files, and make the Tomcat scripts runnable
# OLD: sudo chown -R ubuntu:ubuntu "$LAB"
# OLD: sudo find "$LAB" -name "*.sh" -path "*/bin/*" -exec chmod +x {} \;
chown -R ubuntu:ubuntu "$LAB"
find "$LAB" -name "*.sh" -path "*/bin/*" -exec chmod +x {} \;

# OLD: for NODE in blue red; do
# OLD:   sudo sed -i -E 's/ -Xms[0-9]+[mg]//; s/ -Xmx[0-9]+[mg]//' /opt/lab/$NODE/domibus/bin/setenv.sh
# OLD: done

# OLD: grep -n "JAVA_OPTS" /opt/lab/blue/domibus/bin/setenv.sh
for NODE in $NODES; do
  grep -n "JAVA_OPTS" "$LAB/$NODE/domibus/bin/setenv.sh"
done
