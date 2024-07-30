#ifndef LUNAM_CORE_HPP
#error "This file must be included from core.hpp"
#endif

template <>
struct fmt::formatter<eastl::string> {
    constexpr auto parse (format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format (const eastl::string& str, Context& ctx) const {
        return format_to(ctx.out(), "{}", str.c_str());
    }
};

template <>
struct fmt::formatter<eastl::string_view> {
    constexpr auto parse (format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format (const eastl::string_view& str, Context& ctx) const {
        const std::string_view std_str_v {str.data(), str.size()};
        return format_to(ctx.out(), "{}", std_str_v);
    }
};

template <>
struct fmt::formatter<DirectX::XMFLOAT2> {
    constexpr auto parse (format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format (const DirectX::XMFLOAT2& v, Context& ctx) const {
        return format_to(ctx.out(), "({}, {})", v.x, v.y);
    }
};

template <>
struct fmt::formatter<DirectX::XMFLOAT2A> {
    constexpr auto parse (format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format (const DirectX::XMFLOAT2A& v, Context& ctx) const {
        return format_to(ctx.out(), "({}, {})", v.x, v.y);
    }
};

template <>
struct fmt::formatter<DirectX::XMFLOAT3> {
    constexpr auto parse (format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format (const DirectX::XMFLOAT3& v, Context& ctx) const {
        return format_to(ctx.out(), "({}, {}, {})", v.x, v.y, v.z);
    }
};

template <>
struct fmt::formatter<DirectX::XMFLOAT3A> {
    constexpr auto parse (format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format (const DirectX::XMFLOAT3A& v, Context& ctx) const {
        return format_to(ctx.out(), "({}, {}, {})", v.x, v.y, v.z);
    }
};

template <>
struct fmt::formatter<DirectX::XMFLOAT4> {
    constexpr auto parse (format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format (const DirectX::XMFLOAT4& v, Context& ctx) const {
        return format_to(ctx.out(), "({}, {}, {}, {})", v.x, v.y, v.z, v.w);
    }
};

template <>
struct fmt::formatter<DirectX::XMFLOAT4A> {
    constexpr auto parse (format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format (const DirectX::XMFLOAT4A& v, Context& ctx) const {
        return format_to(ctx.out(), "({}, {}, {}, {})", v.x, v.y, v.z, v.w);
    }
};

template <>
struct fmt::formatter<DirectX::XMINT2> {
    constexpr auto parse (format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format (const DirectX::XMINT2& v, Context& ctx) const {
        return format_to(ctx.out(), "({}, {})", v.x, v.y);
    }
};

template <>
struct fmt::formatter<DirectX::XMINT3> {
    constexpr auto parse (format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format (const DirectX::XMINT3& v, Context& ctx) const {
        return format_to(ctx.out(), "({}, {}, {})", v.x, v.y, v.z);
    }
};

template <>
struct fmt::formatter<DirectX::XMINT4> {
    constexpr auto parse (format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format (const DirectX::XMINT4& v, Context& ctx) const {
        return format_to(ctx.out(), "({}, {}, {}, {})", v.x, v.y, v.z, v.w);
    }
};

template <>
struct ankerl::unordered_dense::hash<eastl::string> {
    using is_avalanching = void;
    [[nodiscard]] auto operator()(const eastl::string& x) const noexcept -> std::uint64_t {
        return detail::wyhash::hash(x.data(), x.size());
    }
};
