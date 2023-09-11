// © Microsoft Corporation. All rights reserved.

#pragma once

#include <cassert>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include "unicode/ucol.h"
#include "unicode/ucoleitr.h"
#include "unicode/ustring.h"
#include "unicode/ulocdata.h"
#include "icu_error.h"

enum class icu_resource_search_mode
{
    exact_match,
    allow_fallback,
    allow_root
};

enum class icu_resource_search_result
{
    exact_match,
    fallback,
    root
};

struct icu_version final
{
    uint8_t major;
    uint8_t minor;
    uint8_t milli;
    uint8_t micro;
};

struct icu_utf16_to_utf32_item final
{
    constexpr icu_utf16_to_utf32_item() noexcept : m_value{ static_cast<char32_t>(U_SENTINEL) } {}

    constexpr bool operator==(const icu_utf16_to_utf32_item& other) const noexcept { return m_value == other.m_value; }

    constexpr static icu_utf16_to_utf32_item create_valid(char32_t codePoint) noexcept
    {
        return icu_utf16_to_utf32_item{ codePoint };
    }

    constexpr static icu_utf16_to_utf32_item create_valid_checked(char32_t codePoint)
    {
        if (U_IS_SURROGATE(codePoint))
        {
            throw std::runtime_error{ "The value is a surrogate code unit, not a code point." };
        }

        return create_valid(codePoint);
    }

    constexpr static icu_utf16_to_utf32_item create_invalid(char16_t surrogateCodeUnit) noexcept
    {
        return icu_utf16_to_utf32_item{ static_cast<char32_t>(surrogateCodeUnit) };
    }

    constexpr static icu_utf16_to_utf32_item create_invalid_checked(char16_t surrogateCodeUnit)
    {
        if (!U16_IS_SURROGATE(surrogateCodeUnit))
        {
            throw std::runtime_error{ "The value is not a surrogate code unit." };
        }

        return create_invalid(surrogateCodeUnit);
    }

    constexpr bool is_code_point() const noexcept { return !U_IS_SURROGATE(m_value); }

    constexpr char32_t code_point() const noexcept
    {
        assert(is_code_point());
        return m_value;
    };

    char32_t code_point_checked() const
    {
        if (!is_code_point())
        {
            throw std::runtime_error{ "The value is an invalid code unit, not a code point." };
        }

        return code_point();
    };

    constexpr char16_t invalid_code_unit() const noexcept
    {
        assert(!is_code_point());
        return static_cast<char16_t>(m_value);
    }

    char16_t invalid_code_unit_checked() const
    {
        if (is_code_point())
        {
            throw std::runtime_error{ "The value is a code point, not an invalid code unit." };
        }

        return invalid_code_unit();
    }

private:
    constexpr explicit icu_utf16_to_utf32_item(char32_t value) noexcept : m_value{ value } {}

    char32_t m_value;
};

struct icu_utf16_to_utf32_iterator final
{
    using difference_type = ptrdiff_t;
    using element_type = icu_utf16_to_utf32_item;

    constexpr icu_utf16_to_utf32_iterator() noexcept : m_remaining{}, m_current{} {}

    constexpr explicit icu_utf16_to_utf32_iterator(std::u16string_view value) noexcept :
        m_remaining{ value }, m_current{}
    {
        move_next_result result{ move_next(value) };
        m_current = result.value;
        m_remaining = m_remaining.substr(result.size);
    }

    constexpr icu_utf16_to_utf32_item operator*() const noexcept { return m_current; }

    constexpr bool operator==(const icu_utf16_to_utf32_iterator& other) const noexcept
    {
        return m_remaining == other.m_remaining && m_current == other.m_current;
    }

    constexpr icu_utf16_to_utf32_iterator& operator++()
    {
        move_next_result result{ move_next(m_remaining) };
        m_current = result.value;
        m_remaining = m_remaining.substr(result.size);
        return *this;
    }

