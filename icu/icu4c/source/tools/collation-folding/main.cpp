

#include <iostream>
#include <fcntl.h>
#include <io.h>
#include "unicode/uchriter.h"
#include "unicode/ucollationfolding.h"
#include "unicode/ustring.h"
#include "unicode/unistr.h"
#include "icu_error.h"
#include "utf8.h"

void set_mode_or_throw(int fd, int mode)
{
    int result{ _setmode(fd, mode) };

    if (result == -1)
    {
        throw std::system_error{ errno, std::generic_category() };
    }
}

constexpr const char* strength_to_string(UCollationStrength value) noexcept
{
    switch (value) {
    case UCollationStrength::UCOL_PRIMARY:
        return "primary";
    case UCollationStrength::UCOL_SECONDARY:
        return "secondary";
    case UCollationStrength::UCOL_TERTIARY:
        return "tertiary";
    case UCollationStrength::UCOL_QUATERNARY:
        return "quaternary";
    case UCollationStrength::UCOL_IDENTICAL:
        return "identical";
    default:
        return "unknown";
    }
}

constexpr char16_t to_hex_digit(uint8_t value) noexcept {
    if (value > 0xF) {
        value = static_cast<uint8_t>(value & 0x0F);
    }
    if (value < 0xA) {
        return u'0' + static_cast<char16_t>(value);
    }
    else {
        return u'A' + static_cast<char16_t>(value - 0xA);
    }
}

icu::UnicodeString toHexString32(UChar32 codepoint) {
    icu::UnicodeString result;

    if (codepoint <= u'\uFFFF') {
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF000) >> 12));
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF00) >> 8));
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF0) >> 4));
        result += to_hex_digit(static_cast<uint8_t>(codepoint & 0xF));
    }
    else if (codepoint <= U'\U000FFFFF') {
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF0000) >> 16));
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF000) >> 12));
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF00) >> 8));
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF0) >> 4));
        result += to_hex_digit(static_cast<uint8_t>(codepoint & 0xF));
    }
    else if (codepoint <= U'\U0010FFFF') {
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF00000) >> 20));
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF0000) >> 16));
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF000) >> 12));
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF00) >> 8));
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF0) >> 4));
        result += to_hex_digit(static_cast<uint8_t>(codepoint & 0xF));
    }
    else {
        std::terminate();
    }

    return result;
}

icu::UnicodeString toHexString(const UChar* source, int32_t sourceLength)
{
    icu::UnicodeString result;
    icu::UCharCharacterIterator iter(source, sourceLength);
    UChar32 c = iter.next32PostInc();
    while (c != icu::CharacterIterator::DONE)
    {
        result += toHexString32(c) + icu::UnicodeString(" ");
        c = iter.next32PostInc();
    }
    return result.trim();
}

const UChar* toNFDString(const UChar* source)
{
    UErrorCode status = U_ZERO_ERROR;
    auto normalizer = unorm2_getNFDInstance(&status);
    if (U_FAILURE(status)) {
        return nullptr;
    }

    // Normalize source string.
    int32_t sourceLength = u_strlen(source);
    int32_t nfdLen = unorm2_normalize(normalizer, source, sourceLength, nullptr, 0, &status);
    icu::LocalArray<UChar> nfdStr(new UChar[nfdLen + 1]);
    if (status == U_BUFFER_OVERFLOW_ERROR) {
        status = U_ZERO_ERROR;
        nfdLen = unorm2_normalize(normalizer, source, sourceLength, nfdStr.getAlias(), nfdLen + 1, &status);
        if (U_FAILURE(status)) {
            return nullptr;
        }
    } else if (U_FAILURE(status)) {
        return nullptr;
    }

    return nfdStr.getAlias();
}

