// uprefs.cpp : Implementation of the APIs declared in uprefs.h
//
#ifndef UPREFS_STATIC_LIB
#define UPREFS_EXPORTS 1
#endif

#define USE_REAL_ICU_HEADERS
#define USE_WINDOWS_ICU

#include <windows.h>
#include "uprefs.h"

U_NAMESPACE_USE

constexpr int32_t UPREFS_API_FAILURE = -1;

#define ARRAYLENGTH(array) (int32_t)(sizeof(array)/sizeof(array[0]))

#define RETURN_FAILURE_STRING_WITH_STATUS_IF(value, error)      \
    if(value)                                                   \
    {                                                           \
        *status = error;                                        \
        return u"";                                             \
    }

#define RETURN_FAILURE_WITH_STATUS_IF(condition, error)         \
    if(condition)                                               \
    {                                                           \
        *status = error;                                        \
        return UPREFS_API_FAILURE;                              \
    }

#define RETURN_VALUE_IF(condition, value)                       \
    if(condition)                                               \
    {                                                           \
        return value;                                           \
    }                                                           \

#define FREE_AND_RETURN_VALUE_IF(condition, value, memoryBlock) \
    if(condition)                                               \
    {                                                           \
        free(memoryBlock);                                      \
        return value;                                           \
    }  

// -------------------------------------------------------
// ----------------- MAPPING FUNCTIONS--------------------
// -------------------------------------------------------

// Maps from a NLS Calendar ID (CALID) to a BCP47 Unicode Extension calendar identifier.
// 
// We map the NLS CALID from GetLocaleInfoEx to the calendar identifier
// used in BCP47 tag with Unicode Extensions.
//
// This does not return a full nor valid BCP47Tag, it only returns the option that the BCP47 tag
// would return after the "ca-" part
//
// For example:
//   CAL_GREGORIAN would return "gregory".
//   CAL_HIJRI would return "islamic".
// 
// These could be used in a BCP47 tag like this: "en-US-u-ca-gregory".
// Note that there are some NLS calendars that are not supported with the BCP47 U extensions,
// and vice-versa.
// 
// NLS CALID reference:https://docs.microsoft.com/en-us/windows/win32/intl/calendar-identifiers
UChar *getCalendarBCP47FromNLSType(int32_t calendar)
{
    switch(calendar){
        case CAL_GREGORIAN:
        case CAL_GREGORIAN_US:
        case CAL_GREGORIAN_ME_FRENCH:
        case CAL_GREGORIAN_ARABIC:
        case CAL_GREGORIAN_XLIT_ENGLISH:
        case CAL_GREGORIAN_XLIT_FRENCH:
            return u"gregory\0";

        case CAL_JAPAN:
            return u"japanese\0";

        case CAL_TAIWAN:
            return u"roc\0";

        case CAL_KOREA:
            return u"dangi\0";

        case CAL_HIJRI:
            return u"islamic\0";

        case CAL_THAI:
            return u"buddhist\0";

        case CAL_HEBREW:
            return u"hebrew\0";

        case CAL_PERSIAN:
            return u"persian\0";

        case CAL_UMALQURA:
            return u"islamic-umalqura\0";

        default:
            return u"";
    }
}