    constexpr icu_utf16_to_utf32_iterator operator++(int)
    {
        icu_utf16_to_utf32_iterator temp{ *this };
        ++(*this);
        return temp;
    }

private:
    struct move_next_result final
    {
        icu_utf16_to_utf32_item value;
        size_t size;
    };

    constexpr static move_next_result move_next(std::u16string_view value) noexcept
    {
        if (value.empty())
        {
            return {};
        }

        char16_t first{ value[0] };

        if (U16_IS_SURROGATE(first))
        {
            if (value.length() == 1 || !U16_IS_SURROGATE_LEAD(first) || !U16_IS_SURROGATE_TRAIL(value[1]))
            {
                return { icu_utf16_to_utf32_item::create_invalid(first), 1 };
            }

            return { icu_utf16_to_utf32_item::create_valid(U16_GET_SUPPLEMENTARY(first, value[1])), 2 };
        }

        return { icu_utf16_to_utf32_item::create_valid(first), 1 };
    }

    constexpr static size_t get_first_code_point_utf16_length(std::u16string_view value) noexcept
    {
        if (value.empty())
        {
            return 0;
        }

        char16_t first{ value[0] };

        if (U16_IS_SURROGATE(first))
        {
            if (value.length() == 1 || !U16_IS_SURROGATE_LEAD(first) || !U16_IS_SURROGATE_TRAIL(value[1]))
            {
                return 1;
            }

            return 2;
        }

        return 1;
    }

    std::u16string_view m_remaining;
    icu_utf16_to_utf32_item m_current;
};

static_assert(std::forward_iterator<icu_utf16_to_utf32_iterator>);

struct icu_utf16_to_utf32_view final
{
    constexpr icu_utf16_to_utf32_view() noexcept : m_value{} {}

    constexpr icu_utf16_to_utf32_view(std::u16string_view value) noexcept : m_value{ value } {}

    constexpr icu_utf16_to_utf32_iterator begin() const noexcept { return icu_utf16_to_utf32_iterator{ m_value }; }

    constexpr icu_utf16_to_utf32_iterator end() const noexcept
    {
        return icu_utf16_to_utf32_iterator{ m_value.substr(m_value.size()) };
    }

private:
    std::u16string_view m_value;
};

// Some basic tests of iterating icu_utf16_to_utf32_view. These can be compile-time since icu_utf16_to_utf32_view is
// constexpr.

// Empty string
static_assert(icu_utf16_to_utf32_view{ std::u16string_view{ u"" } }.begin() ==
              icu_utf16_to_utf32_view{ std::u16string_view{ u"" } }.end());
static_assert(
    (*icu_utf16_to_utf32_view{ std::u16string_view{ u"" } }.begin()).code_point() == static_cast<char32_t>(-1));

// Single character that does not use surrogates
static_assert(icu_utf16_to_utf32_view{ std::u16string_view{ u"a" } }.begin() !=
              icu_utf16_to_utf32_view{ std::u16string_view{ u"a" } }.end());
static_assert((*icu_utf16_to_utf32_view{ std::u16string_view{ u"a" } }.begin()).is_code_point());
static_assert((*icu_utf16_to_utf32_view{ std::u16string_view{ u"a" } }.begin()).code_point() == u'a');
static_assert(++icu_utf16_to_utf32_view{ std::u16string_view{ u"a" } }.begin() ==
              icu_utf16_to_utf32_view{ std::u16string_view{ u"a" } }.end());

// Single character that uses surrogates
static_assert(icu_utf16_to_utf32_view{ std::u16string_view{ u"\U00010000" } }.begin() !=
              icu_utf16_to_utf32_view{ std::u16string_view{ u"\U00010000" } }.end());
static_assert((*icu_utf16_to_utf32_view{ std::u16string_view{ u"\U00010000" } }.begin()).is_code_point());
static_assert((*icu_utf16_to_utf32_view{ std::u16string_view{ u"\U00010000" } }.begin()).code_point() == U'\U00010000');
static_assert(++icu_utf16_to_utf32_view{ std::u16string_view{ u"\U00010000" } }.begin() ==
              icu_utf16_to_utf32_view{ std::u16string_view{ u"\U00010000" } }.end());

