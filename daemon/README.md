# Mira daemon

The Mira daemon is responsible for most utility usages. The daemon is where the RPC will be held, as well as the debugger, trainer scanner, and various other general purpose items. This is where Mira plugins will be loaded into, as well as having crash recovery if something goes wrong. That way you are never left without a daemon running.

## Design

If you were familar with how Mira's `kernel` component was laid out it is very similar. We take advantage of the Open Orbis SDK, compiling with other toolchains will have varying support. In order to build the `daemon` C++11 or greater toolchain support is needed.

---

### Modules

`IModule` interface gives the layout of any kind of code that is used within the daemon. It provides common use cases for most plugins or modules such as:

- Load
- Unload
- Suspend
- Resume
- Name
- Description

---

### Plugins
Plugins will give users the ability to load and unload code in an stable manner. Plugins inherit the `IModule` interface and should be able to be unloaded and reloaded at any point.

There are 2 kinds of Plugins, internal and external. Internal plugins are ones that are compiled directly into the daemon itself and will always be loaded. External plugins will have to be loaded by a directory of choosing. The elf will be loaded and relocated into the current daemon process space then the `IModule.OnLoad` will be called.

#### File Manager
Manage your files. This feature is complete and accessible over RPC.

#### Update Removal
This feature is planned. It will check the update folder and automatically remove it on startup or via RPC if requested.

#### Usermode Debugger and Trainer Scanner
Debugging should be handled in user mode using IOCTL extensions where needed. This will allow us to keep track of a lot more state, and also use modern C++ for development.

This feature is still in the planning phase for vNext.

---

### Rpc
The daemon will support a very basic RPC protocol using `protobuf`. The format for these messages over the wire is as specified

```
uint64_t messageSize;
uint8_t serializedProtobufMessage[messageSize]
```

Follows the `kiss` principal. The maximum message size is currently set to `64 Megabytes`.

#### Rpc Manager
The Rpc manager is responsible for deserializing and routing messages. It will read the amount of data from the wire in the wire format above, attempt to deserialize it, then check a magic to ensure that it is working correctly.

Each message comes in wrapped in a `Mira::Rpc::RpcMessage` as a request and response. The manager will call each `Listener` which will have to filter out which kind of message it is for dispatch.

A `Listener` is a generic interface that will handle incoming message requests. Each of the `Listener.OnMessage` handlers should implement something similar for each of their message types that they can handle.

```
if (p_Request->inner_message().Is<FileManager::EchoRequest>())
        return OnEcho(p_Request, p_Response);
```

An example listener that can be used as an example is the `FileManagerListener` which handles requests and sends back responses when completed.

---

### Settings

The daemon will be responsible as well for updating the local file settings, as well as submitting the settings changes to the `kernel` component. This feature is currently under planning stage, and has not been developed as of yet.


