# Overview

A minimal J1939 implementation, based on the information found in Wilfried Voss' [A Comprehensible Guide to J1939](https://copperhilltech.com/a-comprehensible-guide-to-j1939/).

This library is not a perfect implementation of the standard. Rather, I've taken some liberties to make the implementation as minimal as possible where I can. Above all else, this implementation is designed for me and my basic use cases. Here's an incomplete list of the caveats associated with using this library:
- There is no dynamic memory allocation used in the transport protocol implementation. Rather, there's a single buffer that can hold the maximum number of bytes transferred in a given connection. Each node in the network has a copy of this buffer and it's treated as a shared resource among all nodes. In other words, **there can only be a single broadcast connection open at any given time**.
- The standard defines a method by which a transport protocol connection can remain open while the receiver prepares to receive the data. Given the previous point, I decided to forgo implementing this.
- There is not presently a PGN database compiled into the library, meaning there's no method of querying information about a given PGN at runtime. This means that messages will need to be manually added by the application if it wishes to work with a given PGN.
- There is no mapping of J1939 NAME <-> node address, meaning that node A could claim a new address during runtime and node B wouldn't be able to send a destination-specific to node A (since node B can't determine the node A's new address). This is something I plan on implementing in the near future.
- There is no address claim bus collision management. The standard recommends a prodedure of using random delays in the event of two nodes claiming the same address at the same time, but I couldn't find much information about this process and it seems like an unlikely scenario at best.

This library is meant to be agnostic of any hardware or CAN interface, so the software that links against this library needs to implement certain functions for sending/receiving raw CAN frames on the CAN bus. The functions that implement the low-level interactions with the physical bus are passed to the top-level `j1939_init()` function.

To integrate this project with an existing CMake project, simply clone this repo, add this project as a subdirectory, and link it with your project. For instance:

```cmake
add_subdirectory(mini_j1939)
target_link_libraries(your_project mini_j1939)
```

There are a few CMake variables that can be enabled for controlling the build. This repo contains a demo project that uses the Linux SocketCAN API for testing the library. You can build this by setting the variable `J1939_DEMO` (e.g.: when configuring, pass the option `-DJ1939_DEMO=ON` to CMake). To run the demo, you'll need to load the vcan kernel module and set up the virtual device vcan0 (see [virtual_can.sh](virtual_can.sh)).

You can also optionally enable the `J1939_LISTENER_ONLY_MODE` variable, which will compile the library with the following changes taking effect:
- Every extended CAN frame will be passed to the application layer (including the destination-specific messages that aren't addressed to the receiving node).
- Nodes will not participate in address claim.
- Message transmission is disallowed.