// Lead surrogate without following trail (no character at all)
static_assert(icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 1>{ 0xD800 }.data()), 1 } }.begin() !=
              icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 1>{ 0xD800 }.data()), 1 } }.end());
static_assert(
    !(*icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 1>{ 0xD800 }.data()), 1 } }.begin())
         .is_code_point());
static_assert((*icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 1>{ 0xD800 }.data()), 1 } }.begin())
                  .invalid_code_unit() == 0xD800);
static_assert(
    ++icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 1>{ 0xD800 }.data()), 1 } }.begin() ==
    icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 1>{ 0xD800 }.data()), 1 } }.end());

// Lead surrogate without following trail (non-trail character)
static_assert(
    icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 2>{ 0xD800, u'a' }.data()), 2 } }.begin() !=
    icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 2>{ 0xD800, u'a' }.data()), 2 } }.end());
static_assert(
    !(*icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 2>{ 0xD800, u'a' }.data()), 2 } }.begin())
         .is_code_point());
static_assert(
    (*icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 2>{ 0xD800, u'a' }.data()), 2 } }.begin())
        .invalid_code_unit() == 0xD800);
static_assert(
    (*++icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 2>{ 0xD800, u'a' }.data()), 2 } }.begin())
        .is_code_point());
static_assert(
    (*++icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 2>{ 0xD800, u'a' }.data()), 2 } }.begin())
        .code_point() == u'a');
static_assert(
    ++(++icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 2>{ 0xD800, u'a' }.data()), 2 } }
             .begin()) ==
    icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 2>{ 0xD800, u'a' }.data()), 2 } }.end());

// Trail surrogate without preceding lead
static_assert(icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 1>{ 0xDC00 }.data()), 1 } }.begin() !=
              icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 1>{ 0xDC00 }.data()), 1 } }.end());
static_assert(
    !(*icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 1>{ 0xDC00 }.data()), 1 } }.begin())
         .is_code_point());
static_assert((*icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 1>{ 0xDC00 }.data()), 1 } }.begin())
                  .invalid_code_unit() == 0xDC00);
static_assert(
    ++icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 1>{ 0xDC00 }.data()), 1 } }.begin() ==
    icu_utf16_to_utf32_view{ std::u16string_view{ (std::array<char16_t, 1>{ 0xDC00 }.data()), 1 } }.end());

// Multiple characters that do not use surrogates
static_assert(icu_utf16_to_utf32_view{ std::u16string_view{ u"abc" } }.begin() !=
              icu_utf16_to_utf32_view{ std::u16string_view{ u"abc" } }.end());
static_assert((*icu_utf16_to_utf32_view{ std::u16string_view{ u"abc" } }.begin()).is_code_point());
static_assert((*icu_utf16_to_utf32_view{ std::u16string_view{ u"abc" } }.begin()).code_point() == u'a');
static_assert((*++icu_utf16_to_utf32_view{ std::u16string_view{ u"abc" } }.begin()).is_code_point());
static_assert((*++icu_utf16_to_utf32_view{ std::u16string_view{ u"abc" } }.begin()).code_point() == u'b');
static_assert((*++(++icu_utf16_to_utf32_view{ std::u16string_view{ u"abc" } }.begin())).is_code_point());
static_assert((*++(++icu_utf16_to_utf32_view{ std::u16string_view{ u"abc" } }.begin())).code_point() == u'c');
static_assert(++(++(++icu_utf16_to_utf32_view{ std::u16string_view{ u"abc" } }.begin())) ==
              icu_utf16_to_utf32_view{ std::u16string_view{ u"abc" } }.end());

