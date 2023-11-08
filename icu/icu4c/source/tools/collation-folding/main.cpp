

#include <iostream>
#include "unicode/ucollationfolding.h"
#include "unicode/putil.h"
#include "unicode/ustring.h"
#include "icu_error.h"

int main()
{
    UErrorCode status = U_ZERO_ERROR;
    UCollationFolding* folding = ucolf_open("de_DE", UCollationStrength::UCOL_PRIMARY, &status);
    auto error = icu_error(status, "ucolf_open");
    if (U_FAILURE(status))
    {
        std::cout << "ucolf_open failed with status: " << error.name() << "\n ";
    }

    UChar buffer[100]; 
    int32_t resultSize = ucolf_fold(folding, u"Käse", -1, buffer, 100, &status);
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