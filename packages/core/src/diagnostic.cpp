#include <sstream>

#include "diagnostic.hpp"

namespace conch::detail {

auto format_diagnostic(const std::optional<std::string>&    message,
                       std::string_view                     error_name,
                       const std::optional<SourceLocation>& location) -> std::string {
    std::stringstream ss;
    if (message) { ss << *message << " (" << error_name << ") "; }
    if (location) { ss << std::format("[{}, {}]", location->line, location->column); }
    return ss.str();
}

} // namespace conch::detail