// Multiple characters that use surrogates
static_assert(icu_utf16_to_utf32_view{ std::u16string_view{ u"\U00010000\U00010001\U0010FFFE\U0010FFFF" } }.begin() !=
              icu_utf16_to_utf32_view{ std::u16string_view{ u"\U00010000\U00010001\U0010FFFE\U0010FFFF" } }.end());
static_assert((*icu_utf16_to_utf32_view{ std::u16string_view{ u"\U00010000\U00010001\U0010FFFE\U0010FFFF" } }.begin())
                  .is_code_point());
static_assert((*icu_utf16_to_utf32_view{ std::u16string_view{ u"\U00010000\U00010001\U0010FFFE\U0010FFFF" } }.begin())
                  .code_point() == U'\U00010000');
static_assert((*++icu_utf16_to_utf32_view{ std::u16string_view{ u"\U00010000\U00010001\U0010FFFE\U0010FFFF" } }.begin())
                  .is_code_point());
static_assert((*++icu_utf16_to_utf32_view{ std::u16string_view{ u"\U00010000\U00010001\U0010FFFE\U0010FFFF" } }.begin())
                  .code_point() == U'\U00010001');
static_assert(
    (*++(++icu_utf16_to_utf32_view{ std::u16string_view{ u"\U00010000\U00010001\U0010FFFE\U0010FFFF" } }.begin()))
        .is_code_point());
static_assert(
    (*++(++icu_utf16_to_utf32_view{ std::u16string_view{ u"\U00010000\U00010001\U0010FFFE\U0010FFFF" } }.begin()))
        .code_point() == U'\U0010FFFE');
static_assert(
    (*++(++(++icu_utf16_to_utf32_view{ std::u16string_view{ u"\U00010000\U00010001\U0010FFFE\U0010FFFF" } }.begin())))
        .is_code_point());
static_assert(
    (*++(++(++icu_utf16_to_utf32_view{ std::u16string_view{ u"\U00010000\U00010001\U0010FFFE\U0010FFFF" } }.begin())))
        .code_point() == U'\U0010FFFF');
static_assert(
    ++(++(
        ++(++icu_utf16_to_utf32_view{ std::u16string_view{ u"\U00010000\U00010001\U0010FFFE\U0010FFFF" } }.begin()))) ==
    icu_utf16_to_utf32_view{ std::u16string_view{ u"\U00010000\U00010001\U0010FFFE\U0010FFFF" } }.end());

// Multiple characters including those that use surrogates and those that do not use surrogates
static_assert(icu_utf16_to_utf32_view{ std::u16string_view{ u"a\U00010000b\U0010FFFF" } }.begin() !=
              icu_utf16_to_utf32_view{ std::u16string_view{ u"a\U00010000b\U0010FFFF" } }.end());
static_assert((*icu_utf16_to_utf32_view{ std::u16string_view{ u"a\U00010000b\U0010FFFF" } }.begin()).is_code_point());
static_assert(
    (*icu_utf16_to_utf32_view{ std::u16string_view{ u"a\U00010000b\U0010FFFF" } }.begin()).code_point() == u'a');
static_assert((*++icu_utf16_to_utf32_view{ std::u16string_view{ u"a\U00010000b\U0010FFFF" } }.begin()).is_code_point());
static_assert((*++icu_utf16_to_utf32_view{ std::u16string_view{ u"a\U00010000b\U0010FFFF" } }.begin()).code_point() ==
              U'\U00010000');
static_assert(
    (*++(++icu_utf16_to_utf32_view{ std::u16string_view{ u"a\U00010000b\U0010FFFF" } }.begin())).is_code_point());
static_assert(
    (*++(++icu_utf16_to_utf32_view{ std::u16string_view{ u"a\U00010000b\U0010FFFF" } }.begin())).code_point() == u'b');
static_assert(
    (*++(++(++icu_utf16_to_utf32_view{ std::u16string_view{ u"a\U00010000b\U0010FFFF" } }.begin()))).is_code_point());
