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
  - System-level FUSE implementation (Experimental, WIP)
  - Load sprx modules + IAT + Function Hooking (Thanks theorywrong)

You can also:
  - Mount and decrypt local gamesaves (Thanks ChendoChap) (WIP)
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
* [samsepi0l] - Offset Porting

### Special Thanks
* [bigboss] - liborbis, and being a hater every step of the way
* [Anon #1] - Developer (Code, Non-stop help, <3 thx bruv)
* [Anon #2] - Developer (Code, Non-stop help, gl with job!)
* [Anon #3] - Security (Future proofing design)
* [Anon #4] - Developer (Ideas from Vita)
* [Anon #5] - Security (Software and hardware)

### Installation

#### Checking out repository
In order to start development on the mira-project, you will need a few components installed.
* clang
* clang-tidy
* cppcheck
* objcopy
* llvm-ar
* ld (or lld.ld)
* git
* python3 (3.6.9 at time of writing, but any newer version should work)

These can be installed (on Ubuntu, other platforms may vary) by using the command(s):
`sudo apt install git build-essential clang lldb clang-tidy clang-tools cppcheck`

#### Cloning the repository
Cloning the repository is easily done by:
`git clone https://github.com/kiwidoggie/miracpp.git`

#### Protobuf files
The RPC messaging system leverages protobuf in order to easily expand and add cross-language RPC support. This involves 2 major components for the default build. Due to previous dependencies, we are not using the C++ version of protobuf due to not having a full STL runtime in the kernel (it was too much work/effort) so instead we are leveraging the [protobuf-c] project.

##### Installing protobuf-c
Installing protobuf-c should only need to be done once, there is support for proto3 files at this time and should not be an issue (compared to previous which only supported proto2).

You can follow the protobuf build instructions located [here](https://github.com/protobuf-c/protobuf-c#building)

##### Building the protobuf file definitions + fixing them
Currently there is a script that will handle *most* of the work required for building new protobuf files. From the *ROOT* of the cloned directory (miracpp/) you can use the provided python script for generating new protobuf files.

`python3 ./scripts/build_proto.py --inputDir=./external`

By default the script will only run in the local directory it was called from. This behavior can be changed with command line argument overrides:

`--inputDir=<input directory>`
`--outputDir=<output directory>` (otherwise use the input directory as default)

The vscode `tasks.json` can be configured to do this automatically in the project repository

```
{
            "label": "Build protobuf files",
            "type": "shell",
            "command": "python3 ./scripts/build_proto.py --inputDir=./external",
            "group": "build",
            "problemMatcher": [
                "$gcc"
            ]
        }
```

The script takes care of generating the .c/.h files, as well as the C# (.cs) counterparts for use with the MiraLib/API.

##### Moving and fixing the .c includes
This part has not been scripted yet, because if someone were to add a new proto file, they would have to manually update the script.

###### Moving the protobuf files
At the time of writing, the default protobuf files that are generated inside of the `<miracpp root>/external` will need to be moved

|File|Intended Location|
| ------ | ------ |
|`external/debugger_structs.pb-c.(c/h)` | `src/Plugins/Debugger` |
|`external/debugger.pb-c.(c/h)` | `src/Plugins/Debugger` |
|`external/filemanager.pb-c.(c/h)` | `src/Plugins/FileManager` |
|`external/debugger_structs.pb-c.(c/h)` | `src/Plugins/Debugger` |
|`external/rpc.pb-c.(c/h)` | `src/Messaging/Rpc` |

###### Fixing the moved protobuf files include paths
In order to prevent linking issues due to the new moved file locations, you will manually have to update the include paths (until the python script supports fixing them on it's own, `psst feel free to submit a PR for it`)

|File|Unpatched Include Location|Patched Include Location|
| ------ | ------ | ------ |
|`src/Messaging/Rpc/rpc.pb-c.c`| `#include "external/rpc.pb-c.h"`| `#include "rpc.pb-c.h"` |
|`src/Plugins/FileManager/filemanager.pb-c.c`| `#include "external/filemanager.pb-c.h"`| `#include "filemanager.pb-c.h"` |
|`src/Plugins/Debugger/debugger.pb-c.c`| `#include "external/debugger.pb-c.h"`| `#include "debugger.pb-c.h"` |
|`src/Plugins/Debugger/debugger_structs.pb-c.c`| `#include "external/debugger_structs.pb-c.h"`| `#include "debugger_structs.pb-c.h"` |

So you get the idea, change the include path to the "local" path of the header file. The actual .h file should be fine as-is.

###### (Optional) Manually fix the C# protobuf files
If you did not use the python script, the C# files will not be automatically fixed for you. There is an issue with modern versions of C# and the output that protobuf generates for .cs files.

By default protobuf generates the C# files looking like so (which cause a build error on .net core)
```
[global::System.Diagnostics.DebuggerNonUserCodeAttribute]
  public void MergeFrom(pb::CodedInputStream input) {
    uint tag;
    while ((tag = input.ReadTag()) != 0) {
      switch(tag) {
        default:
          if (!pb::UnknownFieldSet.MergeFieldFrom(ref _unknownFields, input)) {
            return;
          }
          break;
        case 10: {
          Message = input.ReadString();
          break;
        }
      }
    }
  }
```

This needs a very simple fix in order to build in modern .net core, a line change from:
```
if (!pb::UnknownFieldSet.MergeFieldFrom(ref _unknownFields, input)) {
            return;
          }
```

to

```
_unknownFields = pb::UnknownFieldSet.MergeFieldFrom(_unknownFields, input);
          break;
```

And you can include this anywhere in your C# project and begin to use extension methods to add them to MiraConnection for usage in your own application.

#### Building source code
The current miracpp repository is self-contained, meaning everything the project depends on should be within the repo itself. Here is a layout of the source repository (all from root of miracpp directory)

| Directory | Purpose |
| ------ | ------ |
| `build` | Working/output build files, as well as final executables and payloads |
| `docs` | Documentation about PlayStation 4/Mira twiddly bits |
| `external` | External resources, contains `freebsd-headers` (*MODIFIED FOR KERNEL/PS4 USAGE*), `hde64` for disassembly, `protobuf-c` for protobuf handling, `*.proto` protobuf files to be generated into source |
| `loader` | This is the MiraLoader which is required to *properly* relocate the main ELF in kernel memory and execute in a new kernel thread |
| (Optional/Missing) `protobuf-test` | Test project for protobuf serialization on Linux |
| `scripts` | Build/Helper scripts |
| `src` | Main source code |
| `src/Boot` | Contains everything needed for booting Mira/patching consoles (firmware specific implementations) |
| `src/Driver` | The device driver code (used for user process ioctl, thanks TW for implementing) |
| `src/External` | Source files for those headers in `external` |
| `src/Messaging` | Message manager (protobuf handling and RPC) |
| `src/OrbisOS` | PlayStation 4 specific code and utilities |
| `src/Plugins` | All kernel mode plugins + RPC handlers |
| (Deprecated) `src/Trainers` | Trainer launcher source |
| `src/Utils` | General *Mira* based utilities for working in kernel (kernel function resolving, syscall wrappers, etc) |

### Plugins

Mira provies a plugin framework that can run in kernel mode (userland is soon, thanks to TW!), it provies a stable framework for startup, shutdown, suspend, resume in order to ensure clean operation of Mira.

| Plugin | Directory |
| ------ | ------ |
| Debugger | src/plugins/Debugger |
| (WIP) Emulated Registry | src/plugins/EmuRegistry |
| Fake PKG | src/plugins/FakePKG |
| Fake Self | src/plugins/FakeSELF |
| File Manager | src/plugins/FileManager |
| (WIP) Fuse | src/plugins/FuseFS |
| Log Server | src/plugins/LogServer |
| OverlayFS (OrbisAFR) | src/plugins/OverlayFS |

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
   [samsepi0l]: <https://github.com/ethylamine>
   [protobuf-c]: <https://github.com/protobuf-c/protobuf-c>
   [bigboss]: <https://github.com/psxdev>