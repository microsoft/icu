// © Microsoft Corporation. All rights reserved.

#pragma once

#include <cassert>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <system_error>
#include <Windows.h>

inline bool try_to_utf8(std::wstring_view text, std::string& result, std::error_code& error) noexcept
{
    error = {};

    if (text.length() == 0)
    {
        try
        {
            result.resize(0);
        }
        catch (...)
        {
            error = std::make_error_code(std::errc::not_enough_memory);
            return false;
        }

        return true;
    }

    constexpr size_t maxLength{ std::numeric_limits<int>::max() };

    if (text.length() > maxLength)
    {
        error = std::make_error_code(std::errc::value_too_large);
        return false;
    }

    int textLength{ static_cast<int>(text.length()) };

    int resultSize{ WideCharToMultiByte(CP_UTF8, 0, text.data(), textLength, nullptr, 0, nullptr, nullptr) };

    if (resultSize == 0)
    {
        // Should not occur, but do not ignore unexpected errors.
        assert(resultSize != 0); // assert(false) with a nicer message.
        error = std::error_code{ static_cast<int>(GetLastError()), std::system_category() };
        return false;
    }
    else if (resultSize < 0)
    {
        // Should not occur, but do not ignore unexpected errors.
        assert(resultSize >= 0); // assert(false) with a nicer message.
        error = std::make_error_code(std::errc::result_out_of_range);
        return false;
    }

    static_assert(std::numeric_limits<size_t>::max() >= static_cast<uintmax_t>(std::numeric_limits<int>::max()));

    try
    {
        result.resize(static_cast<size_t>(resultSize));
    }
    catch (...)
    {
        error = std::make_error_code(std::errc::not_enough_memory);
        return false;
    }

    int written{ WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), textLength, &result[0], resultSize, nullptr, nullptr) };

    if (written == 0)
    {
        uint32_t errorValue{ GetLastError() };

        if (errorValue == ERROR_NO_UNICODE_TRANSLATION)
        {
            error = std::make_error_code(std::errc::illegal_byte_sequence);
            return false;
        }

        // Should not occur, but do not ignore unexpected errors.
        assert(errorValue == ERROR_NO_UNICODE_TRANSLATION); // assert(false) with a nicer message.
        error = std::error_code{ static_cast<int>(errorValue), std::system_category() };
        return false;
    }
    else if (written != resultSize)
    {
        // Should not occur, but do not ignore unexpected errors.
        assert(written == resultSize); // assert(false) with a nicer message.
        error = std::make_error_code(std::errc::result_out_of_range);
        return false;
    }

    return true;
}

inline bool try_to_utf8(std::wstring_view text, std::string& result) noexcept
{
    std::error_code unused{};
    return try_to_utf8(text, result, unused);
}

inline void to_utf8(std::wstring_view text, std::string& result)
{
    std::error_code error{};

    if (!try_to_utf8(text, result, error))
    {
        if (error == std::make_error_code(std::errc::not_enough_memory))
        {
            throw std::bad_alloc{};
        }
        else if (error == std::make_error_code(std::errc::value_too_large))
        {
            throw std::length_error{ "Unable to convert strings longer than int chars." };
        }
        else if (error == std::make_error_code(std::errc::illegal_byte_sequence))
        {
            throw std::invalid_argument{ "Invalid Unicode string." };
        }
        else
        {
            throw std::system_error{ error };
        }
    }
}

inline std::string to_utf8(std::wstring_view text)
{
    std::string result{};
    to_utf8(text, result);
    return result;
}

