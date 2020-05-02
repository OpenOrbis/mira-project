# Mira Project - PlayStation 4 Homebrew Tools

The Mira Project is a set of tools that grants you more power and control over your jailbroken Playstation 4. It is the result of all the hard work by the OpenOrbis team.

It works differently to the custom firmware experience on Playstation 3, where CFW would be installed on the system via modified PUP files (e.g. Rebug), however once the framework is installed and ran it gives users the same functionality they were previously used to.

# Build Status
| Firmware Version | Passing |
| ------ | ------ |
|4.05|[![Build Status](https://ci.appveyor.com/api/projects/status/github/OpenOrbis/mira-project)](https://ci.appveyor.com/api/projects/status/github/OpenOrbis/mira-project)|
|4.55|[![Build Status](https://ci.appveyor.com/api/projects/status/github/OpenOrbis/mira-project)](https://ci.appveyor.com/api/projects/status/github/OpenOrbis/mira-project)|
|4.74|[![Build Status](https://ci.appveyor.com/api/projects/status/github/OpenOrbis/mira-project)](https://ci.appveyor.com/api/projects/status/github/OpenOrbis/mira-project)|
|5.01|[![Build Status](https://ci.appveyor.com/api/projects/status/github/OpenOrbis/mira-project)](https://ci.appveyor.com/api/projects/status/github/OpenOrbis/mira-project)|
|5.05|[![Build Status](https://ci.appveyor.com/api/projects/status/github/OpenOrbis/mira-project)](https://ci.appveyor.com/api/projects/status/github/OpenOrbis/mira-project)|

# New Features!

  - Homebrew Enabler (HEN)
  - Emulated Registry (EmuReg)
  - Emulated NVS (EmuNVS)
  - Kernel Debugger
  - Remote GDB
  - System-level FUSE implementation (Experimental)

You can also:
  - Mount and decrypt local gamesaves (Thanks ChendoChap)
  - Drag and drop markdown and HTML files into Dillinger
  - Export documents as Markdown, HTML and PDF
  - Transfer files to and from the harddrive
  - Implement your own kernel plugins (RPC using protobuf)
  - Implement your own userland trainers (hooks included!)
  - Dump your HDD encryption keys
  - A bunch of other stuff

### Contributors

This project would not be possible without these people (no paticluar order):

* [kiwidog] - Lead developer
* [flatz] - Developer (Code, writeups, non-stop help we <3 u flatz)
* [CrazyVoid] - Developer (Loader/self/SDK help, overall general help, OO moderator)
* [theorywrong] - Developer (OverlayFS, general)
* [SiSTR0] - Developer (HEN support, general)
* [SocraticBliss] - Developer (HEN support, general)
* [valentinbreiz] - Developer (Mira Companion App v1)
* [Seremo] - Developer (Mira Companion App v2, Log plugin)
* [Al-Azif] - Developer (5.05 lead maintainer, general)
* [z80] - Developer (5.05 maintainer)
* [balika011] - Developer (Fixing userland elf loader entry point, general developer)
* [Zer0xFF] - Developer (OverlayFS, general)
* [CelesteBlue] - Developer (Bugfixes, plugins)
* [Joonie] - Developer (Offsets porting 5.01/5.05)
* [AlexAltea] - Low level and kernel help (go check out [Orbital Emulator])
* [qwertyoruiop] - Security (4.55-5.05 kernel exploits)
* CTurt - Security (Initial payload PS4 SDK and 1.76 kernel exploit)
* [m0rph3us1987] - Developer (Code examples, kernel SDK, overall general help)
* [eeply] - Developer (UART)
* [zecoxao] - RE (4.74 Port)
* [aerosoul] - Developer (Everything elf related, loaders, etc)
* [maxton] - Developer (Everything pkg related, etc)
* [ChendoChap] - RE (Bug hunting, general kernel help)
* [sugarleaf] - Initial 4.55 private exploit, inital help with Mira dev (retired/left)
* [kozarovv] - RE (4.05 offsets)
* [LM] - RE (Research on System-Library-Loading)
* [TheFlow] - RE
* [Anon #1] - Developer (Code, Non-stop help, <3 thx bruv)
* [Anon #2] - Developer (Code, Non-stop help, gl with job!)
* [Anon #3] - Security (Future proofing design)
* [Anon #4] - Developer (Ideas from Vita)
* [Anon #5] - Security (Software and hardware)


### Installation

TODO: Instructions

### Plugins

Mira provies a plugin framework that can run in kernel mode (userland is eta wen), it provies a stable framework for startup, shutdown, suspend, resume in order to ensure clean operation of Mira.

| Plugin | Directory |
| ------ | ------ |
| Kernel Debugger | src/plugins/Debugger |
| Emulated Registry | src/plugins/EmuRegistry |
| Fake PKG | src/plugins/FakePKG |
| Fake Self | src/plugins/FakeSELF |
| Fuse | src/plugins/FuseFS |
| Log Server | src/plugins/LogServer |
| File Manager | src/plugins/FileManager |

### Development

Want to contribute? Great!

Join the OpenOrbis discord and have knowledge of C/C++ and FreeBSD or unix-like operating systems. Kernel experience is a plus.

#### Building for source
TODO: WIP

### TODOs

 - Clean kernel rebooting support
 - Web browser activation
 - Fake Online (spoof for LAN usage)
 - Game dumping and decryption
 - FakeDEX support
 - Linux loader
 - Embedded builds into loader
 - Remote registry

License
----

GPLv3


**Free Software, Hell Yeah!**

[//]: # (These are reference links used in the body of this note and get stripped out when the markdown processor does its job. There is no need to format nicely because it shouldn't be seen. Thanks SO - http://stackoverflow.com/questions/4823468/store-comments-in-markdown-syntax)

   [kiwidog]: <https://github.com/kiwidoggie>
   [flatz]: <https://github.com/flatz>
   [CrazyVoid]: <https://github.com/CrazyVoidProgrammer>
   [theorywrong]: <https://github.com/theorywrong>
   [SiSTR0]: <https://github.com/SiSTR0>
   [SocraticBliss]: <https://github.com/SocraticBliss>
   [valentinbreiz]: <https://github.com/valentinbreiz>
   [Seremo]: <https://github.com/Seremo>
   [Al-Azif]: <https://github.com/Al-Azif>
   [z80]: <https://twitter.com/ZiL0G80>
   [balika011]: <https://github.com/balika011>
   [Zer0xFF]: <https://github.com/Zer0xFF>
   [CelesteBlue]: <https://github.com/CelesteBlue-dev>
   [Joonie]: <https://twitter.com/joonie86>
   [AlexAltea]: <https://github.com/AlexAltea>
   [Orbital Emulator]: <https://github.com/AlexAltea/orbital>
   [qwertyoruiop]: <https://twitter.com/qwertyoruiopz>
   [m0rph3us1987]: <https://twitter.com/m0rph3us1987>
   [eeply]: <https://github.com/eeply>
   [zecoxao]: <https://github.com/zecoxao>
   [aerosoul]: <https://github.com/aerosoul94>
   [maxton]: <https://github.com/maxton>
   [ChendoChap]: <https://github.com/ChendoChap>
   [sugarleaf]: <https://github.com/sugarleaf>
   [kozarovv]: <https://github.com/kozarovv>
   [LM]: <https://github.com/lightningmods>
   [TheFlow]: <https://github.com/TheOfficialFloW>