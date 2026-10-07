#!/bin/bash
set -euo pipefail

# Stages the Prometheus JMX Exporter Java agent in /opt/lab/jmx, which compose.yaml
# mounts read-only into the Domibus container. Safe to run again.
# env.py runs it as: sudo bash -s

VERSION="1.6.0"
SHA256="a95983fd96e865d2bcdf911cc500e7c82808c27ab9fd226bf96732b6c3d8c46e"
URL="https://github.com/prometheus/jmx_exporter/releases/download/${VERSION}/jmx_prometheus_javaagent-${VERSION}.jar"
DIR="/opt/lab/jmx"
JAR="$DIR/jmx_prometheus_javaagent.jar"

mkdir -p "$DIR"

# Download only when the pinned jar is not already there
if ! echo "$SHA256  $JAR" | sha256sum -c >/dev/null 2>&1; then
  curl -fsSL --retry 3 -o "$JAR.tmp" "$URL"
  echo "$SHA256  $JAR.tmp" | sha256sum -c -
  mv "$JAR.tmp" "$JAR"
fi

# HikariCP only registers its pool MBeans when asked to. Domibus builds its pools with
# new HikariDataSource(), which reads this file; it switches JMX on and changes nothing else.
echo "registerMbeans=true" > "$DIR/hikari.properties"

chown -R ubuntu:ubuntu "$DIR"
echo "JMX exporter $VERSION staged in $DIR"
