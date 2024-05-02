#pragma once

#include "../../../../assetmgr/assetmgr.hpp"

#include <NsGui/Stream.h>

namespace NoesisApp {
    class noesis_lunam_file_stream final : public Noesis::Stream {
    public:
        explicit noesis_lunam_file_stream(std::shared_ptr<assetmgr::istream>&& stream) : m_stream{std::move(stream)} {
            passert(m_stream != nullptr);
        }

        virtual auto SetPosition(const std::uint32_t pos) -> void override{
            m_stream->set_pos(pos, std::ios::beg);
        }
        virtual auto GetPosition() const -> std::uint32_t override {
            return m_stream->get_pos();
        }
        virtual auto GetLength() const -> std::uint32_t override {
            return m_stream->get_length();
        }
        virtual auto Read(void* const buffer, const std::uint32_t size) -> std::uint32_t override {
            return m_stream->read(buffer, size);
        }
        virtual auto GetMemoryBase() const -> const void* override {
            return nullptr;
        }
        virtual auto Close() -> void override {
            m_stream.reset();
        }

        [[nodiscard]] static auto open(const char* uri) -> Noesis::Ptr<Noesis::Stream> {
            auto stream = assetmgr::file_stream::open(uri);
            if (stream) [[likely]] {
                return *new noesis_lunam_file_stream {std::move(stream)}; // TODO: CLion's clangd says this is a memory leak, but it's ok is it
            } else {
                return nullptr;
            }
        }

        [[nodiscard]] auto get_stream() const -> const std::shared_ptr<assetmgr::istream>& {
            return m_stream;
        }

    private:
        std::shared_ptr<assetmgr::istream> m_stream {};
    };
}
