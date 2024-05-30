# Lunam Engine Scripting API

<img src="../icons/logo.png" width="200" height="200">

## Directories
- `editor` - Editor scripts
- `ext` - External libraries (useable in all scripts)
- `jit` - LuaJIT scripts (internal scripts)
- `lu` - Lunam core API with event handlers
- `system` - System scripts (internal scripts)
- `tools` - Tool scripts (internal scripts)

## Naming Conventions
The naming conventions are similar to Java:
- Classes are named in PascalCase: `MyClass = {}`
- Functions are named in camelCase `function myFunction()`
- Constants are named in UPPER_CASE `MY_CONSTANT = 1`
- Variables and parameters are named in camelCase `local myVariable = 1`
- Private variables are named in _camelCase `local _myPrivateVariable = 1`
- Private functions are named in _camelCase `local function _myPrivateFunction()`
- Event handler begin with two underscores `function __onEvent()`
- FFI functions are all snake case `ffi.cdef("void my_function()")`
- Native to Lua callback functions are also all snake case `function my_callback()`

## Events
**Only scripts inside the 'lu' folder can register event handlers.**<br>
The engine uses events to notify the script about things that happen.<br>
There are two types of events: start and tick events.<br>
Start events are called once when the world is loaded.<br>
Tick events are called every frame.<br>
You can register event handlers by defining functions with the correct name: <br>
`function __onStart() end`<br>
`function __onTick() end`<br>

## General Lua Programming Advice
- Use `local` variables as much as possible
- Use `local` functions as much as possible
- Write functions that do one thing and do it well
- Write clean and readable code
- Use comments to explain complex code
- Use `assert` to check for errors
- Use `error` to throw errors
- Use `panic` to throw fatal errors
- Use `print` to print debug information

## Lua JIT
Lunam engine uses LuaJIT, which is a Just-In-time compiler for Lua.<br>
This means that Lua code is compiled to machine code at runtime,<br>
which makes it much faster than the standard Lua interpreter.<br>
LuaJIT is also compatible with the standard Lua interpreter,<br>
so you can use it to run your scripts.