void print_collation_folding(const char* locale, UCollationStrength strength, const UChar* input)
{
    icu::UnicodeString hexInput = toHexString(input, u_strlen(input));
    const UChar* nfdInput = toNFDString(input);
    icu::UnicodeString hexNfdInput = toHexString(nfdInput, u_strlen(nfdInput));
    wprintf(L"Locale:   %s\nStrength: %s\nInput:    %s\t(%s)\nNFDInput: %s\t(%s)\n", 
        from_utf8(locale).c_str(), 
        from_utf8(strength_to_string(strength)).c_str(),
        reinterpret_cast<const wchar_t*>(input),
        reinterpret_cast<const wchar_t*>(hexInput.getTerminatedBuffer()),
        reinterpret_cast<const wchar_t*>(nfdInput),
        reinterpret_cast<const wchar_t*>(hexNfdInput.getTerminatedBuffer()));
    fflush(stdout);
              
    UErrorCode status = U_ZERO_ERROR;
    UCollationFolding* folding = ucolf_open(locale, strength, &status);
    auto error = icu_error(status, "ucolf_open");
    if (U_FAILURE(status))
    {
        wprintf(L"ucolf_open failed with status: %s\n", from_utf8(error.name()).c_str());
        fflush(stdout);
    }

    UChar result[100]; 
    int32_t resultSize = ucolf_fold(folding, input, -1, result, 100, &status);
    error = icu_error(status, "ucolf_fold");
    if (U_FAILURE(status))
    {
        wprintf(L"ucolf_fold failed with status: %s\n", from_utf8(error.name()).c_str());
        fflush(stdout);
    }

    icu::UnicodeString hexResult = toHexString(result, resultSize);
    wprintf(L"Output:   %s\t(%s)\n\n", reinterpret_cast<const wchar_t*>(result), reinterpret_cast<const wchar_t*>(hexResult.getTerminatedBuffer()));
    fflush(stdout);
    ucolf_close(folding);
}

int main()
{
    set_mode_or_throw(_fileno(stdout), _O_U8TEXT);

    //print_NFD_hex(u"Käse");
    //print_NFD_hex(u"Käse");
    //print_NFD_hex(u"Æ");
    //print_NFD_hex(u"Ȧ");
    //print_NFD_hex(u"Ǣ");

    print_collation_folding("de_DE", UCollationStrength::UCOL_PRIMARY, u"Käse");
    print_collation_folding("root", UCollationStrength::UCOL_PRIMARY, u"Käse");
    print_collation_folding("en_US", UCollationStrength::UCOL_PRIMARY, u"Résumé");
    print_collation_folding("fr_FR", UCollationStrength::UCOL_SECONDARY, u"Résumé");
    print_collation_folding("de_DE", UCollationStrength::UCOL_SECONDARY, u"é"); // U+00E9
    print_collation_folding("de_DE", UCollationStrength::UCOL_SECONDARY, u"é"); // U+0065 U+0301
    print_collation_folding("root", UCollationStrength::UCOL_PRIMARY, u"Ǣ"); // U+01E2
    //print_collation_folding("da", UCollationStrength::UCOL_PRIMARY, u"AȦ\x0304"); // U+0041 U+0226 U+0304
    //print_collation_folding("da", UCollationStrength::UCOL_PRIMARY, u"Aȧ\x0304"); // U+0041 U+0227 U+0304
    //print_collation_folding("da", UCollationStrength::UCOL_SECONDARY, u"AȦ\x0304"); // U+0041 U+0226 U+0304
    //print_collation_folding("da", UCollationStrength::UCOL_SECONDARY, u"Aȧ\x0304"); // U+0041 U+0227 U+0304
    //print_collation_folding("da", UCollationStrength::UCOL_TERTIARY, u"AȦ\x0304"); // U+0041 U+0226 U+0304
    //print_collation_folding("da", UCollationStrength::UCOL_TERTIARY, u"Aȧ\x0304"); // U+0041 U+0227 U+0304
    //print_collation_folding("en", UCollationStrength::UCOL_PRIMARY, u"AȦ\x0304"); // U+0041 U+0226 U+0304
    //print_collation_folding("en", UCollationStrength::UCOL_PRIMARY, u"Aȧ\x0304"); // U+0041 U+0227 U+0304
    //print_collation_folding("en", UCollationStrength::UCOL_SECONDARY, u"AȦ\x0304"); // U+0041 U+0226 U+0304
    //print_collation_folding("en", UCollationStrength::UCOL_SECONDARY, u"Aȧ\x0304"); // U+0041 U+0227 U+0304
    //print_collation_folding("en", UCollationStrength::UCOL_TERTIARY, u"AȦ\x0304"); // U+0041 U+0226 U+0304
    //print_collation_folding("en", UCollationStrength::UCOL_TERTIARY, u"Aȧ\x0304"); // U+0041 U+0227 U+0304
}