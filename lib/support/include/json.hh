#include <string_view>

#include <fmt/base.h>
#include <fmt/format.h>

namespace ghoti::json {

// A lazily sanitized string operating at format time
struct SanitizedString {
    std::string_view raw;
};

} // namespace ghoti::json

template <> struct fmt::formatter<ghoti::json::SanitizedString> : fmt::formatter<string_view> {
    static auto format(ghoti::json::SanitizedString s, format_context& ctx) {
        auto out{ctx.out()};
        for (auto c : s.raw) {
            switch (c) {
            case '"':  out = format_to(out, "\\\""); continue;
            case '\\': out = format_to(out, "\\\\"); continue;
            case '\b': out = format_to(out, "\\b"); continue;
            case '\f': out = format_to(out, "\\f"); continue;
            case '\n': out = format_to(out, "\\n"); continue;
            case '\r': out = format_to(out, "\\r"); continue;
            case '\t': out = format_to(out, "\\t"); continue;
            default:   break;
            }

            if (c >= 0x00 && c <= 0x1F) {
                out = format_to(out, "\\u{:04x}", c);
                continue;
            }
            *out++ = c;
        }
        return out;
    }
};