static_assert(
    (*++(++(++icu_utf16_to_utf32_view{ std::u16string_view{ u"a\U00010000b\U0010FFFF" } }.begin()))).code_point() ==
    U'\U0010FFFF');
static_assert(++(++(++(++icu_utf16_to_utf32_view{ std::u16string_view{ u"a\U00010000b\U0010FFFF" } }.begin()))) ==
              icu_utf16_to_utf32_view{ std::u16string_view{ u"a\U00010000b\U0010FFFF" } }.end());

struct icu_utf32_to_utf16_converter final
{
    constexpr icu_utf32_to_utf16_converter(char32_t value) noexcept : m_value{ value } {}

    constexpr bool has_surrogate_pair() const noexcept { return !U_IS_BMP(m_value); }

    constexpr char16_t non_surrogate_value() const noexcept
    {
        assert(!has_surrogate_pair());
        return static_cast<char16_t>(m_value);
    }

    char16_t non_surrogate_value_checked() const
    {
        if (has_surrogate_pair())
        {
            throw std::runtime_error{ "The code point requires a surrogate pair." };
        }

        return non_surrogate_value();
    }

    constexpr char16_t lead_surrogate() const noexcept
    {
        assert(has_surrogate_pair());
        return U16_LEAD(m_value);
    }

    char16_t lead_surrogate_checked() const
    {
        if (!has_surrogate_pair())
        {
            throw std::runtime_error{ "The code point does not use a surrogate pair." };
        }

        return lead_surrogate();
    }

    constexpr char16_t trail_surrogate() const noexcept
    {
        assert(has_surrogate_pair());
        return U16_TRAIL(m_value);
    }

    char16_t trail_surrogate_checked() const
    {
        if (!has_surrogate_pair())
        {
            throw std::runtime_error{ "The code point does not use a surrogate pair." };
        }

        return trail_surrogate();
    }

private:
    char32_t m_value;
};

struct unique_UCollator final
{
    constexpr unique_UCollator() noexcept : m_value{} {}

    constexpr explicit unique_UCollator(UCollator* value) noexcept : m_value{ value } {}

    unique_UCollator(const unique_UCollator&) = delete;

    unique_UCollator(unique_UCollator&& other) noexcept : m_value{ other.m_value } { other.m_value = nullptr; }

    ~unique_UCollator() noexcept
    {
        if (m_value == nullptr)
        {
            return;
        }

        ucol_close(m_value);
    }

    unique_UCollator& operator=(const unique_UCollator&) = delete;

    unique_UCollator& operator=(unique_UCollator&& other) noexcept
    {
        if (this != &other)
        {
            if (m_value != nullptr)
            {
                ucol_close(m_value);
            }

            m_value = other.m_value;
            other.m_value = nullptr;
        }

        return *this;
    }

    constexpr UCollator* get() noexcept { return m_value; }

    operator bool() const noexcept { return m_value != nullptr; }

private:
    UCollator* m_value;
};

struct unique_UCollationElements final
{
    constexpr unique_UCollationElements() noexcept : m_value{} {}

    constexpr explicit unique_UCollationElements(UCollationElements* value) noexcept : m_value{ value } {}

    unique_UCollationElements(const unique_UCollationElements&) = delete;

    unique_UCollationElements(unique_UCollationElements&& other) noexcept : m_value{ other.m_value }
    {
        other.m_value = nullptr;
    }

    ~unique_UCollationElements() noexcept
    {
        if (m_value == nullptr)
        {
            return;
        }

        ucol_closeElements(m_value);
    }

    unique_UCollationElements& operator=(const unique_UCollationElements&) = delete;

    unique_UCollationElements& operator=(unique_UCollationElements&& other) noexcept
    {
        if (this != &other)
        {
            if (m_value != nullptr)
            {
                ucol_closeElements(m_value);
            }

            m_value = other.m_value;
            other.m_value = nullptr;
        }

        return *this;
    }

