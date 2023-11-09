

#include <iostream>
#include <fcntl.h>
#include <io.h>
#include "unicode/ucollationfolding.h"
#include "unicode/ustring.h"
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

void print_collation_folding(const char* locale, UCollationStrength strength, const UChar* input)
{
    wprintf(L"Locale: %s, Strength: %s, Input: %s\n", 
        from_utf8(locale).c_str(), 
        from_utf8(strength_to_string(strength)).c_str(), 
        reinterpret_cast<const wchar_t*>(input));
    fflush(stdout);
              
    UErrorCode status = U_ZERO_ERROR;
    UCollationFolding* folding = ucolf_open(locale, strength, &status);
    auto error = icu_error(status, "ucolf_open");
    if (U_FAILURE(status))
    {
        wprintf(L"ucolf_open failed with status: %s\n", from_utf8(error.name()).c_str());
        fflush(stdout);
    }

    UChar buffer[100]; 
    int32_t resultSize = ucolf_fold(folding, input, -1, buffer, 100, &status);
    error = icu_error(status, "ucolf_fold");
    if (U_FAILURE(status))
    {
        wprintf(L"ucolf_fold failed with status: %s\n", from_utf8(error.name()).c_str());
        fflush(stdout);
    }

    wprintf(L"%s\n", reinterpret_cast<const wchar_t*>(buffer));
    fflush(stdout);
    ucolf_close(folding);
}

int main()
{
    set_mode_or_throw(_fileno(stdout), _O_U8TEXT);
    print_collation_folding("de_DE", UCollationStrength::UCOL_PRIMARY, u"Käse");
    print_collation_folding("root", UCollationStrength::UCOL_PRIMARY, u"Käse");
    print_collation_folding("en_US", UCollationStrength::UCOL_PRIMARY, u"Résumé");
    print_collation_folding("fr_FR", UCollationStrength::UCOL_SECONDARY, u"Résumé");
}