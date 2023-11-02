

#include <iostream>
#include "unicode/ucollationfolding.h"
#include "unicode/putil.h"
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
    auto error2 = icu_error(status, "ucolf_fold");
    if (U_FAILURE(status))
    {
        std::cout << "ucolf_fold failed with status: " << error2.name() << "\n ";
    }

    char output[100];
    // Need a better way to convert UChar to char... current mapped output should be only invariant chars though.
    // austrdup is specific to the cintltst code.
    u_UCharsToChars(buffer, output, 100);
    printf("%s\n", output);
    ucolf_close(folding);
}