// Maps from a NLS Alternate sorting system to a BCP47 U extension sorting system.
// 
// We map the alternate sorting method from GetLocaleInfoEx to the sorting method
// used in BCP47 tag with Unicode Extensions.
//
// This does not return a full nor valid BCP47Tag, it only returns the option that the BCP47 tag
// would return after the "co-" part
//
// For example:
//   "phoneb" (parsed from "de-DE_phoneb") would return "phonebk".
//   "radstr" (parsed from "ja-JP_radstr") would return "unihan".
// 
// These could be used in a BCP47 tag like this: "de-DE-u-co-phonebk".
// Note that there are some NLS Alternate sort methods that are not supported with the BCP47 U extensions,
// and vice-versa.
UChar *getSortingSystemBCP47FromNLSType(wchar_t *sortingSystem) 
{
    if (wcscmp(sortingSystem, L"phoneb") == 0) // Phonebook style ordering (such as in German)
    {
        return u"phonebk";
    }
    else if (wcscmp(sortingSystem, L"tradnl") == 0) // Traditional style ordering (such as in Spanish)
    {
        return u"trad";
    }
    else if (wcscmp(sortingSystem, L"stroke") == 0) // Pinyin ordering for Latin, stroke order for CJK characters (used in Chinese)
    {
        return u"stroke";
    }
    else if (wcscmp(sortingSystem, L"radstr") == 0) // Pinyin ordering for Latin, Unihan radical-stroke ordering for CJK characters (used in Chinese)
    {
        return u"unihan";
    }
    else if (wcscmp(sortingSystem, L"pronun") == 0) // Phonetic ordering (sorting based on pronunciation)
    {
        return u"phonetic";
    }
    else 
    {
        return u"";
    }
}

// Maps from a NLS first day of week value to a BCP47 U extension first day of week.
// 
// NLS defines:
// 0 -> Monday, 1 -> Tuesday, ...  5 -> Saturday, 6 -> Sunday
//
// We map the first day of week from GetLocaleInfoEx to the first day of week
// used in BCP47 tag with Unicode Extensions.
//
// This does not return a full nor valid BCP47Tag, it only returns the option that the BCP47 tag
// would return after the "fw-" part
//
// For example:
//   1 (Tuesday) would return "tue".
//   6 (Sunday) would return "sun".
// 
// These could be used in a BCP47 tag like this: "en-US-u-fw-sun".
UChar *getFirstDayBCP47FromNLSType(int32_t firstday) 
{
    switch(firstday){
        case 0:
            return u"mon";

        case 1:
            return u"tue";

        case 2:
            return u"wed";

        case 3:
            return u"thu";

        case 4:
            return u"fri";

        case 5:
            return u"sat";

        case 6:
            return u"sun";

        default:
            return u"";
    }
}

// Maps from a NLS Measurement system to a BCP47 U extension measurement system.
// 
// NLS defines:
// 0 -> Metric system, 1 -> U.S. System
//
// This does not return a full nor valid BCP47Tag, it only returns the option that the BCP47 tag
// would return after the "ms-" part
//
// For example:
//   0 (Metric) would return "metric".
//   6 (U.S. System) would return "ussystem".
// 
// These could be used in a BCP47 tag like this: "en-US-u-ms-metric".
UChar *getMeasureSystemBCP47FromNLSType(int32_t measureSystem) 
{
    switch(measureSystem){
        case 0:
            return u"metric";
        case 1:
            return u"ussystem";
        default:
            return u"";
    }
}

// -------------------------------------------------------
// --------------- END OF MAPPING FUNCTIONS --------------
// -------------------------------------------------------

// -------------------------------------------------------
// ------------------ HELPER FUCTIONS  -------------------
// -------------------------------------------------------
void WstrToUTF8(UChar *dest, const wchar_t* str, size_t cch, UErrorCode* status) 
{
    int32_t i;
    for (i = 0; i <= ARRAYLENGTH(str); i++)
    {
        *(dest + i) = static_cast<UChar>(*(str + i));
    }
    *(dest + i) = '\0';
}

// Although we could use the CRT upper and lower case functions,
// these are sensitive to the global CRT locale, and we need to always have invariant casing.
// Therefore, we use our own toLowercase function.
inline UChar toLowercase(UChar c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return c + 32;
    }
    return c;
}

