#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "assert.hh"
#include "diagnostic.hh"
#include "iterator.hh"
#include "option.hh"
#include "types.hh"
#include "utility.hh"

namespace ghoti {

// A map from 0-indexed line number to the start of the line
class LineOffsets {
  public:
    MAKE_ITERATOR(Offsets, std::vector<usize>, offsets_)

  public:
    explicit LineOffsets(std::string_view input);
    ~LineOffsets() = default;

    MAKE_MOVE_ONLY(LineOffsets)

    [[nodiscard]] auto operator[](usize line) const noexcept -> usize {
        ASSERT(line < offsets_.size(), "Line offset out of range");
        return offsets_[line];
    }

  private:
    Offsets offsets_;
};

// A source file with efficient source location seeking for diagnostic reporting
class SourceFile {
  public:
    explicit SourceFile(std::string source) noexcept
        : source_{std::move(source)}, offsets_{source} {}
    explicit SourceFile(std::string_view source) : source_{std::string{source}}, offsets_{source} {}

    ~SourceFile() = default;

    MAKE_MOVE_ONLY(SourceFile)

    // Returns the trimmed relevant line in the source along with a caret to the column if possible
    template <traits::Locateable T> [[nodiscard]] auto get_diagnostic_strings(const T& t) const {
        return get_diagnostic_strings_at(traits::SourceInfo<T>::get(t));
    }

    [[nodiscard]] constexpr operator std::string_view() const noexcept { return source_; }
    [[nodiscard]] constexpr operator const std::string&() const noexcept { return source_; }

    [[nodiscard]] constexpr auto empty() const noexcept -> bool { return source_.empty(); }
    [[nodiscard]] constexpr auto size() const noexcept -> usize { return source_.size(); }

  private:
    [[nodiscard]] auto get_diagnostic_strings_at(const SourceLocation& loc) const
        -> std::pair<std::string_view, opt::Option<std::string>>;

  private:
    std::string source_;
    LineOffsets offsets_;
};

} // namespace ghoti
