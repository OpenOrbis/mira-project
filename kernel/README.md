# Mira Kernel Component

TODO: Specific documentation/build instructions for the kernel component

### Plugins

Mira provides a plugin framework that can run in kernel mode (userland is soon, thanks to TW!), it provides a stable framework for startup, shutdown, suspend, resume in order to ensure clean operation of Mira. These paths are within the `<miraroot>/kernel` directory

| Plugin | Directory |
| ------ | ------ |
| Debugger | src/plugins/Debugger |
| (WIP) Emulated Registry | src/plugins/EmuRegistry |
| Fake PKG | src/plugins/FakePKG |
| Fake Self | src/plugins/FakeSELF |
| File Manager | src/plugins/FileManager |
| (WIP) Fuse | src/plugins/FuseFS |
| Log Server | src/plugins/LogServer |
| (WIP) OverlayFS (OrbisAFR) | src/plugins/OverlayFS |
| (WIP) Substitute | src/plugins/Substitute |