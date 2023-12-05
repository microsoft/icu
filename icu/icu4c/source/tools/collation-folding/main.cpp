

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
    UCollationFolding* folding = ucolfold_open(locale, strength, &status);
    auto error = icu_error(status, "ucolfold_open");
    if (U_FAILURE(status))
    {
        wprintf(L"ucolfold_open failed with status: %s\n", from_utf8(error.name()).c_str());
        fflush(stdout);
    }

    UChar result[100]; 
    int32_t resultSize = ucolfold_fold(folding, input, -1, result, 100, &status);
    error = icu_error(status, "ucolfold_fold");
    if (U_FAILURE(status))
    {
        wprintf(L"ucolfold_fold failed with status: %s\n", from_utf8(error.name()).c_str());
        fflush(stdout);
    }

    icu::UnicodeString hexResult = toHexString(result, resultSize);
    wprintf(L"Output:   %s\t(%s)\n\n", reinterpret_cast<const wchar_t*>(result), reinterpret_cast<const wchar_t*>(hexResult.getTerminatedBuffer()));
    fflush(stdout);
    ucolfold_close(folding);
}

int main()
{
    set_mode_or_throw(_fileno(stdout), _O_U8TEXT);

    print_collation_folding("de_DE", UCollationStrength::UCOL_PRIMARY, u"Käse");
    print_collation_folding("root", UCollationStrength::UCOL_PRIMARY, u"Käse");
    print_collation_folding("en_US", UCollationStrength::UCOL_PRIMARY, u"Résumé");
    print_collation_folding("fr_FR", UCollationStrength::UCOL_SECONDARY, u"Résumé");
    print_collation_folding("de_DE", UCollationStrength::UCOL_SECONDARY, u"é"); // U+00E9
    print_collation_folding("de_DE", UCollationStrength::UCOL_SECONDARY, u"é"); // U+0065 U+0301
    print_collation_folding("root", UCollationStrength::UCOL_PRIMARY, u"Ǣ"); // U+01E2
    print_collation_folding("bs", UCollationStrength::UCOL_PRIMARY, u"nǰ"); // U+006E U+01F0
    print_collation_folding("bs", UCollationStrength::UCOL_PRIMARY, u"Ş̌"); // U+015E U+030C
    print_collation_folding("root", UCollationStrength::UCOL_PRIMARY, u"\uD757"); // U+D757
    print_collation_folding("root", UCollationStrength::UCOL_PRIMARY, u"\u0958"); // U+0958
    
    print_collation_folding("da", UCollationStrength::UCOL_PRIMARY, u"AȦ\u0304"); // U+0041 U+0226 U+0304
    print_collation_folding("da", UCollationStrength::UCOL_PRIMARY, u"Aȧ\u0304"); // U+0041 U+0227 U+0304
    print_collation_folding("da", UCollationStrength::UCOL_SECONDARY, u"AȦ\u0304"); // U+0041 U+0226 U+0304
    print_collation_folding("da", UCollationStrength::UCOL_TERTIARY, u"AȦ\u0304"); // U+0041 U+0226 U+0304
    print_collation_folding("en", UCollationStrength::UCOL_PRIMARY, u"AȦ\u0304"); // U+0041 U+0226 U+0304
    print_collation_folding("en", UCollationStrength::UCOL_PRIMARY, u"Aȧ\u0304"); // U+0041 U+0227 U+0304
    print_collation_folding("en", UCollationStrength::UCOL_SECONDARY, u"AȦ\u0304"); // U+0041 U+0226 U+0304
    print_collation_folding("en", UCollationStrength::UCOL_TERTIARY, u"AȦ\u0304"); // U+0041 U+0226 U+0304

    // Test discontiguous contractions.
    // The ucolfold_fold API normalizes input to NFD, and the collation folding data tables also store strings in NFD form.

    // In Danish, the sequence [U+0061 U+030A U+0323] is canonically equivalent to the sequence [U+0061 U+0323 U+030A] at primary strength.
    // [U+0061 U+030A U+0323] gets normalized to [U+0061 U+0323 U+030A] before performing a lookup to the collation folding data tables.
    print_collation_folding("da", UCollationStrength::UCOL_PRIMARY, u"å"); // U+00E5
    print_collation_folding("da", UCollationStrength::UCOL_PRIMARY, u"ạ"); // U+1EA1
    print_collation_folding("da", UCollationStrength::UCOL_PRIMARY, u"å\u0323"); // U+00E5 U+0323
    print_collation_folding("da", UCollationStrength::UCOL_PRIMARY, u"ạ\u030a"); // U+1EA1 U+030A
    print_collation_folding("da", UCollationStrength::UCOL_PRIMARY, u"a\u030a\u0323"); // U+0061 U+030A U+0323
    print_collation_folding("da", UCollationStrength::UCOL_PRIMARY, u"a\u0323\u030a"); // U+0061 U+0323 U+030A
    
    // Test discontiguous contractions.
    print_collation_folding("sk", UCollationStrength::UCOL_PRIMARY, u"c\u034fh"); // c<CGJ>h
    print_collation_folding("sk", UCollationStrength::UCOL_PRIMARY, u"ch"); // ch
    
    print_collation_folding("root", UCollationStrength::UCOL_PRIMARY, u"a\u0327\u0327\u0327\u030a");
    print_collation_folding("root", UCollationStrength::UCOL_SECONDARY, u"a\u0327\u0327\u0327\u030a");
    
    print_collation_folding("root", UCollationStrength::UCOL_PRIMARY, u"\u0FB3\u0F81");
    print_collation_folding("root", UCollationStrength::UCOL_PRIMARY, u"\u0FB3\u0F71\u0F71\u0F80");

    // Test aliased locales
    print_collation_folding("sh", UCollationStrength::UCOL_PRIMARY, u"C\u0301"); // U+0043 U+0301
    print_collation_folding("sh", UCollationStrength::UCOL_PRIMARY, u"\u0110"); // U+0110
    print_collation_folding("sr_Latn", UCollationStrength::UCOL_PRIMARY, u"C\u0301"); // U+0043 U+0301
    print_collation_folding("sr_Latn", UCollationStrength::UCOL_PRIMARY, u"\u0110"); // U+0110

    print_collation_folding("iw", UCollationStrength::UCOL_PRIMARY, u"\""); // U+0022
    print_collation_folding("iw", UCollationStrength::UCOL_PRIMARY, u"'"); // U+0027
    print_collation_folding("he", UCollationStrength::UCOL_PRIMARY, u"\""); // U+0022
    print_collation_folding("he", UCollationStrength::UCOL_PRIMARY, u"'"); // U+0027

    print_collation_folding("no_NO", UCollationStrength::UCOL_PRIMARY, u"A\u0308"); // U+0041 U+0308
    print_collation_folding("no_NO", UCollationStrength::UCOL_PRIMARY, u"O\u031b\u0323\u0308"); // U+0027 U+031B U+0323 U+0308
    print_collation_folding("no", UCollationStrength::UCOL_PRIMARY, u"A\u0308"); // U+0041 U+0308
    print_collation_folding("no", UCollationStrength::UCOL_PRIMARY, u"O\u031b\u0323\u0308"); // U+0027 U+031B U+0323 U+0308
}