# Mira's Trainer Loader Component

This bit of magic wizardry is brought to you by ChendoChap, implemented by kiwidog. Buckle up kids it's about to be a wild ride.

## About
In order for trainers to function properly there were multiple approaches taken. The first was an implementation by kiwidog that would inject a payload or ELF into an existing process's memory space, then create a new thread on the entrypoint. This worked, sort-of. There were issues when using any of the normal shared libraries (prx's) that were on the system due to some custom Sony stuff with dynlib and how threads are created from `libKernel`. This idea was tossed.

Here come's ChendoChap's baller plan that was quite a bit more work, but in the end has the best and most maintainable result, with 0 nasty hardcoded offsets per firmware. Digging through FreeBSD loader internals you will find a stage after all modules and memory has been loaded, but right before `_start` is called on the process. We hijack the function pointer for this stage in Mira's `kernel` component then we inject `trainer_loader`'s payload into the process, then change the entry point to be the `trainer_loader`'s entry point.

There is some code already on the Mira's `kernel` component that will save the original entry point and make it available via an ioctl. The `trainer_loader` will then request Mira's driver to load any trainers for the specific process and to retrieve the original entry point to jump to it after everything has been loaded.

The trainer loader will expect that the kernel portion has mounted `/mira` and `/_substitute` in the sandbox and will iterate those and load any prx in those directories. (The intital mounting is done when `MIRA_TRAINERS_LOAD` ioctl is called)

## Development
Developing for the `trainer_loader` stub is pretty trivial. You do not have access to any libraries, and syscalls will need to be wrapped. This is to keep the payload as small and portable in-between firmwares as possible.

### Debugging
If you have UART installed in your PS4 and it is enabled by calling `stub_debug_log` in the trainer_loader stub you will be able to see your messages over UART. Otherwise have fun spinloop debugging, or intentionally crashing with various errors to figure out where you are failing. (another reason to keep this code small)