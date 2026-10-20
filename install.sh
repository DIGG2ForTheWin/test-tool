#!/usr/bin/env bash
# install.sh – builds and installs the tester on Ubuntu 24.04.
# Does NOT: set up Domibus, upload PModes or create test files.
set -euo pipefail

# stops script and writes error message to the user
die() {                      
    echo "install: error: $*" >&2   
    exit 1
}

build_dir="build-release"

# 0. Go to the repo root (the folder this script lives in)
cd "$(dirname "$0")"
# 1. Check that we are on Ubuntu 24.04
if [ ! -f /etc/os-release ]; then 
    die "/etc/os-release was not found (is this Ubuntu?)"
fi

required_id="ubuntu"
required_version="24.04"
. /etc/os-release

if [ "${ID:-}" != "$required_id" ] || [ "${VERSION_ID:-}" != "$required_version" ]; then 
   die "needs Ubuntu $required_version, found ${ID:-} ${VERSION_ID:-}"
fi
# 2. Install build dependencies with apt
sudo apt-get update
sudo apt-get install -y build-essential cmake libyaml-cpp-dev
# 3. Configure and build (Release)
cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_dir" -j "$(nproc)"
# 4. Install the binary to /usr/local/bin
sudo cmake --install "$build_dir"
install_path="$(command -v tester)" || die "tester was installed but is not on PATH (check /usr/local/bin)"
# 5. Create node.yaml from node.example.yaml if it does not exist
if [ ! -f node.yaml ]; then
    cp node.example.yaml node.yaml
    echo "node.yaml was created and needs to be configured"
else 
    echo "node.yaml already exists, nothing was done"
fi
# 6. Print next steps
echo
echo "tester was successfully installed to $install_path"
echo
echo "remember to configure node.yaml such that 'node' and 'data_root' are correct"
echo
echo "run 'tester run all' from $PWD"