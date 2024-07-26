// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "assimp/DefaultLogger.hpp"
#include "assimp/StringComparison.h"
#include "assimp/IOSystem.hpp"
#include "assimp/DefaultIOStream.h"
#include "assimp/DefaultIOSystem.h"
#include "assimp/LogStream.hpp"

namespace lu::graphics {
    class assimp_logger final : public Assimp::LogStream {
        auto write(const char* message) -> void override {
            const auto len = std::strlen(message);
            auto* copy = static_cast<char*>(alloca(len));
            std::memcpy(copy, message, len);
            copy[len - 1] = '\0'; // replace \n with \0
            log_info("[Mesh Import]: {}", copy);
        }
    };
}
