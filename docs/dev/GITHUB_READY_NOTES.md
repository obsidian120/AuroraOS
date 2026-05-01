# GitHub-ready snapshot notes

This tree was prepared as a repository snapshot rather than a patch ZIP.

## Included current features

- Desktop text-mode prototype
- x32FS console
- sysdo-only kernelconfig
- CyralithFS config entries
- CBDD groundwork
- CCS/Core settings module
- GRUB ISO build target

## Kernelconfig behavior

`kernelconfig` is intentionally blocked unless opened through:

```text
sysdo cyralith kernelconfig
```

The console color is reset on exit to avoid the gray-background VGA state bug.