    constexpr UCollationElements* get() noexcept { return m_value; }

    operator bool() const noexcept { return m_value != nullptr; }

private:
    UCollationElements* m_value;
};

struct unique_USet final
{
    constexpr unique_USet() noexcept : m_value{} {}

    constexpr explicit unique_USet(USet* value) noexcept : m_value{ value } {}

    unique_USet(const unique_USet&) = delete;

    unique_USet(unique_USet&& other) noexcept : m_value{ other.m_value } { other.m_value = nullptr; }

    ~unique_USet() noexcept
    {
        if (m_value == nullptr)
        {
            return;
        }

        uset_close(m_value);
    }

    unique_USet& operator=(const unique_USet&) = delete;

    unique_USet& operator=(unique_USet&& other) noexcept
    {
        if (this != &other)
        {
            if (m_value != nullptr)
            {
                uset_close(m_value);
            }

            m_value = other.m_value;
            other.m_value = nullptr;
        }

        return *this;
    }

    constexpr USet* get() noexcept { return m_value; }

    operator bool() const noexcept { return m_value != nullptr; }

private:
    USet* m_value;
};

struct USet_range final
{
    char32_t start = {};
    char32_t end = {};
};

struct USet_item_result final
{
    std::optional<USet_range> range;
    std::optional<size_t> string_size;
};

inline icu_version to_icu_version(UVersionInfo value) noexcept { return { value[0], value[1], value[2], value[3] }; }

constexpr UBool to_ubool(bool value) noexcept { return value ? 1 : 0; }

inline icu_version u_get_version_cpp() noexcept
{
    UVersionInfo version{};
    u_getVersion(version);
    return to_icu_version(version);
}

inline size_t u_str_to_utf32_cpp(std::u16string_view text, std::span<char32_t> buffer)
{
    if (text.size() > static_cast<std::uintmax_t>(std::numeric_limits<int32_t>::max()))
    {
        throw std::length_error{ "An ICU string cannot exceed int32_t characters." };
    }

    if (buffer.size() > static_cast<std::uintmax_t>(std::numeric_limits<int32_t>::max()))
    {
        throw std::length_error{ "An ICU string cannot exceed int32_t characters." };
    }

    UErrorCode status{};
    int32_t written{};
    UChar32* result{ u_strToUTF32(reinterpret_cast<UChar32*>(buffer.data()), static_cast<int32_t>(buffer.size()),
        &written, static_cast<const UChar*>(text.data()), static_cast<int32_t>(text.size()), &status) };
    assert(result == reinterpret_cast<UChar32*>(buffer.data()));
    std::ignore = result;

    if (status != U_ZERO_ERROR && !((status == U_BUFFER_OVERFLOW_ERROR || status == U_STRING_NOT_TERMINATED_WARNING) &&
                                      buffer.data() == nullptr && buffer.size() == 0))
    {
        throw icu_error{ status, "u_strFromUTF32" };
    }

    if (written < 0)
    {
        assert(written >= 0); // assert(false) with a nicer error message
        throw std::runtime_error{ "Invalid returned size from u_strToUTF32." };
    }

    static_assert(std::numeric_limits<size_t>::max() >= static_cast<uintmax_t>(std::numeric_limits<int32_t>::max()));
    return static_cast<size_t>(written);
}

inline std::u32string u_str_to_utf32_cpp(std::u16string_view text)
{
    size_t size{ u_str_to_utf32_cpp(text, {}) };
    std::u32string buffer{};
    buffer.resize(size + 1);

    if (u_str_to_utf32_cpp(text, buffer) != size)
    {
        assert(false);
        throw std::runtime_error{ "Invalid returned size from u_strToUTF32." };
    }

    buffer.resize(size);
    return buffer;
}