// Return the CLDR "h12" or "h23" format for the 12 or 24 hour clock.
// NLS only gives us a "time format" of a form similar to "h:mm:ss tt"
// The NLS "h" is 12 hour, and "H" is 24 hour, so we'll scan for the
// first h or H.
// Note that the NLS string could have sections escaped with single
// quotes, so be sure to skip those parts. Eg: "'Hours:' h:mm:ss"
// would skip the "H" in 'Hours' and use the h in the actual pattern.
UChar *get12_or_24hourFormat(wchar_t *hourFormat)
{
    bool isInEscapedString = false;
    for (int i = 0; i < wcslen(hourFormat); i++)
    {
        // Toggle escaped flag if in ' quoted portion
        if (hourFormat[i] == L'\'') 
        {
            isInEscapedString = !isInEscapedString;
        }
        if (!isInEscapedString) 
        {
            // Check for both so we can escape early
            if (hourFormat[i] == L'H') 
            {
                return u"h23";
            }
            if (hourFormat[i] == L'h')
            {
                return u"h12";
            }
        }
    }
    // default to a 24 hour clock as that's more common worldwide
    return u"h23";
}

UErrorCode getUErrorCodeFromLastError()
{
    DWORD error = GetLastError();
    if (error == ERROR_INSUFFICIENT_BUFFER)
    {
        return U_BUFFER_OVERFLOW_ERROR;
    }
    else if (error == ERROR_INVALID_FLAGS)
    {
        return U_ILLEGAL_ARGUMENT_ERROR;
    }
    else if (error == ERROR_INVALID_PARAMETER)
    {
        return U_ILLEGAL_ARGUMENT_ERROR;
    }
    return U_INTERNAL_PROGRAM_ERROR;
}

int32_t GetLocaleInfoExWrapper(LPCWSTR lpLocaleName, LCTYPE LCType, LPWSTR lpLCData, int cchData, UErrorCode* errorCode)
{
    int32_t result = GetLocaleInfoEx(lpLocaleName, LCType, lpLCData, cchData);

    if (result == 0)
    {
        *errorCode = getUErrorCodeFromLastError();
        return UPREFS_API_FAILURE;
    }
    *errorCode = U_ZERO_ERROR;
    return result;
}

// This obtains data from NLS for the given LCTYPE as a wstring for cases such as locale name, sorting method or currency.
// If an error occurs an empty string is returned.
int32_t GetLocaleInfoAsString(wchar_t * dataBuffer, int32_t bufferSize, PCWSTR localeName, LCTYPE type, UErrorCode* status)
{
    int32_t neededBufferSize = GetLocaleInfoExWrapper(localeName, type, nullptr, 0, status);
    RETURN_VALUE_IF(neededBufferSize < 0, -1);
    RETURN_VALUE_IF(dataBuffer == nullptr, neededBufferSize);

    int32_t result = GetLocaleInfoExWrapper(localeName, type, dataBuffer, neededBufferSize, status);
    RETURN_VALUE_IF(result < 0, -1);

    return neededBufferSize;
}

// Get data from GetLocaleInfoEx as an int for cases such as Calendar, First day of the week, and Measurement system
// This only works for LCTYPEs that start with LOCALE_I, such as LOCALE_IFIRSTDAYOFWEEK or LOCALE_ICALENDARTYPE,
// it will not work for LCTYPEs that start with LOCALE_S, such as LOCALE_SNAME or LOCALE_SINTLSYMBOL
// This allows us to then use defined constants such as CAL_GREGORIAN, and avoid unneeded allocations.
int32_t GetLocaleInfoAsInt(PCWSTR localeName, LCTYPE type, UErrorCode* status)
{
    int32_t result = 0;
    int32_t neededBufferSize = GetLocaleInfoExWrapper(localeName, 
                                                      type | LOCALE_RETURN_NUMBER, 
                                                      reinterpret_cast<PWSTR>(&result), 
                                                      sizeof(result) / sizeof(wchar_t), 
                                                      status);

    return result;
}

// Copies a string to a buffer if its size allows it and returns the size.
// The returned needed buffer size includes the terminating \0 null character.
// If the buffer's size is set to 0, the needed buffer size is returned before copying the string.
size_t checkBufferCapacityAndCopy(const UChar& uprefsString, char* uprefsBuffer, size_t bufferSize, UErrorCode* status)
{
    size_t neededBufferSize = u_strlen(&uprefsString) + 1;

    RETURN_VALUE_IF(bufferSize == 0, neededBufferSize);
    RETURN_FAILURE_WITH_STATUS_IF(neededBufferSize > bufferSize, U_BUFFER_OVERFLOW_ERROR);

    u_UCharsToChars(&uprefsString, uprefsBuffer, static_cast<int>(bufferSize));

    return neededBufferSize;
}

