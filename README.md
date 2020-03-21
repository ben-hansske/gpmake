# gpmake
a small generic build system using lua as an scriptable interface

The goal of gpmake is in first hand to provide a new build-system which can be used to compile any project with c++ modules.
Therefore it needs be able to handle dynamic dependencies as well as multithreaded builds
## Roadmap
we want to provide following functions for lua:
```
require_dependencies("dep1", "dep2", ...) --suspend the currently running task until all the requirements are met
satisfy_dependencies("dep1", "dep2", ...) --tell the dependency manager, that all given dependencies are there and haven't changed
changed_dependencies("dep1", "dep2", ...) --tell the dependency manager, that all given dependencies are rebuilded or changed in any way
failed_dependencies("dep1", "dep2", ...) --tell the dependency manager, that all given dependencies could not be build
spawn_task(function) --create a new task which will be run asynchronously
```

Dependencies are just strings. They could mean targets, single files, libraries, etc.
The order in which task are executed will be unpredictable, but it will make sure that dependencies are met.

## Requirements
+ Lua 5.3
+ cmake 3.12
+ C++17 compiler

### Ubuntu / Debian
```
sudo apt install cmake liblua5.3-dev
```
## Building it
```
git clone https://github.com/ben-hansske/gpmake
cd gpmake
mkdir build && cd build
cmake ..
make
```