inline void ucol_get_contractions_and_expansions_cpp(
    const UCollator* collator, USet* contractions, USet* expansions, bool add_prefixes)
{
    UErrorCode status{};
    ucol_getContractionsAndExpansions(collator, contractions, expansions, to_ubool(add_prefixes), &status);

    if (status != U_ZERO_ERROR)
    {
        throw icu_error{ status, "ucol_getContractionsAndExpansions" };
    }
}

inline icu_version ucol_get_uca_version_cpp(const UCollator* collator) noexcept
{
    UVersionInfo version{};
    ucol_getUCAVersion(collator, version);
    return to_icu_version(version);
}

inline icu_version ucol_get_version_cpp(const UCollator* collator) noexcept
{
    UVersionInfo version{};
    ucol_getVersion(collator, version);
    return to_icu_version(version);
}

unique_UCollator ucol_open_cpp(const char* locale, icu_resource_search_mode resource_search_mode,
    icu_resource_search_result& resource_search_result)
{
    UErrorCode status{};
    unique_UCollator result{ ucol_open(locale, &status) };

    if (status != U_ZERO_ERROR)
    {
        if (status == U_USING_FALLBACK_WARNING && resource_search_mode >= icu_resource_search_mode::allow_fallback)
        {
            resource_search_result = icu_resource_search_result::fallback;
        }
        else if (status == U_USING_DEFAULT_WARNING && resource_search_mode == icu_resource_search_mode::allow_root)
        {
            resource_search_result = icu_resource_search_result::root;
        }
        else
        {
            throw icu_error{ status, "ucol_open" };
        }
    }
    else
    {
        resource_search_result = icu_resource_search_result::exact_match;
    }

    return result;
}

inline unique_UCollator ucol_open_cpp(const char* locale, icu_resource_search_mode resource_search_mode)
{
    icu_resource_search_result ignore{};
    return ucol_open_cpp(locale, resource_search_mode, ignore);
}

unique_UCollationElements ucol_open_elements_cpp(const UCollator* collator, std::u16string_view text)
{
    static_assert(sizeof(UChar) == sizeof(char16_t));

    if (text.size() > static_cast<std::uintmax_t>(std::numeric_limits<int32_t>::max()))
    {
        throw std::length_error{ "An ICU string cannot exceed int32_t characters." };
    }

    UErrorCode status{};

    unique_UCollationElements result{ ucol_openElements(
        collator, static_cast<const UChar*>(text.data()), static_cast<int32_t>(text.size()), &status) };

    if (status != U_ZERO_ERROR)
    {
        throw icu_error{ status, "ucol_openElements" };
    }

    return result;
}

uint16_t ucol_primary_order_cpp(int32_t value)
{
    int32_t result{ ucol_primaryOrder(value) };

    if (result < 0 || result > static_cast<intmax_t>(std::numeric_limits<uint16_t>::max()))
    {
        throw std::runtime_error{ "Invalid ICU collation element primary order." };
    }

    return static_cast<uint16_t>(result);
}

uint8_t ucol_secondary_order_cpp(int32_t value)
{
    int32_t result{ ucol_secondaryOrder(value) };

    if (result < 0 || result > static_cast<intmax_t>(std::numeric_limits<uint8_t>::max()))
    {
        throw std::runtime_error{ "Invalid ICU collation element secondary order." };
    }

    return static_cast<uint8_t>(result);
}

uint8_t ucol_tertiary_order_cpp(int32_t value)
{
    int32_t result{ ucol_tertiaryOrder(value) };

    if (result < 0 || result > static_cast<intmax_t>(std::numeric_limits<uint8_t>::max()))
    {
        throw std::runtime_error{ "Invalid ICU collation element tertiary order." };
    }

    return static_cast<uint8_t>(result);
}

bool ucol_try_next_cpp(UCollationElements* elements, int32_t& value)
{
    UErrorCode status{};
    int32_t result{ ucol_next(elements, &status) };

    if (status != U_ZERO_ERROR)
    {
        value = {};
        throw icu_error{ status, "ucol_next" };
    }

    if (result == UCOL_NULLORDER)
    {
        value = {};
        return false;
    }

    value = result;
    return true;
}

