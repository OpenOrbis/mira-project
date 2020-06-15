# Mira Project - PlayStation 4 Homebrew Tools

The Mira Project is a set of tools that grants you more power and control over your jailbroken Playstation 4. It is the result of all the hard work by the OpenOrbis team.

It works differently to the custom firmware experience on Playstation 3, where CFW would be installed on the system via modified PUP files (e.g. Rebug), however once the framework is installed and ran it gives users the same functionality they were previously used to.

If you would like to contribute join us on [Open Orbis Discord], it is the only location where you can find the official project.

# Build Status
| Firmware Version | Passing |
| ------ | ------ |
|4.05|WIP|
|4.55|WIP|
|4.74|![](https://github.com/OpenOrbis/mira-project/workflows/Orbis474/badge.svg)|
|5.01|![](https://github.com/OpenOrbis/mira-project/workflows/Orbis501/badge.svg)|
|5.03|![](https://github.com/OpenOrbis/mira-project/workflows/Orbis503/badge.svg)|
|5.05|![](https://github.com/OpenOrbis/mira-project/workflows/Orbis505/badge.svg)|

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

* [kiwidog] - Developer
* [SpecterDev] - Developer
* [flatz] - Developer (Code, writeups, non-stop help we <3 u flatz)
* [CrazyVoid] - Developer (Loader/self/SDK help, overall general help, OO moderator)
* [theorywrong] - Developer (Substitute, OverlayFS, general)
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
* [LM] - RE (Research on System-Library-Loading), assembler and linker script help
* [TheFlow] - RE
* [samsepi0l] - Offset Porting
* [xvortex] - Original VTX-Hen
* [2much4u] - Ptrace patches
* [golden] - Ptrace patches, rpc ideas
* [idc] - Developer
* [lordfriky] - MacOS support and script fixes
* [ethylamine] - Developer

### Special Thanks
* [bigboss] - liborbis with examples and orbisdev (and complaining a lot)
* [rogero] - Original 5.01 testing
* [AbkarinoMHM] - Original 5.01 testing
* [wildcard] - General questions, and hardware help
* [frangarcj] - orbisdev sdk, musl, C++ support
* [masterzorag] - orbisdev sdk, musl, C++ support
* [fjtrujy] - orbisdev sdk, musl, C++ support
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
`git clone https://github.com/OpenOrbis/mira-project.git`

#### Protobuf files
The RPC messaging system leverages protobuf in order to easily expand and add cross-language RPC support. This involves 2 major components for the default build. Due to previous dependencies, we are not using the C++ version of protobuf due to not having a full STL runtime in the kernel (it was too much work/effort) so instead we are leveraging the [protobuf-c] project.

##### Installing protobuf-c
Installing protobuf-c should only need to be done once, there is support for proto3 files at this time and should not be an issue (compared to previous which only supported proto2).

You can follow the protobuf build instructions located [here](https://github.com/protobuf-c/protobuf-c#building)

##### Building the protobuf file definitions + fixing them
Currently there is a script that will handle *most* of the work required for building new protobuf files. From the *ROOT* of the cloned directory (mira-project/) you can use the provided python script for generating new protobuf files.

`python3 ./scripts/build_proto.py --inputDir=./external`

By default the script will only run in the local directory it was called from. This behavior can be changed with command line argument overrides:

`--inputDir=<input directory>`
`--outputDir=<output directory>` (otherwise use the input directory as default)
`--miraDir=<mira directory>`
`--noMvPbcFiles=<true or false>` (allows you to skip the move of the protobuf files, default: false)

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

The script takes care of generating, fixing, and moving the .c/.h files, as well as the C# (.cs) counterparts for use with the MiraLib/API.

##### Moving and fixing the .c includes
This part has not been scripted yet, because if someone were to add a new proto file, they would have to manually update the script.

###### (Optional) Moving the protobuf files manually
If you decide to not move the protobuf files automatically with the script you can still do it manually (or you can move your own ones).

|File|Intended Location|
| ------ | ------ |
|`external/debugger_structs.pb-c.(c/h)` | `src/Plugins/Debugger` |
|`external/debugger.pb-c.(c/h)` | `src/Plugins/Debugger` |
|`external/filemanager.pb-c.(c/h)` | `src/Plugins/FileManager` |
|`external/rpc.pb-c.(c/h)` | `src/Messaging/Rpc` |

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

#### Source code layout
The current mira-project repository is self-contained, meaning everything the project depends on should be within the repo itself. Here is a layout of the source repository (all from root of mira-project directory)

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

Mira provides a plugin framework that can run in kernel mode (userland is soon, thanks to TW!), it provides a stable framework for startup, shutdown, suspend, resume in order to ensure clean operation of Mira.

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

### Development

Want to contribute? Great! There is no set limit on contributors and people wanting to help out in any way!

Join the [OpenOrbis discord](https://discord.gg/GQr8ydn) and have knowledge of C/C++ and FreeBSD or unix-like operating systems, web design and programming, rust-lang, content creator (youtube, twitch), or artist, or just want to find something to help out with like documentation, hosting, etc, kernel experience is a plus but not required by any means.

#### Building from source
After following the instructions on cloning the repository and generating the protobuf files, you should be ready to build Mira from source. It was designed to be as easy as possible to build with the provided makefiles.

Each makefile (for MiraLoader, and Mira itself) follow a specific format due to compilers ignoring most changes in header (.h) files causing issues down the line.

In order to create the proper build directory output (so you don't get "directory not found errors") is done with
`make create` this will generate the proper folder structure that's required to build Mira.

After this is done, then you will need to `make clean` which will clean up old object files (.o), payloads (.bin), and elf files. Without doing this, any changes you make in the headers will not be cleaned up and you *will* be wondering why you are getting a kernel panic on *correct* code (due to offset change or what not, that you can verify yourself in the binary)

Finally build Mira, this will automatically build everything into the specified build directory using the default `MIRA_PLATFORM` target. You are able to override these options in both MiraLoader and Mira using `MIRA_PLATFORM=<specified platform> make` for your specific platform. (this works with make clean, make create as well)

| Available Overrides | Description |
| ------ | ------ |
|`MIRA_PLATFORM`| Changes firmware version that MiraLoader/Mira targets, they *must* match for your firmware you are building for|
|`BSD_INC`| freebsd-headers include path, defaults to `external/freebsd-headers/include`|
|`OUT_DIR`| Output directory, defaults to `build`|

There is a list of available platforms that Mira can be configured with located in `src/Boot/Config.hpp` for the most up-to-date, but as of writing here is the available list.
| Support Status | Description |
| ------ | ------ |
| Unsupported | May build, may not, previously was updated but no active updating |
| Supported | Active updating and testing, as well as producing builds on CI |
| No Support | No real support added, needs developer work to become functional + stable |

| Available Platforms | Firmware |
| ------ | ------ |
|`MIRA_PLATFORM_ORBIS_BSD_176`| (Unsupported) 1.76 |
|`MIRA_PLATFORM_ORBIS_BSD_355`| (Unsupported) 3.55 |
|`MIRA_PLATFORM_ORBIS_BSD_400`| (Unsupported) 4.00 |
|`MIRA_PLATFORM_ORBIS_BSD_405`| (Supported) 4.05 |
|`MIRA_PLATFORM_ORBIS_BSD_407`| (Unsupported) 4.07 |
|`MIRA_PLATFORM_ORBIS_BSD_455`| (Supported) 4.55 |
|`MIRA_PLATFORM_ORBIS_BSD_474`| (Unsupported) 4.74 |
|`MIRA_PLATFORM_ORBIS_BSD_500`| (Unsupported) 5.00 |
|`MIRA_PLATFORM_ORBIS_BSD_501`| (Unsupported) 5.01 |
|`MIRA_PLATFORM_ORBIS_BSD_503`| (Unsupported) 5.03 |
|`MIRA_PLATFORM_ORBIS_BSD_505`| (Supported) 5.05 |
|`MIRA_PLATFORM_ORBIS_BSD_550`| (No Support) 5.50 |
|`MIRA_PLATFORM_ORBIS_BSD_553`| (No Support) 5.53 |
|`MIRA_PLATFORM_ORBIS_BSD_555`| (No Support) 5.55 |
|`MIRA_PLATFORM_ORBIS_BSD_600`| (No Support) 6.00 |
|`MIRA_PLATFORM_ORBIS_BSD_620`| (Unsupported) 6.20 |
|`MIRA_PLATFORM_ORBIS_BSD_650`| (Unsupported) 6.50 |

Provided are some VSCode `tasks.json` formats:

```
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build Mira",
            "type": "shell",
            "command": "make clean;make create;make",
            "group": "build",
            "problemMatcher": [
                "$gcc"
            ]
        },
        {
            "label": "Build MiraLoader",
            "type": "shell",
            "command": "cd loader;make clean;make create;make",
            "group": "build",
            "problemMatcher": [
                "$gcc"
            ]
        },
        {
            "label": "Build protobuf-tests",
            "type": "shell",
            "command": "clang protobuf-c.c rpc.pb-c.c filemanager.pb-c.c main.c -o test.elf",
            "group": "build",
            "problemMatcher": [
                "$gcc"
            ]
        },
        {
            "label": "Build protobuf files",
            "type": "shell",
            "command": "python3 ./scripts/build_proto.py --inputDir=./external",
            "group": "build",
            "problemMatcher": [
                "$gcc"
            ]
        }
    ]
}
```

And here is an example VSCode `c_cpp_properties.json`
```
{
    "configurations": [
        {
            "name": "Mira",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/src",
                "${workspaceFolder}/external/freebsd-headers/include"
            ],
            "defines": [
                "_KERNEL",
                "MIRA_PLATFORM=MIRA_PLATFORM_ORBIS_BSD_501",
                "__LP64__=1",
                "_M_X64",
                "__amd64__",
                "_DEBUG",
                "__BSD_VISIBLE",
                "MIRA_UNSUPPORTED_PLATFORMS"
            ],
            "compilerPath": "/usr/bin/clang",
            "cStandard": "c11",
            "cppStandard": "c++11",
            "intelliSenseMode": "clang-x64"
        },
        {
            "name": "MiraLoader",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/loader",
                "${workspaceFolder}/external/freebsd-headers/include"
            ],
            "defines": [
                "_KERNEL",
                "MIRA_PLATFORM=MIRA_PLATFORM_ORBIS_BSD_501",
                "__LP64__=1",
                "_M_X64",
                "__amd64__",
                "_DEBUG",
                "__BSD_VISIBLE",
                "MIRA_UNSUPPORTED_PLATFORMS"
            ],
            "compilerPath": "/usr/bin/clang",
            "cStandard": "c11",
            "cppStandard": "c++11",
            "intelliSenseMode": "clang-x64"
        }
    ],
    "version": 4
}
```

#### Firmware porting guide
Lets say you are an eager developer, even a newbie that wants to try and contribute in some way or form to porting to a firmware that is not under active support. Here's the steps you would need to accomplish new builds *from scratch*. We will start by adding a non-existent firmware and work our way from that.

**NOTE: This assumes you already have a kernel dump for your firmware, and things already labeled. If you need help with this step, you can ask in `#help` on the [Discord](https://discord.gg/GQr8ydn) but you are pretty much on your own.***

**WARNING: DO NOT SEND YOUR DUMPED KERNEL IN THE CHANNEL/[DISCORD SERVER](https://discord.gg/GQr8ydn) AS IT IS COPYRIGHTED MATERIALS AND YOU WILL BE WARNED/BANNED!!**

Lets assume our firmware is `8.88` found in the PlayStation 4 System Software menu.

1. Add your new firmware to `src/Boot/Config.hpp` you will see a bunch of defines already there, add your firmware in the correct version order
    a. **#define MIRA_PLATFORM_ORBIS_BSD_888 888**
2. Fix any structure changes for the kernel in freebsd-headers. You should compare against what's already there and add fields that have been added via
    a. **#if MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_888**
    b. *HINT:* These are usually done in `struct proc`, `struct thread`, `struct ucred` if applicable, located in `exernal/freebsd-headers/include`.
3. Add a new static function in `src/Boot/Patches.hpp` with your pre-boot patches, this will be called after MiraLoader finishes and before Mira runs
    a. **static void install_prerunPatches_888();**
4. Add your firmwares version to the case within `install_prePatches` in `src/Boot/Patches.cpp`
    a. `case MIRA_PLATFORM_ORBIS_BSD_888: install_prerunPatches_888(); break;`
5. Next create a new file named `Patches888.cpp` inside of `src/Boot/Patches` directory (or copy an existing one and rename it)
6. You must follow the same format as all of the other patch files, this involves including the `Patches.hpp` and defining the `install_prerunPatches_888()` function with all needed patches
    a. As new features are added, this will need to be updated for any kernel patches required, so far a baseline is Enable UART, Verbose Kernel Panics, Enable RWX mappings, Enable MAP_SELF, Patching copy(in/out)(str) checks, patching memcpy checks, patching ptrace checks, patching setlogin (for autolaunch check), patch mprotect to allow RWX, patching pfs signature checking, patching to enable debug rifs, patch to enable all logs to console, (newer fws: disable sceverifier, delayed panics)
    b. All patches are required for full functionality, but to get up and running only the rwx patches, copy(in/out)(str), memcpy, mprotect patches are needed (I think, someone correct documentation + send PR if wrong).
7. Add support to the MiraLoader by copying the newly finished `src/Boot/Patches.cpp` to `loader/src/Boot/Patches.cpp` and the new `src/Boot/Patches/Patches888.cpp` to `loader/src/Boot/Patches/Patches888.cpp`
8. Next would be to create a new kernel symbol file in `src/Utils/Kdlsym/Orbis888.hpp` or copy one from a supported platform (more offsets than what's probably needed)
9. Add support by modifying `src/Utils/Kdlsym.hpp` and adding either within `#if defined(MIRA_UNSUPPORTED_PLATFORMS)` before the `#endif` a line for your firmware file (make sure these are in numeric order) `#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_888
    #include "Kdlsym/Orbis888.hpp"`
9. The next step would be finding all of the functions that Mira/MiraLoader use in the kernel... This is the most time consuming portion of this and will need to be verified before upstreamed. The easiest way to handle this is to try building (using the build instructions provided) you will get a **massive ton** of errors around `kdlsym` and it not being able to find errors. One of such errors are shown as such:

```
src/External/protobuf-c.c: In function ‘protobuf_c_message_unpack’:
src/Utils/Kdlsym.hpp:49:52: error: ‘kdlsym_addr_printf’ undeclared (first use in this function)
 #define kdlsym(x) ((void*)((uint8_t *)&gKernelBase[kdlsym_addr_ ## x]))
```

10. (continued) This means if you break it down, that **printf** was undeclared, look in your kernel dump with a dissassembler of choice (Ghidra/IDA Preferred, untested with others such as Binary Ninja, Relyze) and get the offset from the start of the loading address for the function `printf` (Calculated by Function Address - Base Address of Kernel where it was dumped from) and add it to your `src/Utils/Kdlsym/Orbis888.hpp` with the line `#define kdlsym_addr_printf                                 0x<offset address>` and repeat for all other build errors.
11. Once complete you should have a full port to a new firmware completed (unless I missed a step/something unclear, create issue or fix + PR please)


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
   [xvortex]: <https://github.com/xvortex>
   [rogero]: <https://github.com/Rogero->
   [AbkarinoMHM]: https://github.com/AbkarinoMHM
   [wildcard]: <https://github.com/VV1LD>
   [2much4u]: <https://github.com/2much4u>
   [golden]: <https://github.com/jogolden>
   [fjtrujy]: <https://github.com/fjtrujy>
   [frangarcj]: <https://github.com/frangarcj>
   [masterzorag]: <https://github.com/masterzorag>
   [SpecterDev]: <https://github.com/Cryptogenic>
   [idc]: <https://github.com/idc>
   [lordfriky]: <https://github.com/lordfriky>
   [ethylamine]: <https://github.com/ethylamine>
   [Open Orbis Discord]: <https://discord.com/invite/GQr8ydn>
