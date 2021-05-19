# MiraModule

This is a shared library (prx) that will maintain an API with the `kernel` component and provide hooking, patching, and trainer capabilities to all trainers that are loaded reducing the amount of duplicate code that is needed per trainer. It also will provide many helper functions that trainers would be able to use.

## Substitute
Substitute is used for import table hooking, function hooking chains, and vtable hooks. It originally lived in the `kernel` component but has since been removed in order to be ported to usermode. This way all hooking/patching stays within the process that it's operating on and will no longer cross ring boundaries.

TODO: Fill out more later