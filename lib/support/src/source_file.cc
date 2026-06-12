#include "source_file.hh"

#include <cctype>
#include <string>
#include <string_view>
#include <utility>

#include "diagnostic.hh"
#include "option.hh"
#include "profiler.hh"
#include "string.hh"
#include "types.hh"

namespace ghoti {

LineOffsets::LineOffsets(std::string_view input) {
    PROFILE_FUNCTION();
    offsets_.emplace_back(0);
    for (usize i{0}; i < input.size(); ++i) {
        if (input[i] == '\n') { offsets_.emplace_back(i + 1); }
    }
}

auto SourceFile::get_diagnostic_strings_at(const SourceLocation& loc) const
    -> std::pair<std::string_view, opt::Option<std::string>> {
    PROFILE_FUNCTION();
    if (loc.line > offsets_.size()) { return {"<invalid line>", opt::none}; }

    const auto start{offsets_[loc.line]};
    const auto end{loc.line + 1 < offsets_.size() ? offsets_[loc.line + 1] : source_.size()};
    auto       substr{string::substr(source_, start, end - start)};

    // Count skipped on the left but not right since the caret is right-clipped
    usize skipped{0};
    substr = string::trim_left(substr, [&skipped](char c) -> bool {
        if (std::isspace(c)) {
            skipped += 1;
            return true;
        }
        return false;
    });
    substr = string::trim_right(substr);

    // Adjust the column number based on skipped spaces
    if (substr.empty() || loc.column < skipped) { return {substr, opt::none}; }
    const auto true_col{loc.column - skipped};

    // Allow 1 past the end to accommodate missing semicolons
    if (true_col > substr.size() + 1) { return {substr, opt::none}; }

    // The caret gets put one after the column size since the location in 0-indexed
    std::string caret_line;
    caret_line.reserve(true_col + 1);
    for (usize i{0}; i < true_col; ++i) {
        if (substr[i] == '\t') {
            caret_line += '\t';
        } else {
            caret_line += ' ';
        }
    }
    caret_line += '^';

    return {substr, std::move(caret_line)};
}

} // namespace ghoti
