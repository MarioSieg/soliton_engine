## Soliton Engine - Debugging
The C++ code can be debugged using GDB, LLDB, or Visual Studio Debugger.
The easiest way is to just run the engine in a debugger from your IDE, such as CLion, Visual Studio Code, or Qt Creator.

### CLion
Next to the run arrow, there is a bug icon. Click on it to start the debugger.

## Debugging Leaks and Memory Corruption
Sometimes the normal debugging tools are not enough to find memory leaks or memory corruption.
Soliton uses MiMalloc, a fast and efficient memory allocator that can help you find memory leaks and memory corruption.
To enable MiMalloc's debugging features, head to the `extern/mimalloc/CMakeLists.txt` file and change following options:
```cfg
MI_DEBUG_FULL -> ON
MI_TRACK_VALGRIND -> ON
```
Full rebuild and CMake regeneration required.
Delete your build directory and rebuild the project.
