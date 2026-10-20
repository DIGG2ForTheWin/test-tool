# test-tool

A small C++ command-line program that stages large test messages for the
Domibus **FS plugin**, so that AS4 transfers between two Domibus nodes can be measured.

For every test, the program builds one message folder per message
(`metadata.xml` + payload), then moves it into the plugin's `OUT` folder with an
atomic rename. It prints how long each step took.

Built as part of the SIMS project at Mid Sweden University, together with Digg.

---

## What it does – and what it does not do

**It does**

- read which machine/node it runs on from `node.yaml`
- read test definitions from `tests.yaml`
- generate payload files of a given size (random, incompressible data), **or**
  hard-link an existing file from `<data_root>/testfiles`
- write `metadata.xml` for each message
- place each message in `OUT` atomically, so the FS plugin never sees a half-written message
- print timing per message and a summary

**It does not**

- set up the environment, Domibus or AWS machines
- upload PModes (handled by the environment setup)
- check whether messages were delivered
- use the WS plugin, SSH, or fetch anything from the internet

---

## Requirements

- Ubuntu 24.04 (for `install.sh`)
- Domibus with the FS plugin, and its data folder on the **same filesystem** as
  the test files (needed for atomic rename and hard links)

---

## Installation (Ubuntu 24.04)

```bash
git clone https://github.com/DIGG2ForTheWin/test-tool.git
cd test-tool
./install.sh
```

The script:

1. checks that the machine runs Ubuntu 24.04
2. installs `build-essential`, `cmake` and `libyaml-cpp-dev` with apt
3. builds a Release version in `build-release/`
4. installs `tester` to `/usr/local/bin`
5. creates `node.yaml` from `node.example.yaml` (an existing `node.yaml` is never overwritten)

It is safe to run again, for example after `git pull`.

### Building manually (e.g. macOS)

```bash
brew install cmake yaml-cpp          # macOS
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

A plain `cmake -S . -B build` gives a Debug build with AddressSanitizer –
good for development, but **do not use it for measurements**.

---

## Configuration

The program reads two files from the folder you run it in.

### `node.yaml` – one per machine, not in git

```yaml
node: blue                          # which node this machine is
data_root: /opt/lab/blue/domibus    # Domibus folder on this machine
nodes:                              # exactly two node names
  - blue
  - red
```

`data_root` must contain:

```
<data_root>/
├── fs_plugin_data/
│   └── OUT/          # must exist – watched by the FS plugin
└── testfiles/        # only needed for tests that use 'file'
```

`fs_plugin_data/.staging/` is created by the program.

### `tests.yaml` – shared, in git

Only the `tester:` section is read by this program.

```yaml
tester:
  message:                  # header values for metadata.xml – must match the PMode
    party_id_type: urn:oasis:names:tc:ebcore:partyid-type:unregistered
    sender_role: http://docs.oasis-open.org/ebxml-msg/ebms/v3.0/ns/core/200704/sender
    receiver_role: http://docs.oasis-open.org/ebxml-msg/ebms/v3.0/ns/core/200704/receiver
    service: bdx:noprocess
    service_type: tc1
    action: TC1Leg1

  tests:
    - name: small_files     # a-z, 0-9, '_' and '-'
      from: blue
      to: red
      size: 10 KiB          # generated payload
      count: 2              # optional, default 1
    - name: dicom_sample
      from: blue
      to: red
      file: dicom/0002.dcm  # existing file, relative to <data_root>/testfiles
```

| Field   | Required | Notes |
|---------|----------|-------|
| `name`  | yes | unique; lower-case letters, digits, `_` and `-` |
| `from`, `to` | yes | must be names from `nodes` and must differ |
| `size`  | one of `size`/`file` | a number and a unit: `B`, `KiB`, `MiB` or `GiB` |
| `file`  | one of `size`/`file` | relative path, no `..` – the file is hard-linked, not copied |
| `count` | no | number of messages, at least 1 |

> **Important:** the node names (used as AS4 party ids) and every value under
> `message:` must match the PMode uploaded to Domibus. Otherwise Domibus
> rejects the messages.

---

## Usage

```
tester run <test...|all> [--node <name>] [--data-root <path>]
                         [--node-config <path>] [--test-config <path>]
```

- `run all` runs every test that sends **from** this node
- `run small_files dicom_sample` runs the named tests (they must send from this node)
- `--node` and `--data-root` override `node.yaml`
- `--node-config` / `--test-config` point to other files than `./node.yaml` / `./tests.yaml`

Run it from the repository folder, where `tests.yaml` and `node.yaml` are:

```bash
cd ~/test-tool
tester run all
```

### Example output

```
run id: 20261020T131021Z-6589

small_files_20261020T131021Z-6589_001   10 KiB    metadata 0.000 s  payload 0.037 s  rename 0.000 s
small_files_20261020T131021Z-6589_002   10 KiB    metadata 0.000 s  payload 0.032 s  rename 0.000 s
dicom_sample_20261020T131021Z-6589_001  6 B       metadata 0.000 s  payload 0.000 s  rename 0.000 s  (hard link)

done: 3 messages, 20.0 KiB, 0.1 s
```

For small files, the payload time is mostly the time to fill the random buffer,
not disk speed. Use larger sizes for throughput measurements.

### Message names

Each message folder and payload is named:

```
<test>_<run id>_<n>          e.g. small_files_20261020T131021Z-6589_001
```

- **run id** – UTC start time plus a random hex suffix, unique per run
- **n** – message number, zero-padded to at least three digits
- the payload is `<name>.bin` for generated files, or `<name><original extension>` for hard links

Other tools (e.g. monitoring) can rely on this format. Split the name from the
right, since test names may contain `_`.

### Exit codes

| Code | Meaning |
|------|---------|
| 0 | all messages were placed in `OUT` |
| 1 | runtime error (disk full, permission denied, …) |
| 2 | usage error (wrong command-line arguments) |
| 3 | configuration error (`node.yaml` / `tests.yaml`) |

If a run fails half-way, the program prints how many messages were already
placed in `OUT`. A message that failed is removed from `.staging`, so the FS
plugin never sees a half-written message.

---

## Good to know

- **Free space:** before starting, the program checks that the disk has room for
  all generated payloads plus 10 %. Hard-linked files are not counted, since
  they take no extra space.
- **Don't browse `OUT` in Finder.** macOS creates `.DS_Store` files, which the
  FS plugin may try to send. Use `ls` in a terminal instead.
- **Permissions:** Domibus must be allowed to rename and delete the folders the
  program creates in `OUT`. Check this when Domibus runs as another user
  (for example in a container).

---

## Development

```
src/            program source
tools/          gen_bench – tests file generation on its own
install.sh      build + install script for Ubuntu 24.04
```

```bash
cmake -S . -B build && cmake --build build      # Debug build with AddressSanitizer
./build/gen_bench /tmp/test.bin "500 MiB"       # measure write speed only
```

Rules for this repo: all changes go through a branch and a pull request,
and no force pushes to `main`.
