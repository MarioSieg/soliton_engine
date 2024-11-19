import os

files = [
    '/usr/local/bin/dxc',
    '/usr/local/bin/spirv-val',
    '/usr/local/bin/spirv-cross',
    '/usr/local/bin/spirv-remap',
    '/usr/local/bin/spirv-dis',
    '/usr/local/bin/glslc',
    '/usr/local/bin/spirv-lint',
    '/usr/local/bin/spirv-opt',
    '/usr/local/bin/vkvia',
    '/usr/local/bin/glslang',
    '/usr/local/bin/spirv-reflect',
    '/usr/local/bin/spirv-reflect-pp',
    '/usr/local/bin/spirv-cfg',
    '/usr/local/bin/spirv-lesspipe.sh',
    '/usr/local/bin/spirv-reduce',
    '/usr/local/bin/dxc-3.7',
    '/usr/local/bin/spirv-link',
    '/usr/local/bin/vulkaninfo',
    '/usr/local/bin/glslangValidator',
    '/usr/local/bin/MoltenVKShaderConverter',
    '/usr/local/bin/spirv-as',
    '/usr/local/lib/libspirv-cross-util.a',
    '/usr/local/lib/libvulkan.dylib',
    '/usr/local/lib/libvulkan.1.dylib'
]

for f in files:
    if os.path.exists(f):
        os.remove(f)
        print(f + " removed")
    else:
        print(f + " not found")