int32_t getLocaleBCP47Tag_impl(UChar *languageTag, UErrorCode* status)
{
    // First part of a bcp47 tag looks like an NLS user locale, so we get the NLS user locale.
    int32_t neededBufferSize = GetLocaleInfoAsString(nullptr, 0, LOCALE_NAME_USER_DEFAULT, LOCALE_SNAME, status);
    if(U_FAILURE(*status) || neededBufferSize == -1)
    {
        languageTag = u"";
        return -1;
    }

    wchar_t *NLSLocale = (wchar_t*)malloc(neededBufferSize * sizeof(*NLSLocale));
    int32_t result = GetLocaleInfoAsString(NLSLocale, neededBufferSize, LOCALE_NAME_USER_DEFAULT, LOCALE_SNAME, status);

    if(U_FAILURE(*status) || result == -1)
    {
        free(NLSLocale);
        languageTag = u"";
        return -1;
    }

    // The NLS locale may include a non-default sort, such as de-DE_phoneb. We only want the locale name before the _.
    wchar_t * position = wcsstr(NLSLocale, L"_");
    if (position != nullptr)
    {
        // sacar el substring PENDING
        position = L"\0";
    }

    WstrToUTF8(languageTag, NLSLocale, 0, status);

    free(NLSLocale);
    return 0;
}

UChar *getCalendarSystem_impl(UErrorCode* status)
{
    int32_t NLSCalendar = GetLocaleInfoAsInt(LOCALE_NAME_USER_DEFAULT, LOCALE_ICALENDARTYPE, status);
    RETURN_VALUE_IF(U_FAILURE(*status), u"");

    UChar *calendar(getCalendarBCP47FromNLSType(NLSCalendar));
    RETURN_FAILURE_STRING_WITH_STATUS_IF(u_strlen(calendar) == 0, U_UNSUPPORTED_ERROR);

    return calendar;
}

UChar *getSortingSystem_impl(UErrorCode* status)
{
    // In order to get the sorting system, we need to get LOCALE_SNAME, which appends the sorting system (if any) to the locale
    int32_t neededBufferSize = GetLocaleInfoAsString(nullptr, 0, LOCALE_NAME_USER_DEFAULT, LOCALE_SNAME, status);
    if(U_FAILURE(*status) || neededBufferSize == -1)
    {
        return u"";
    }
    wchar_t *NLSsortingSystem = (wchar_t*)malloc(neededBufferSize * sizeof(*NLSsortingSystem));
    int32_t result = GetLocaleInfoAsString(NLSsortingSystem, neededBufferSize, LOCALE_NAME_USER_DEFAULT, LOCALE_SNAME, status);

    if(U_FAILURE(*status) || result == -1)
    {
        free(NLSsortingSystem);
        return u"";
    }   

    // We use LOCALE_SNAME to get the sorting method (if any). So we need to keep
    // only the sorting bit after the _, removing the locale name.
    // Example: from "de-DE_phoneb" we only want "phoneb"
    wchar_t * startPosition = wcsstr(NLSsortingSystem, L"_");

    // Note: not finding a "_" is not an error, it means the user has not selected an alternate sorting method, which is fine.
    if (startPosition != nullptr) 
    {
        NLSsortingSystem = startPosition + 1;
        UChar *sortingSystem(getSortingSystemBCP47FromNLSType(NLSsortingSystem));

        if(u_strlen(sortingSystem) == 0)
        {
            free(NLSsortingSystem);
            *status = U_UNSUPPORTED_ERROR;
            return u"";
        }
        return sortingSystem;
    }
    return u"";
}

