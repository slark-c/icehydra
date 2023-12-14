# icehydra - a simple inter-process-communication

## Introduction

Great that you've made it to this project! Let's get you started by providing a quick background
tour, introducing the project scope and all you need for installation and a first running example.

So first off: What is icehydra?

icehydra is an inter-process-communication (IPC) middleware for Linux operating systems (currently we only support Linux,maybe other OS will join in).The inspiration for this came from iceorxy, however,it‘s more lightweight,simpler,and easier to use.




icehydra uses Unix Domain to transfer short data(maybe less than 4KB)  from publishers to subscribers.

Additionally,it also supports the sharing of memory,depending on the user's preferences.



## Build and Install

- mkdir build

- cd build

- cmake .. 

- make && make install



## Examples

After you've built all the necessary things, you can continue playing around with the examples in dir test/

Please see the dedicated [README.md](test/README.md) for information on how to do this.

Enjoy !




