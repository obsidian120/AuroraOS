# Build instructions

## Dependencies

For kernel-only build:

```bash
sudo apt update
sudo apt install build-essential gcc-multilib binutils make
```

For ISO and QEMU run:

```bash
sudo apt install grub-pc-bin xorriso qemu-system-x86
```

## Build kernel

```bash
cd kernel
make clean
make all
```

Output:

```text
kernel/build/cyralith.kernel
```

## Build ISO

```bash
cd kernel
make iso
```

Output:

```text
kernel/build/cyralith.iso
```

## Run

```bash
cd kernel
make run
```

## Notes

The current build targets i386 and uses a freestanding C environment. The OS is experimental and hardware support is limited.