int32_t getCurrencyCode_impl(UChar* currency, UErrorCode* status)
{
    int32_t neededBufferSize = GetLocaleInfoAsString(nullptr, 0, LOCALE_NAME_USER_DEFAULT, LOCALE_SINTLSYMBOL, status);
    if(U_FAILURE(*status) || neededBufferSize == -1)
    {
        currency = u"";
        return -1;
    }
    wchar_t *NLScurrencyData = (wchar_t*)malloc(neededBufferSize * sizeof(*NLScurrencyData));
    int32_t result = GetLocaleInfoAsString(NLScurrencyData, neededBufferSize, LOCALE_NAME_USER_DEFAULT, LOCALE_SINTLSYMBOL, status);
    if(U_FAILURE(*status) || result == -1)
    {
        free(NLScurrencyData);
        currency = u"";
        return -1;
    }   

    WstrToUTF8(currency, NLScurrencyData, 0, status);
    if(u_strlen(currency) == 0)
    {
        free(NLScurrencyData);
        *status = U_INTERNAL_PROGRAM_ERROR;
        currency = u"";
        return -1;
    }

    // Since we retreived the currency code in caps, we need to make it lowercase for it to be in CLDR BCP47 U extensions format.
    for (int i = 0; i < u_strlen(currency); i++)
    {
        currency[i] = toLowercase(currency[i]);
    }

    free(NLScurrencyData);
    return 0;
}

UChar *getFirstDayOfWeek_impl(UErrorCode* status)
{
    int32_t NLSfirstDay = GetLocaleInfoAsInt(LOCALE_NAME_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK, status);
    RETURN_VALUE_IF(U_FAILURE(*status), u"");

    UChar *firstDay(getFirstDayBCP47FromNLSType(NLSfirstDay));
    RETURN_FAILURE_STRING_WITH_STATUS_IF(u_strlen(firstDay) == 0, U_UNSUPPORTED_ERROR);

    return firstDay;
}

UChar *getHourCycle_impl(UErrorCode* status)
{
    int32_t neededBufferSize = GetLocaleInfoAsString(nullptr, 0, LOCALE_NAME_USER_DEFAULT, LOCALE_STIMEFORMAT, status);
    if(U_FAILURE(*status) || neededBufferSize == -1)
    {
        return u"";
    }
    wchar_t *NLShourCycle = (wchar_t*)malloc(neededBufferSize * sizeof(*NLShourCycle));
    int32_t result = GetLocaleInfoAsString(NLShourCycle, neededBufferSize, LOCALE_NAME_USER_DEFAULT, LOCALE_STIMEFORMAT, status);
    if(U_FAILURE(*status) || result == -1)
    {
        free(NLShourCycle);
        return u"";
    }   

    UChar *hourCycle = get12_or_24hourFormat(NLShourCycle);
    if(u_strlen(hourCycle) == 0)
    {
        free(NLShourCycle);
        *status = U_INTERNAL_PROGRAM_ERROR;
        return u"";
    }

    return hourCycle;
}

UChar *getMeasureSystem_impl(UErrorCode* status)
{
    int32_t NLSmeasureSystem = GetLocaleInfoAsInt(LOCALE_NAME_USER_DEFAULT, LOCALE_IMEASURE, status);
    RETURN_VALUE_IF(U_FAILURE(*status), u"");

    UChar *measureSystem(getMeasureSystemBCP47FromNLSType(NLSmeasureSystem));
    RETURN_FAILURE_STRING_WITH_STATUS_IF(u_strlen(measureSystem) == 0, U_UNSUPPORTED_ERROR);

    return measureSystem;
}

void appendIfDataNotEmpty(UChar* dest, const UChar *firstData, const UChar *secondData, bool& warningGenerated, UErrorCode* status)
{
    if(*status == U_UNSUPPORTED_ERROR)
    {
        warningGenerated = true;
    }

    if(u_strlen(secondData) != 0)
    {
        u_strcat(dest, firstData);
        u_strcat(dest, secondData);
    }
}
// -------------------------------------------------------
// --------------- END OF HELPER FUNCTIONS ---------------
// -------------------------------------------------------


// -------------------------------------------------------
// ---------------------- APIs ---------------------------
// -------------------------------------------------------

