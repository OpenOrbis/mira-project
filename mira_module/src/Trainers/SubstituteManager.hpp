/**
 * @file SubstituteManager.hpp
 * @author kd_tech_ (@kd_tech_)
 * @brief 
 * @version 0.1
 * @date 2021-03-14
 * 
 * @copyright Copyright (c) OpenOrbis 2021
 * 
 * Alright here goes hopefully the *last* rewrite of Substitute. Instead of
 * forcing Substitute in Mira, we will redesign Mira to support substitutes
 * features from the start.
 * 
 * This will so far involve a few different components which will be laid
 * out as follows.
 * 
 * - Mira Module
 * - Trainers Shared Memory
 * - Communications
 * 
 * - Mira module will be a "common" module that gets loaded into *every* single process.
 * This should contain all of the common features needed by Trainers. This way we can
 * change the underlying implementation without breaking the API.
 * 
 * Each trainer will have some struct that is exposed. This API will have versioning
 * so we won't have to break existing previous trainers in order to make changes.
 * 
 * This common module will also maintain all of the hook states for the current process.
 * This is a semi-strange design decision, but in the end it keeps more *out* of the
 * kernel. I want the kernel footprint to be as small as possible, easily auditable
 * for the most stability.
 * 
 * Common Features:
 * - Function Hooking
 * - VTable Hooking
 * - IAT hooking
 * - taiHEN-like interface (shoutout TheOfficialFloW) but using mira's driver instead
 * - Pattern scanning (change to simd)
 * - Peek/Poke (Kernel/Userland/Other Process)
 * - Process/Module Info
 * - Automatic backup/restore of hooks
 * - Multiple callee support
 * - Allocate/Free Memory (Kernel/Userland/Other Process)
 * - Shared memory support
 * - Mono integration (Trainer options built into PS4 UI)
 * - Load module (kernel/userland)
 * 
 * Trainers will be able to link against this common module, and since it's always
 * loaded into every process, trainers can depend that it is there.
 * 
 * Safety Measures:
 * To prevent crashes left right and center due to users not developing their
 * plugins correctly (which lets face it and not give them as many footguns).
 * There will be a bunch of safety measures in place at the cost of performance
 * because there will be santization on each request to make sure that memory
 * ranges are in-bounds/found with the correct permissions.
 * 
 * At a later date we may make an API to bypass these measures, but meh.
 * 
 * 
 * Function Hooking:
 * Hooking standard code functions will first start with a macro to declare
 * a hook, the return type, function arguments, etc. Then another to define
 * it in code, and the final one to actually "recieve" the hooked execution.
 * 
 * I also want the ability to "skip", "force return" but unsure how to go
 * about it without introducing issues. Current taiHEN just yeets all the
 * calls in one go and return values/changes get stacked on top of each other.
 * 
 * :/ meh again, may start with a similar method then worry about changing it
 * for a v2.
 * 
 * VTable Hooking:
 * This will give an easy API to hook a vtable by index. It will also 
 * attempt to validate that the vtable being overwritten exists in +eXecutable
 * memory. This will help with preventing footguns.
 * 
 * IAT Hooking:
 * Currently there is an API in Substitute for hooking IAT. I think the
 * only refactoring that would need to happen is to give the state to userland.
 * 
 * That way we don't need to keep any extra kernel state around. I'm not sure
 * how feasible this is as of yet.
 * 
 * taiHEN-like interface:
 * People are already used to how taiHEN works. The goal here is to design
 * something very similar so it is easy to pick up and use. This will also
 * help "standardize" how trainer interfaces work. This involves
 * - Hooking Exports
 * - Hooking Imports
 * - Hooking at function offset (user/kernel)
 * - Get process/module information
 * - Remove/disable hooks
 * - Memory injection (user/kernel)
 * - Load/Unload sprx (user/kernel) (current/pid)
 * - Start/Stop prx
 * - Peek/Poke memory (user/kernel)
 * - Reloading configuration
 * 
 * Pattern Scanning:
 * This should be a feature available to trainers for both user and kernel.
 * This will hopefully release some pain when porting trainers to newer versions.
 * If the pattern doesn't change, it should continue to work forwards/backwardss
 * compatible with explicit. This will also skip any non-eXecutable sections for
 * speed.
 * 
 * Example: 
 * MACRO_TO_DECLARE(mygame_engine_function, "\x82\xED\x28\xFF\xBB\xF1\xE8\x00\x00\x00\x00\x80\x2E\x00\xEE\x0E", "xxxxxxx????xxxxx");
 * The first is the byte pattern to match, the second is the mask x = match, ? = wildcard.
 * This will be updated in the future as well to use SIMD instructions for even more speed.
 * 
 * Peek/Poke:
 * Pretty simple, read/write process memory. Create a easy to use api for
 * reading current process memory, another process memory, kernel memory but leave
 * the management up to user-mode for everything. That way trainers can manage
 * *everything*.
 * 
 * Get Process/Module info:
 * This will have to be baked into Mira's driver. But we should create an easy to
 * use API here for getting the information.
 * 
 * Automatic backup/restore of hooks:
 * Hooks should be able to be undone at any point, reverting the code back to original.
 * TODO: Determine how multiple trainers hooking the same thing should work. Should we
 * follow the taiHEN example and clobber or what?
 * 
 * Multiple callee support:
 * There should be an nice way in order to have multiple trainers hook the same thing
 * and not have it blow up in everyones faces. There will be some design issues around that
 * but should not be that hard to work around (if clobbering everything.)
 * 
 * Shared Memory Support:
 * There should be a very easy to use API for creating shared mappings between two processes.
 * This will be used for the Mono Integration explained below, but could be used for any
 * number of things (process scanning, etc).
 * 
 * Trainers themselves will be able to create a Read-Only, or Read-Write "window" into itself
 * and have other trainers, or the Mono Integration read/write to it. These "windows" will
 * have an id, tracking and much more in order to ensure that they are used kinda-safely.
 * 
 * This will involve hooking signal handlers from the common module in order to catch access
 * violations due to the trainer tearing down the shared memory.
 * 
 * ShellUI: Creates local unix socket server
 * Trainer -> Mira Module
 * Trainer: Creates/Maps Memory Region
 * Trainer: Create Shm with generated ID
 * Trainer: Connects to ShellUI on local unix socket
 * Trainer: Sends local info/shared memory name
 * ShellUI: Opens shared memory and parses trainer info
 * ShellUI: Creates UI elements in code/tracks them
 *  - Elements should have the ability to be read-only
 *  - Trainers should not modify any options state (ever)
 * 
 * ShellUI: User Changes UI option
 * ShellUI: Writes into the shared memory region to trigger trainer behavior
 * 
 * Mono Integeration:
 * First of all, huge shoutout to Seremo for getting the original implementation of this done.
 * ShellUI will get a special sprx injected into it which will set up a few things.
 * 
 * 1. Use the MiraModule in order to pattern scan and find function prototypes/hooks/mono crap
 * 2. A local unix socket server to listen for new trainers "coming online"
 *  2a. This will involve the trainer telling ShellUI what shm id was created so ShellUI can map/open it
 *  2b. This should not really change, and should be super simplistic (Request, Pid, ShmName).
 * 3. For each trainer, handle loading/unloading/reloading menu options in ShellUI
 * 4. Modify ShellUI with patches needed for whatever else we could possibly do
 *  4a. Debug Options
 *  4b. Mira options
 *  4c. Trainer options
 *  4d. Mira flair
 * 
 * ╭────────────────────╮
 * │ User Toggles Action│─┐
 * ╰────────────────────╯ │
 *          ╭─────────────╯
 *   ╭────────────────────────╮
 *   │  UI Callback Pokes Shm │         ╭─────────────────────────────────────────────────╮
 *   │  Toggling Trainer Opt  │──────── │ Trainer in thread looping checking for new opts │
 *   ╰────────────────────────╯         │ Or on-hook applying new value set in opts       │
 *                                      ╰─────────────────────────────────────────────────╯
 */