inline bool try_from_utf8(std::string_view text, std::wstring& result, std::error_code& error) noexcept
{
    error = {};

    if (text.length() == 0)
    {
        try
        {
            result.resize(0);
        }
        catch (...)
        {
            error = std::make_error_code(std::errc::not_enough_memory);
            return false;
        }

        return true;
    }

    constexpr size_t maxLength{ std::numeric_limits<int>::max() };

    if (text.length() > maxLength)
    {
        error = std::make_error_code(std::errc::value_too_large);
        return false;
    }

    int textLength{ static_cast<int>(text.length()) };

    int resultSize{ MultiByteToWideChar(CP_UTF8, 0, text.data(), textLength, nullptr, 0) };

    if (resultSize == 0)
    {
        // Should not occur, but do not ignore unexpected errors.
        assert(resultSize != 0); // assert(false) with a nicer message.
        error = std::error_code{ static_cast<int>(GetLastError()), std::system_category() };
        return false;
    }
    else if (resultSize < 0)
    {
        // Should not occur, but do not ignore unexpected errors.
        assert(resultSize >= 0); // assert(false) with a nicer message.
        error = std::make_error_code(std::errc::result_out_of_range);
        return false;
    }

    static_assert(std::numeric_limits<size_t>::max() >= static_cast<uintmax_t>(std::numeric_limits<int>::max()));

    try
    {
        result.resize(static_cast<size_t>(resultSize));
    }
    catch (...)
    {
        error = std::make_error_code(std::errc::not_enough_memory);
        return false;
    }

    int written{ MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), textLength, &result[0], resultSize) };

    if (written == 0)
    {
        uint32_t errorValue{ GetLastError() };

        if (errorValue == ERROR_NO_UNICODE_TRANSLATION)
        {
            error = std::make_error_code(std::errc::illegal_byte_sequence);
            return false;
        }

        // Should not occur, but do not ignore unexpected errors.
        assert(errorValue == ERROR_NO_UNICODE_TRANSLATION); // assert(false) with a nicer message.
        error = std::error_code{ static_cast<int>(errorValue), std::system_category() };
        return false;
    }
    else if (written != resultSize)
    {
        // Should not occur, but do not ignore unexpected errors.
        assert(written == resultSize); // assert(false) with a nicer message.
        error = std::make_error_code(std::errc::result_out_of_range);
        return false;
    }

    return true;
}

inline bool try_from_utf8(std::string_view text, std::wstring& result) noexcept
{
    std::error_code unused{};
    return try_from_utf8(text, result, unused);
}

inline void from_utf8(std::string_view text, std::wstring& result)
{
    std::error_code error{};

    if (!try_from_utf8(text, result, error))
    {
        if (error == std::make_error_code(std::errc::not_enough_memory))
        {
            throw std::bad_alloc{};
        }
        else if (error == std::make_error_code(std::errc::value_too_large))
        {
            throw std::length_error{ "Unable to convert strings longer than int chars." };
        }
        else if (error == std::make_error_code(std::errc::illegal_byte_sequence))
        {
            throw std::invalid_argument{ "Invalid Unicode string." };
        }
        else
        {
            throw std::system_error{ error };
        }
    }
}

inline std::wstring from_utf8(std::string_view text)
{
    std::wstring result{};
    from_utf8(text, result);
    return result;
}

constexpr inline bool is_high_surrogate(wchar_t value) noexcept { return 0xD800 <= value && value <= 0xDBFF; }
constexpr inline bool is_low_surrogate(wchar_t value) noexcept { return 0xDC00 <= value && value <= 0xDFFF; }

constexpr inline bool is_utf8_one_byte_sequence_start(unsigned char value) noexcept { return value <= 0x7F; }

constexpr inline bool is_utf8_two_byte_sequence_start(unsigned char value) noexcept
{
    return 0xC2 <= value && value <= 0xDF;
}

constexpr inline bool is_utf8_three_byte_sequence_start(unsigned char value) noexcept
{
    return 0xE0 <= value && value <= 0xEF;
}

constexpr inline bool is_utf8_four_byte_sequence_start(unsigned char value) noexcept
{
    return 0xF0 <= value && value <= 0xF4;
}

constexpr inline bool is_utf8_trailing_byte(unsigned char value) noexcept { return 0x80 <= value && value <= 0xBF; }

/**
 * @brief Converts the provided UTF-16 text and UTF-16 index to its corresponding UTF-8 index.
 *
 * @param text The UTF-16 text to be examined
 * @param index The UTF-16 index to convert
 * @return size_t The resulting UTF-8 index
 */
inline size_t to_utf8_index(std::wstring_view text, size_t index)
{
    size_t currentUtf8Index{ 0 };
    size_t currentUtf16Index{ 0 };

    if (index > text.size())
    {
        return text.npos;
    }

    for (; currentUtf16Index < index; ++currentUtf16Index)
    {
        wchar_t codeUnit{ text[currentUtf16Index] };

        if (codeUnit <= 0x7F)
        {
            currentUtf8Index += 1;
        }
        else if (codeUnit <= 0x7FF)
        {
            currentUtf8Index += 2;
        }
        else if (is_high_surrogate(codeUnit))
        {
            ++currentUtf16Index;

            if (currentUtf16Index >= index)
            {
                throw std::system_error{ std::make_error_code(std::errc::illegal_byte_sequence) };
            }

            codeUnit = text[currentUtf16Index];

            if (!is_low_surrogate(codeUnit))
            {
                throw std::system_error{ std::make_error_code(std::errc::illegal_byte_sequence) };
            }

            currentUtf8Index += 4;
        }
        else if (is_low_surrogate(codeUnit))
        {
            throw std::system_error{ std::make_error_code(std::errc::illegal_byte_sequence) };
        }
        else
        {
            currentUtf8Index += 3;
        }
    }

    return currentUtf8Index;
}

