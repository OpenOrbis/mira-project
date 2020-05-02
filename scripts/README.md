In order to build the protobuf files you will need to do a few things.

1. Install protobuf-c & protoc-c compilers from source elsewhere (https://github.com/protobuf-c/protobuf-c)
2. Generate all of the c-bindings (protoc-c --c_out=. rpc.proto filemanager.proto debugger.proto) this will create the Mira-.h/.c files
3. Generate all of the C# bindings (protoc --csharp_out=. rpc.proto filemanager.proto debugger.proto) this will create the needed .cs files for use with MiraUtils
4. Fix the include paths in the .h/.c files (usually point to local file path, change to use proper `#include <Messages/Rpc/rpc-pb.blah>`)
5. The csharp generator is broken out of box (for some odd reason), you will need to find all references to

```
    default:
    if (!pb::UnknownFieldSet.MergeFieldFrom(_unknownFields, input)) { ... }
    ...
```

and change them to

```default:
    _unknownFields = pb::UnknownFieldSet.MergeFieldFrom(_unknownFields, input);
    break;
```

Then everything should build cleanly/work properly providing you did everything correctly