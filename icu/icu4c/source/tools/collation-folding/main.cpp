

#include <iostream>
#include <fcntl.h>
#include <io.h>
#include "unicode/ucollationfolding.h"
#include "unicode/ustring.h"
#include "icu_error.h"

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
    char inputStr[100];
    std::cout << "Locale: " << locale 
              << ", Strength: " << strength_to_string(strength)
              << ", Input: " << u_austrcpy(inputStr, input) << std::endl;
              
    UErrorCode status = U_ZERO_ERROR;
    UCollationFolding* folding = ucolf_open(locale, strength, &status);
    auto error = icu_error(status, "ucolf_open");
    if (U_FAILURE(status))
    {
        std::cout << "ucolf_open failed with status: " << error.name() << "\n ";
    }

    UChar buffer[100]; 
    int32_t resultSize = ucolf_fold(folding, input, -1, buffer, 100, &status);
    error = icu_error(status, "ucolf_fold");
    if (U_FAILURE(status))
    {
        std::cout << "ucolf_fold failed with status: " << error.name() << "\n ";
    }

    char output[100];
    u_austrcpy(output, buffer);
    printf("%s\n", output);
    ucolf_close(folding);
}

int main()
{
    print_collation_folding("de_DE", UCollationStrength::UCOL_PRIMARY, u"Käse");
    print_collation_folding("root", UCollationStrength::UCOL_PRIMARY, u"Käse");
    print_collation_folding("en_US", UCollationStrength::UCOL_PRIMARY, u"Résumé");
    print_collation_folding("fr_FR", UCollationStrength::UCOL_SECONDARY, u"Résumé");
}