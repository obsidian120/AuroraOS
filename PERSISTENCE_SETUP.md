# Cyralith persistence setup

Cyralith v15 can persist these things to a **virtual ATA hard disk**:
- CyralithFS files and folders
- CyralithFS rights and owners
- user passwords
- network config (hostname, IP, gateway, DHCP flag)
- app install state

## Important
The ISO itself is read-only.
Persistence only works when you attach a **virtual hard disk** in your VM.

## Quick test
1. boot Cyralith from the ISO
2. attach a virtual hard disk
3. run:
   - `elevate aurora`
   - `app install browser`
   - `hostname cyralith-box`
   - `savefs`
4. reboot
5. run:
   - `loadfs`
   - `app list`
   - `network`
