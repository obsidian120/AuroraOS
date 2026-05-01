# Cyralith

Cyralith is an experimental, independent hobby operating system by Obsidian. It is not Linux, DOS, UNIX, or Windows based.

This repository snapshot is a **GitHub-ready development tree** for the current Cyralith OS line.

## Current focus

- Custom x86 kernel booted through GRUB Multiboot
- CyralithFS RAM filesystem with persistence hooks
- Shell with German, English, and Indonesian language support
- `sysdo` system-right command flow
- `kernelconfig` as a sysdo-only, menuconfig-style configuration UI
- Desktop text-mode prototype
- Native `x32fs` console for deeper filesystem work
- Basic Display Driver groundwork (`cbdd`)
- User, app, action log, recovery, task, process, paging, and network foundations

## Quick build

From the repository root:

```bash
cd kernel
make clean
make all
```

To build and run an ISO locally, install `grub-mkrescue`, `xorriso`, and QEMU, then run:

```bash
cd kernel
make iso
make run
```

## Important commands inside Cyralith

```text
help
sysdo cyralith kernelconfig
open desktop
x32fs
status
files
network
shutdown
```

`kernelconfig` is intentionally protected. Direct calls such as `kernelconfig`, `settings`, `open kernelconfig`, or `app run kernelconfig` should be blocked unless opened through `sysdo`.

## Repository layout

```text
assets/              Screenshots and project images
docs/                Architecture, roadmap, notes, and dev docs
hosted_ai_shell/     Hosted prototype/helper shell
kernel/              Kernel source, linker script, Makefile, ISO build target
```

## Project status

Cyralith is very early-stage OS development. Expect bugs, missing hardware support, and breaking changes.

This snapshot is intended for source control, review, and continued development.

## License / usage

No open-source license is granted in this snapshot. Do not copy, redistribute, or relicense without permission from the author.

## Author

Programmiert von Obsidian.
