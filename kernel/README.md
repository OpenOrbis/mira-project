# Mira Kernel Component

Mira's kernel component is used to handle all privileged access in a safe and tested environment. This will get rid of the need of using direct kexec syscalls to execute code that only works per-firmware. This is what gives Mira most of it's portability and functionality. In recent times there has been a major push to properly respect all of the ring levels and use proper mechanisms for accessing privileged code. This involved the creation of the `daemon` to handle most taks that are usually done in usermode, as well as adding a device driver that is applied to every newly created process. This device driver is the main way of communication to kernel mode from usermode.

There is a framework in place to "maintain" Mira's kernel state as well as keeping the code somewhat organized. Most of Mira's functionality takes place in `Plugins` which are just interfaces to provide a standard way for handling certain events such as Suspend, Resume, ProcessExit, ProcessExec, etc. Currently all plugins are hard-coded, but in the future will use a modified version of `MiraLoader` to be able to load kernel plugins from file/network as ELF's.

## Boot

## Driver

## External

## Messaging

## OrbisOS

## Plugins

## Trainers

## Utils

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