#!/bin/bash
set -e

# Installs the Domibus nodes listed in NODES on this machine.
# env.py runs it as: sudo NODES='blue red' bash -s   (or just 'blue', or just 'red')
# Old lines are kept as comments, marked "# OLD:".

VERSION="5.2.1.3-JEE10"
URL="https://ec.europa.eu/digital-building-blocks/artifact/repository/eDelivery/eu/domibus/domibus-msh-distribution/${VERSION}/domibus-msh-distribution-${VERSION}-tomcat-full.zip"
# OLD: FS_URL="https://ec.europa.eu/digital-building-blocks/artifact/repository/eDelivery/eu/domibus/domibus-msh-distribution/${VERSION}-JEE10/domibus-msh-distribution-${VERSION}-JEE10-default-fs-plugin.zip"
# Patch releases (5.2.1.3) have no plugin zip of their own, so the plugin comes from the 5.2 release
FS_VERSION="5.2-JEE10"
FS_URL="https://ec.europa.eu/digital-building-blocks/artifact/repository/eDelivery/eu/domibus/domibus-msh-distribution/${FS_VERSION}/domibus-msh-distribution-${FS_VERSION}-default-fs-plugin.zip"
LAB="/opt/lab"
NODES="${NODES:?NODES must be set, e.g. NODES='blue red'}"

# unzip is not installed on Ubuntu by default
# OLD: sudo apt-get install -y -o DPkg::Lock::Timeout=300 unzip
apt-get install -y -o DPkg::Lock::Timeout=300 unzip

# Download once
# -sS: no progress bar, but still show errors
curl -fsSL -o /tmp/domibus.zip "$URL"
curl -fsSL -o /tmp/fs-plugin.zip "$FS_URL"
rm -rf /tmp/fs-plugin
unzip -q /tmp/fs-plugin.zip -d /tmp/fs-plugin

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

  # FS plugin: jar -> plugins/lib, properties -> plugins/config
  # (find is used so it works whatever folder layout the zip has)
  PLUGINS="$LAB/$NODE/domibus/conf/domibus/plugins"
  mkdir -p "$PLUGINS/lib" "$PLUGINS/config"
  find /tmp/fs-plugin -name "*.jar" -exec cp {} "$PLUGINS/lib/" \;
  find /tmp/fs-plugin -name "*.properties" -exec cp {} "$PLUGINS/config/" \;

  # Where the FS plugin watches for files (OUT/IN/SENT/FAILED are created inside).
  # /opt/domibus in the container = /opt/lab/$NODE/domibus on the server
  FS_PROPS=$(ls "$PLUGINS"/config/*fs*plugin*.properties | head -1)
  sed -i '/^fsplugin.messages.location/d' "$FS_PROPS"
  echo "fsplugin.messages.location=/opt/domibus/fs_plugin_data" >> "$FS_PROPS"
  mkdir -p "$LAB/$NODE/domibus/fs_plugin_data"

  echo "Installed $NODE"
  ls "$PLUGINS/lib" "$PLUGINS/config"
done

rm /tmp/domibus.zip
rm -rf /tmp/fs-plugin.zip /tmp/fs-plugin

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
