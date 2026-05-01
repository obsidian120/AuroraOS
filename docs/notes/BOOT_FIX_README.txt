Cyralith Boot-Fix
==================

Wenn QEMU/VirtualBox direkt im "grub>" Prompt landet, hat GRUB zwar gestartet, aber keine bootbare Konfiguration gefunden.

Fix in dieser Version:
- kernel/Makefile erzeugt beim Target `iso` automatisch `iso/boot/grub/grub.cfg`
- die Config startet `/boot/cyralith.kernel` per Multiboot

Build/Run:
  cd cyralith/kernel
  make clean
  make run

Oder nur ISO bauen:
  make iso

Architektur-Hinweis:
Die Shell bleibt Kernel-/System-Shell. Kernel-Settings bleiben in `settings`. Desktop-Settings und die spaetere native x32 FS Command Line gehoeren getrennt in den Desktop/System-Mode.