inline icu_version ulocdata_get_cldr_version_cpp()
{
    UVersionInfo version{};
    UErrorCode status{};
    ulocdata_getCLDRVersion(version, &status);

    if (status != U_ZERO_ERROR)
    {
        throw icu_error{ status, "ulocdata_getCLDRVersion" };
    }

    return to_icu_version(version);
}

inline USet_item_result uset_get_item_cpp(const USet* set, size_t index)
{
    UErrorCode status{};
    UChar32 start{};
    UChar32 end{};

    if (index > static_cast<std::uintmax_t>(std::numeric_limits<int32_t>::max()))
    {
        throw std::length_error{ "An ICU USet index cannot exceed int32_t." };
    }

    int32_t size{ uset_getItem(set, static_cast<int32_t>(index), &start, &end, nullptr, 0, &status) };
    assert(size != 1);

    if (size == 0)
    {
        if (status != U_ZERO_ERROR)
        {
            throw icu_error{ status, "uset_getItem" };
        }

        static_assert(sizeof(UChar32) == sizeof(char32_t));
        USet_range range{ static_cast<char32_t>(start), static_cast<char32_t>(end) };
        return USet_item_result{ range };
    }
    else
    {
        if (status != U_ZERO_ERROR && status != U_BUFFER_OVERFLOW_ERROR && status != U_STRING_NOT_TERMINATED_WARNING)
        {
            throw icu_error{ status, "uset_getItem" };
        }

        if (size < 0)
        {
            assert(size >= 0); // assert(false) with a nicer error message
            throw std::runtime_error{ "Invalid return value from uset_getItem." };
        }

        static_assert(
            std::numeric_limits<size_t>::max() >= static_cast<std::uintmax_t>(std::numeric_limits<int32_t>::max()));
        return USet_item_result{ {}, static_cast<size_t>(size) };
    }
}

inline size_t uset_get_item_count_cpp(const USet* set)
{
    int32_t result{ uset_getItemCount(set) };

    if (result < 0)
    {
        assert(result >= 0); // assert(false) with a nicer error message
        throw std::runtime_error{ "Invalid return value from uset_getItemCount." };
    }

    static_assert(
        std::numeric_limits<size_t>::max() >= static_cast<std::uintmax_t>(std::numeric_limits<int32_t>::max()));
    return static_cast<size_t>(result);
}

inline size_t uset_get_item_string_cpp(const USet* set, size_t index, std::span<char16_t> buffer)
{
    UErrorCode status{};
    static_assert(sizeof(UChar) == sizeof(char16_t));

    if (index > static_cast<std::uintmax_t>(std::numeric_limits<int32_t>::max()))
    {
        throw std::length_error{ "An ICU USet index cannot exceed int32_t." };
    }

    if (buffer.size() > static_cast<std::uintmax_t>(std::numeric_limits<int32_t>::max()))
    {
        throw std::length_error{ "An ICU string cannot exceed int32_t characters." };
    }

    int32_t size{ uset_getItem(set, static_cast<int32_t>(index), nullptr, nullptr, static_cast<UChar*>(buffer.data()),
        static_cast<int32_t>(buffer.size()), &status) };
    assert(size != 1);

    if (status != U_ZERO_ERROR)
    {
        throw icu_error{ status, "uset_getItem" };
    }

    if (size < 0)
    {
        assert(size >= 0); // assert(false) with a nicer error message
        throw std::runtime_error{ "Invalid return value from uset_getItem." };
    }

    if (size == 0)
    {
        throw std::invalid_argument{ "The item at that USet index is not a string." };
    }

    static_assert(
        std::numeric_limits<size_t>::max() >= static_cast<std::uintmax_t>(std::numeric_limits<int32_t>::max()));
    return static_cast<size_t>(size);
}

inline unique_USet uset_open_empty_cpp() { return unique_USet{ uset_openEmpty() }; }
