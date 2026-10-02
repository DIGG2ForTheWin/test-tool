#!/bin/bash
set -e

for NODE in blue red; do
  FILE="/opt/lab/$NODE/domibus/conf/domibus/domibus.properties"
  sed -i '/domibus.auth.unsecureLoginAllowed/d' "$FILE"
  echo "domibus.auth.unsecureLoginAllowed=true" >> "$FILE"
done