// Gets the Locale with script and region (if any) set in the user preferences.
// Returns the needed buffer size for the locale, including the terminating \0 null character.
// There is no mapping done in this library between NLS locales and CLDR locale names. That will be done in 
// a future version of the library.
UPREFS_API size_t U_EXPORT2 uprefs_getLocaleBCP47Tag(char* uprefsBuffer, size_t bufferSize, UErrorCode* status)
{
    RETURN_FAILURE_WITH_STATUS_IF(uprefsBuffer == nullptr && bufferSize != 0, U_ILLEGAL_ARGUMENT_ERROR);

    UChar *languageTag = (UChar*)malloc(LOCALE_NAME_MAX_LENGTH);
    int32_t localeResult = getLocaleBCP47Tag_impl(languageTag, status);

    RETURN_VALUE_IF(U_FAILURE(*status) || localeResult == -1, UPREFS_API_FAILURE);

    size_t result = checkBufferCapacityAndCopy(*languageTag, uprefsBuffer, bufferSize, status);
    free(languageTag);
    return result;
}

// Gets the calendar set in the user preferences.
// This API does not get a valid BCP47 Tag, it only gets the option for the user setting in a CLDR BCP47 U extensions format.
// Example: Instead of getting "en-US-u-ca-gregory", it will get only "gregory"
// Returns the needed buffer size for the user preference, including the terminating \0 null character. 
UPREFS_API size_t U_EXPORT2 uprefs_getCalendarSystem(char* uprefsBuffer, size_t bufferSize, UErrorCode* status)
{
    RETURN_FAILURE_WITH_STATUS_IF(uprefsBuffer == nullptr && bufferSize != 0, U_ILLEGAL_ARGUMENT_ERROR);

    UChar *calendar = getCalendarSystem_impl(status);

    RETURN_FAILURE_WITH_STATUS_IF(*status == U_UNSUPPORTED_ERROR, U_UNSUPPORTED_ERROR);
    RETURN_VALUE_IF(U_FAILURE(*status), UPREFS_API_FAILURE);

    return checkBufferCapacityAndCopy(*calendar, uprefsBuffer, bufferSize, status);
}


// Gets sorting system set in the user preferences (if any) 
// If there is no sorting system set, uprefsBuffer will be empty.
// This API does not get a valid BCP47 Tag, it only gets the option for the user setting in a CLDR BCP47 U extensions format.
// Example: Instead of getting "de-DE-u-co-phoneb", it will get only "phoneb"
// Returns the needed buffer size for the user preference, including the terminating \0 null character.
UPREFS_API size_t U_EXPORT2 uprefs_getSortingSystem(char* uprefsBuffer, size_t bufferSize, UErrorCode *status)
{
    RETURN_FAILURE_WITH_STATUS_IF(uprefsBuffer == nullptr && bufferSize != 0, U_ILLEGAL_ARGUMENT_ERROR);

    UChar *sortingSystem = getSortingSystem_impl(status);

    RETURN_FAILURE_WITH_STATUS_IF(*status == U_UNSUPPORTED_ERROR, U_UNSUPPORTED_ERROR);
    RETURN_VALUE_IF(U_FAILURE(*status), UPREFS_API_FAILURE);

    if(u_strlen(sortingSystem) == 0 && bufferSize != 0)
    {
        uprefsBuffer[0] = '\0';
        return 1;
    }
    return checkBufferCapacityAndCopy(*sortingSystem, uprefsBuffer, bufferSize, status);
}

