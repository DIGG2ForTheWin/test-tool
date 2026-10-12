#!/bin/bash
set -e
JMX_VERSION=1.0.1
mkdir -p /opt/lab/jmx
curl -fsSL -o /opt/lab/jmx/jmx_prometheus_javaagent.jar \
  "https://repo1.maven.org/maven2/io/prometheus/jmx/jmx_prometheus_javaagent/${JMX_VERSION}/jmx_prometheus_javaagent-${JMX_VERSION}.jar"
chown -R ubuntu:ubuntu /opt/lab