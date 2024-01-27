// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "mesh.hpp"

// STB impls are in src/stb/stb_impls.cpp
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_ENABLE_DRACO
#define TINYGLTF_IMPLEMENTATION
#include "../gltf/tiny_gltf.h"

namespace graphics {
    mesh::mesh(const std::string& path) {
        tinygltf::Model model {};
        tinygltf::TinyGLTF loader {};
        std::string err {};
        std::string warn {};
    }

    mesh::~mesh() {

    }
}