// Gets the currency set in the user preferences.
// This API does not get a valid BCP47 Tag, it only gets the option for the user setting in a CLDR BCP47 U extensions format.
// Example: Instead of getting "en-US-u-cu-usd", it will get only "usd"
// Returns the needed buffer size for the user preference, including the terminating \0 null character. 
UPREFS_API size_t U_EXPORT2 uprefs_getCurrencyCode(char* uprefsBuffer, size_t bufferSize, UErrorCode* status)
{
    // Calls GetLocaleInfoExWrapper to get the needed buffer size, and then to retreive the LOCALE_SINTLSYMBOL
    // for the currency code, that will be in all caps and checks for errors on both calls.
    RETURN_FAILURE_WITH_STATUS_IF(uprefsBuffer == nullptr && bufferSize != 0, U_ILLEGAL_ARGUMENT_ERROR);

    // Currencies have a maximum length of 3, so we only need to allocate 4 for the null terminator.
    UChar *currency = (UChar*)malloc(4);
    int32_t currencyResult = getCurrencyCode_impl(currency, status);

    RETURN_VALUE_IF(U_FAILURE(*status) || currencyResult == -1, UPREFS_API_FAILURE);
    
    size_t result = checkBufferCapacityAndCopy(*currency, uprefsBuffer, bufferSize, status);
    free(currency);
    return result;
}

// Gets the first day of the week set in the user preferences.
// This API does not get a valid BCP47 Tag, it only gets the option for the user setting in a CLDR BCP47 U extensions format.
// Example: Instead of getting "en-US-u-fw-mon", it will get only "mon"
// Returns the needed buffer size for the user preference, including the terminating \0 null character. 
UPREFS_API size_t U_EXPORT2 uprefs_getFirstDayOfWeek(char* uprefsBuffer, size_t bufferSize, UErrorCode *status)
{
    RETURN_FAILURE_WITH_STATUS_IF(uprefsBuffer == nullptr && bufferSize != 0, U_ILLEGAL_ARGUMENT_ERROR);

    UChar *firstDay = getFirstDayOfWeek_impl(status);

    RETURN_FAILURE_WITH_STATUS_IF(*status == U_UNSUPPORTED_ERROR, U_UNSUPPORTED_ERROR);
    RETURN_VALUE_IF(U_FAILURE(*status), UPREFS_API_FAILURE);

    return checkBufferCapacityAndCopy(*firstDay, uprefsBuffer, bufferSize, status);
}

// Gets the hour cycle set in the user preferences.
// This API does not get a valid BCP47 Tag, it only gets the option for the user setting in a CLDR BCP47 U extensions format.
// Example: Instead of getting "en-US-u-hc-h12", it will get only "h12"
// Returns the needed buffer size for the user preference, including the terminating \0 null character. 
UPREFS_API size_t U_EXPORT2 uprefs_getHourCycle(char* uprefsBuffer, size_t bufferSize, UErrorCode *status)
{
    // Calls GetLocaleInfoExWrapper to get the needed buffer size, and then to retreive the LOCALE_STIMEFORMAT
    // for the hour cycle, which will be a string with the user chosen format, and we need to map it to either 'h23' or 'h12'.
    RETURN_FAILURE_WITH_STATUS_IF(uprefsBuffer == nullptr && bufferSize != 0, U_ILLEGAL_ARGUMENT_ERROR);

    UChar *hourCycle = getHourCycle_impl(status);

    RETURN_FAILURE_WITH_STATUS_IF(*status == U_UNSUPPORTED_ERROR, U_UNSUPPORTED_ERROR);
    RETURN_VALUE_IF(U_FAILURE(*status), UPREFS_API_FAILURE);

    return checkBufferCapacityAndCopy(*hourCycle, uprefsBuffer, bufferSize, status);
}

// Gets the measurement system set in the user preferences.
// This API does not get a valid BCP47 Tag, it only gets the option for the user setting in a CLDR BCP47 U extensions format.
// Example: Instead of getting "en-US-u-ms-ussystem", it will get only "ussystem" 
// Returns the needed buffer size for the user preference, including the terminating \0 null character. 
UPREFS_API size_t U_EXPORT2 uprefs_getMeasureSystem(char* uprefsBuffer, size_t bufferSize, UErrorCode *status)
{
    RETURN_FAILURE_WITH_STATUS_IF(uprefsBuffer == nullptr && bufferSize != 0, U_ILLEGAL_ARGUMENT_ERROR);

    UChar *measureSystem = getMeasureSystem_impl(status);

    RETURN_FAILURE_WITH_STATUS_IF(*status == U_UNSUPPORTED_ERROR, U_UNSUPPORTED_ERROR);
    RETURN_VALUE_IF(U_FAILURE(*status), UPREFS_API_FAILURE);

    return checkBufferCapacityAndCopy(*measureSystem, uprefsBuffer, bufferSize, status);
}