/**
 * @brief Converts the provided UTF-8 text and UTF-8 index to its corresponding UTF-16 index.
 *
 * @param text The UTF-8 text to be examined
 * @param index The UTF-8 index to convert
 * @return size_t The resulting UTF-16 index
 */
inline size_t from_utf8_index(std::string_view text, size_t index)
{
    size_t currentUtf8Index{ 0 };
    size_t currentUtf16Index{ 0 };

    if (index > text.size())
    {
        return text.npos;
    }

    for (; currentUtf8Index < index; ++currentUtf8Index)
    {
        unsigned char firstCodeUnit{ static_cast<unsigned char>(text[currentUtf8Index]) };
        size_t numberOfTrailingBytes{ 0 };

        if (is_utf8_one_byte_sequence_start(firstCodeUnit))
        {
            currentUtf16Index += 1;
        }
        else if (is_utf8_two_byte_sequence_start(firstCodeUnit))
        {
            numberOfTrailingBytes = 1;
            currentUtf16Index += 1;
        }
        else if (is_utf8_three_byte_sequence_start(firstCodeUnit))
        {
            numberOfTrailingBytes = 2;
            currentUtf16Index += 1;
        }
        else if (is_utf8_four_byte_sequence_start(firstCodeUnit))
        {
            numberOfTrailingBytes = 3;
            currentUtf16Index += 2;
        }
        else
        {
            throw std::system_error{ std::make_error_code(std::errc::illegal_byte_sequence) };
        }

        // Validate trailing bytes
        size_t trailingIndex{ currentUtf8Index };

        for (size_t i = 0; i < numberOfTrailingBytes; ++i)
        {
            ++trailingIndex;

            if (trailingIndex >= index)
            {
                throw std::system_error{ std::make_error_code(std::errc::illegal_byte_sequence) };
            }

            unsigned char trailingCodeUnit{ static_cast<unsigned char>(text[trailingIndex]) };

            if (!is_utf8_trailing_byte(trailingCodeUnit))
            {
                throw std::system_error{ std::make_error_code(std::errc::illegal_byte_sequence) };
            }
        }

        // Check for exceptions
        // see Table 3-7. Well-Formed UTF-8 Byte Sequences here:
        // https://www.unicode.org/versions/Unicode14.0.0/ch03.pdf#G7404
        if (numberOfTrailingBytes == 2)
        {
            if (firstCodeUnit == 0xE0)
            {
                unsigned char secondCodeUnit{ static_cast<unsigned char>(text[currentUtf8Index + 1]) };

                if (!(0xA0 <= secondCodeUnit && secondCodeUnit <= 0xBF))
                {
                    throw std::system_error{ std::make_error_code(std::errc::illegal_byte_sequence) };
                }
            }
            else if (firstCodeUnit == 0xED)
            {
                unsigned char secondCodeUnit{ static_cast<unsigned char>(text[currentUtf8Index + 1]) };

                if (!(0x80 <= secondCodeUnit && secondCodeUnit <= 0x9F))
                {
                    throw std::system_error{ std::make_error_code(std::errc::illegal_byte_sequence) };
                }
            }
        }
        else if (numberOfTrailingBytes == 3)
        {
            if (firstCodeUnit == 0xF0)
            {
                unsigned char secondCodeUnit{ static_cast<unsigned char>(text[currentUtf8Index + 1]) };

                if (!(0x90 <= secondCodeUnit && secondCodeUnit <= 0xBF))
                {
                    throw std::system_error{ std::make_error_code(std::errc::illegal_byte_sequence) };
                }
            }
            else if (firstCodeUnit == 0xF4)
            {
                unsigned char secondCodeUnit{ static_cast<unsigned char>(text[currentUtf8Index + 1]) };

                if (!(0x80 <= secondCodeUnit && secondCodeUnit <= 0x8F))
                {
                    throw std::system_error{ std::make_error_code(std::errc::illegal_byte_sequence) };
                }
            }
        }

        currentUtf8Index = trailingIndex;
    }

    return currentUtf16Index;
}
