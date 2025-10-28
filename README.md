# libfabric Practice

A practice application designed to implement a client-server two-way and 
one-way model using libfabric. My previous idea was to implement it alongside
an overridden OpenMPI implementation. What I soon came to realize was that I 
clinically insane, and that I am going to implement libfabric alone first, and
if we get to implementing it alongside OpenMPI, then we can.

## Usage

There isn't one yet. XD

## Installation

Obviously, libfabric is the main dependency used throughout this application, 
so that needs to be installed. 

For installation on FreeBSD...

```
pkg install libfabric-1.15.1_4
```

For installation on Debian Linux.

```
apt install libfabric1 libfabric-bin libfabric-dev
```

Installation is simple, and CMake is used for that simplicity, as well as 
portability.

```
mkdir -p ./build
cmake -S . -B ./build
cmake --build ./build
```

## Author

Javontae Alexander Martin