// Gets the valid and canonical BCP47 tag with the user settings for Language, Calendar, Sorting, Currency,
// First day of week, Hour cycle, and Measurement system.
// Calls all of the other APIs
// Returns the needed buffer size for the BCP47 Tag. 
UPREFS_API size_t U_EXPORT2 uprefs_getBCP47Tag(char* uprefsBuffer, size_t bufferSize, UErrorCode* status)
{
    RETURN_FAILURE_WITH_STATUS_IF(uprefsBuffer == nullptr && bufferSize != 0, U_ILLEGAL_ARGUMENT_ERROR);

    UChar *BCP47Tag = (UChar*)malloc(LOCALE_NAME_MAX_LENGTH * sizeof(UChar*));
    bool warningGenerated = false;

    UChar *languageTag = (UChar*)malloc(LOCALE_NAME_MAX_LENGTH);
    int32_t localeBCP47Result = getLocaleBCP47Tag_impl(languageTag, status);
    FREE_AND_RETURN_VALUE_IF(U_FAILURE(*status) || localeBCP47Result == -1, UPREFS_API_FAILURE, languageTag);
    u_strcpy(BCP47Tag, languageTag);
    u_strcat(BCP47Tag, u"-u");
    free(languageTag);


    UChar *calendar = getCalendarSystem_impl(status);
    RETURN_VALUE_IF(U_FAILURE(*status) && *status != U_UNSUPPORTED_ERROR, UPREFS_API_FAILURE);
    appendIfDataNotEmpty(BCP47Tag, u"-ca-", calendar, warningGenerated, status);
    
    UChar *sortingSystem = getSortingSystem_impl(status);
    RETURN_VALUE_IF(U_FAILURE(*status) && *status != U_UNSUPPORTED_ERROR, UPREFS_API_FAILURE);
    appendIfDataNotEmpty(BCP47Tag, u"-co-", sortingSystem, warningGenerated, status);

    UChar *currency = (UChar *)malloc(4 * sizeof(UChar *));
    size_t currencyResult = getCurrencyCode_impl(currency, status);
    RETURN_VALUE_IF(U_FAILURE(*status) && *status != U_UNSUPPORTED_ERROR || currencyResult == -1, UPREFS_API_FAILURE);
    appendIfDataNotEmpty(BCP47Tag, u"-cu-", currency, warningGenerated, status);
    free(currency);

    UChar *firstDay = getFirstDayOfWeek_impl(status);
    RETURN_VALUE_IF(U_FAILURE(*status) && *status != U_UNSUPPORTED_ERROR, UPREFS_API_FAILURE);
    appendIfDataNotEmpty(BCP47Tag, u"-fw-", firstDay, warningGenerated, status);

    UChar *hourCycle = getHourCycle_impl(status);
    RETURN_VALUE_IF(U_FAILURE(*status) && *status != U_UNSUPPORTED_ERROR, UPREFS_API_FAILURE);
    appendIfDataNotEmpty(BCP47Tag, u"-hc-", hourCycle, warningGenerated, status);

    UChar *measureSystem = getMeasureSystem_impl(status);
    RETURN_VALUE_IF(U_FAILURE(*status) && *status != U_UNSUPPORTED_ERROR, UPREFS_API_FAILURE);
    appendIfDataNotEmpty(BCP47Tag, u"-ms-", measureSystem, warningGenerated, status);

    if(warningGenerated)
    {
        *status = U_USING_FALLBACK_WARNING;
    }

    size_t result = checkBufferCapacityAndCopy(*BCP47Tag, uprefsBuffer, bufferSize, status);
    free(BCP47Tag);
    return result;
}

// -------------------------------------------------------
// ---------------------- END OF APIs --------------------
// -------------------------------------------------------