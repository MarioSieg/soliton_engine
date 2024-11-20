# Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

import os
from os import path
import time
import shutil

SHIPPING_BUILD_DIR = 'shipping_build'
SHIPPING_BUILD_OUTPUT_DIR = SHIPPING_BUILD_DIR + '/final'
ARTIFACTS_DIR = SHIPPING_BUILD_DIR + '/artifacts'
LUNAM_ENGINE_BINARY = ARTIFACTS_DIR + '/soliton_engine'
ENGINE_ASSETS_DIR = 'engine_assets'
COM_C = '/usr/bin/clang'
COM_CXX = '/usr/bin/clang++'
ENABLE_WNO_ERROR = '-Wno-error'


# Builds a LUPACK archive from a directory
def build_lupack_archive_from_dir(src_dir: str, filename: str):
    shutil.make_archive(filename, 'zip', src_dir) # For now LUPACK files are just zip files but don't tell anyone, in the future I'll add some custom encryption
    os.rename(filename + '.zip', filename + '.lupack')


start_time = time.time()

print('########################## Soliton Engine Shipping Tool ##########################')
print('This tool will create a shipping build of the Soliton Engine.')
print('Starting the building process...')

if not path.exists('engine_assets'):  # Check that we are in the correct directory
    print('Error: This script must be run from the root directory of the Soliton Engine project.')
    exit(-1)

if path.exists(SHIPPING_BUILD_DIR):
    shutil.rmtree(SHIPPING_BUILD_DIR)
os.mkdir(SHIPPING_BUILD_DIR)
os.mkdir(SHIPPING_BUILD_OUTPUT_DIR)
os.mkdir(ARTIFACTS_DIR)

# Compile engine
num_cpus = os.cpu_count()
print('Configuring engine...')
os.system(f'export CC={COM_C} && export CXX={COM_CXX} && cmake -B {ARTIFACTS_DIR} -S . -DCMAKE_BUILD_TYPE=Release -Wno-dev ')

print(f'Building engine with {num_cpus} cpus ...')
print('This may take a while...')
os.system(f'cmake --build {ARTIFACTS_DIR} --config Release -j {num_cpus}')

# Check that the engine was built
if not path.exists(LUNAM_ENGINE_BINARY):
    print(f'Error: Engine was not built successfully, resulting binary not found: {LUNAM_ENGINE_BINARY}.')
    exit(-1)

print('Engine built successfully!')

# Build engine assets
print('Building engine assets...')
build_lupack_archive_from_dir(ENGINE_ASSETS_DIR, SHIPPING_BUILD_OUTPUT_DIR + '/engine_assets')

# Compose final files
print('Composing final files...')
shutil.copy(LUNAM_ENGINE_BINARY, SHIPPING_BUILD_OUTPUT_DIR + '/soliton_engine')
