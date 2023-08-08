// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
#include "uprefstest.h"
#if U_PLATFORM_USES_ONLY_WIN32_API && UCONFIG_USE_WINDOWS_PREFERENCES_LIBRARY

#define ARRAY_SIZE 512

    std::wstring UPrefsTest::language = L"";
    std::wstring UPrefsTest::currency = L"";
    std::wstring UPrefsTest::hourCycle = L"";
    int32_t UPrefsTest::firstday = 0;
    int32_t UPrefsTest::measureSystem = 0;
    CALID UPrefsTest::calendar = 0;

void UPrefsTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite UPrefsTest: ");
    TESTCASE_AUTO_BEGIN;
    TESTCASE_AUTO(TestGetDefaultLocaleAsBCP47Tag);
    TESTCASE_AUTO(TestBCP47TagWithSorting);
    TESTCASE_AUTO(TestBCP47TagChineseSimplified);
    TESTCASE_AUTO(TestBCP47TagChineseSortingStroke);
    TESTCASE_AUTO(TestBCP47TagJapanCalendar);
    TESTCASE_AUTO(TestUseNeededBuffer);
    TESTCASE_AUTO(TestGetNeededBuffer);
    TESTCASE_AUTO(TestGetUnsupportedSorting);
    TESTCASE_AUTO(Get24HourCycleMixed);
    TESTCASE_AUTO(Get12HourCycleMixed);
    TESTCASE_AUTO(Get12HourCycleMixed2);
    TESTCASE_AUTO(Get12HourCycle);
    TESTCASE_AUTO(Get12HourCycle2);
    TESTCASE_AUTO_END;
}

int32_t UPrefsTest::MockGetLocaleInfoEx(LPCWSTR lpLocaleName, LCTYPE LCType, LPWSTR lpLCData, int cchData, UErrorCode* status)
{
    switch (LCType)
    {
    case LOCALE_SNAME:
        if (cchData == 0)
        {
            *status = U_ZERO_ERROR;
            return language.length() + 1;
        }

        if (language.length() + 1 > cchData)
        {
            *status = U_BUFFER_OVERFLOW_ERROR;
            return 0;
        }
        wcsncpy(lpLCData, language.c_str(), cchData);
        *status = U_ZERO_ERROR;
        return language.length();

    case LOCALE_ICALENDARTYPE | LOCALE_RETURN_NUMBER:
        if (cchData == 0)
        {
            *status = U_ZERO_ERROR;
            return 2;
        }
        if (cchData < 2)
        {
            *status = U_BUFFER_OVERFLOW_ERROR;
            return 0;
        }
        *(reinterpret_cast<int32_t*>(lpLCData)) = calendar;
        *status = U_ZERO_ERROR;
        return 2;

    case LOCALE_SINTLSYMBOL:
        if (cchData == 0)
        {
            *status = U_ZERO_ERROR;
            return currency.length() + 1;
        }
        if (currency.length() + 1 > cchData)
        {
            *status = U_BUFFER_OVERFLOW_ERROR;
            return 0;
        }
        wcsncpy(lpLCData, currency.c_str(), cchData);
        *status = U_ZERO_ERROR;
        return currency.length();

    case LOCALE_IFIRSTDAYOFWEEK | LOCALE_RETURN_NUMBER:
        if (cchData == 0)
        {
            *status = U_ZERO_ERROR;
            return 2;
        }
        if (cchData < 2)
        {
            *status = U_BUFFER_OVERFLOW_ERROR;
            return 0;
        }

        *(reinterpret_cast<int32_t*>(lpLCData)) = firstday;
        *status = U_ZERO_ERROR;
        return 2;

    case LOCALE_STIMEFORMAT:
        if (cchData == 0)
        {
            *status = U_ZERO_ERROR;
            return hourCycle.length() + 1;
        }
        
        if (hourCycle.length() + 1 > cchData)
        {
            *status = U_BUFFER_OVERFLOW_ERROR;
            return 0;
        }
        wcsncpy(lpLCData, hourCycle.c_str(), cchData);
        *status = U_ZERO_ERROR;
        return 0;
    
    case LOCALE_IMEASURE | LOCALE_RETURN_NUMBER:
        if (cchData == 0)
        {
            *status = U_ZERO_ERROR;
            return 2;
        }

        if (cchData < 2)
        {
            *status = U_BUFFER_OVERFLOW_ERROR;
            return 0;
        }
        *(reinterpret_cast<int32_t*>(lpLCData)) = measureSystem;
        *status = U_ZERO_ERROR;
        return 2;

    default:
        *status = U_INTERNAL_PROGRAM_ERROR;
        return 0;
    }
}

// The code above is independent of the library itself, but for the code below this point,
// we need to include the library to be able to use the definitions of the API uprefs_getBCP47Tag
#include "uprefs.cpp"

void UPrefsTest::TestGetDefaultLocaleAsBCP47Tag()
{
    char languageBuffer[ARRAY_SIZE] = {0};
    language = L"en-US";
    currency = L"USD";
    hourCycle = L"HH:mm:ss";
    firstday = 0;
    measureSystem = 1;
    calendar = CAL_GREGORIAN;
    UErrorCode status = U_ZERO_ERROR;
    const char* expectedValue = "en-US-u-ca-gregory-cu-usd-fw-mon-hc-h23-ms-ussystem";
    
    if ( uprefs_getBCP47Tag(languageBuffer, ARRAY_SIZE, &status) != 52)
    {
        errln("Expected length to be 52, but got: %d\n",uprv_strlen(languageBuffer));
    }
    if ( uprv_strcmp(expectedValue, languageBuffer) != 0)
    {
        errln("Expected BCP47Tag to be %s, but got: %s\n", expectedValue, languageBuffer);
    }
}

void UPrefsTest::TestBCP47TagWithSorting()
{
    char languageBuffer[ARRAY_SIZE] = {0};
    language = L"de-DE_phoneb";
    currency = L"EUR";
    hourCycle = L"HH:mm:ss";
    firstday = 0;
    measureSystem = 1;
    calendar = CAL_GREGORIAN;
    UErrorCode status = U_ZERO_ERROR;
    char* expectedValue = "de-DE-u-ca-gregory-co-phonebk-cu-eur-fw-mon-hc-h23-ms-ussystem";
    
    if ( uprefs_getBCP47Tag(languageBuffer, ARRAY_SIZE, &status) != 63)
    {
        errln("Expected length to be 63, but got: %d\n",uprv_strlen(languageBuffer));
    }
    if ( uprv_strcmp(expectedValue, languageBuffer) != 0)
    {
        errln("Expected BCP47Tag to be %s, but got: %s\n", expectedValue, languageBuffer);
    }
}

void UPrefsTest::TestBCP47TagChineseSimplified()
{
    char languageBuffer[ARRAY_SIZE] = {0};
    language = L"zh-Hans-HK";
    currency = L"EUR";
    hourCycle = L"hh:mm:ss";
    firstday = 2;
    measureSystem = 1;
    calendar = CAL_GREGORIAN;
    UErrorCode status = U_ZERO_ERROR;
    char* expectedValue = "zh-Hans-HK-u-ca-gregory-cu-eur-fw-wed-hc-h12-ms-ussystem";
    
    if ( uprefs_getBCP47Tag(languageBuffer, ARRAY_SIZE, &status) != 57)
    {
        errln("Expected length to be 57, but got: %d\n",uprv_strlen(languageBuffer));
    }
    if ( uprv_strcmp(expectedValue, languageBuffer) != 0)
    {
        errln("Expected BCP47Tag to be %s, but got: %s\n", expectedValue, languageBuffer);
    }
}

void UPrefsTest::TestBCP47TagChineseSortingStroke()
{
    char languageBuffer[ARRAY_SIZE] = {0};
    language = L"zh-SG_stroke";
    currency = L"EUR";
    hourCycle = L"hh:mm:ss";
    firstday = 2;
    measureSystem = 0;
    calendar = CAL_GREGORIAN;
    UErrorCode status = U_ZERO_ERROR;
    char* expectedValue = "zh-SG-u-ca-gregory-co-stroke-cu-eur-fw-wed-hc-h12-ms-metric";
    
    if ( uprefs_getBCP47Tag(languageBuffer, ARRAY_SIZE, &status) != 60)
    {
        errln("Expected length to be 60, but got: %d\n",uprv_strlen(languageBuffer));
    }
    if ( uprv_strcmp(expectedValue, languageBuffer) != 0)
    {
        errln("Expected BCP47Tag to be %s, but got: %s\n", expectedValue, languageBuffer);
    }
}

void UPrefsTest::TestBCP47TagJapanCalendar()
{
    char languageBuffer[ARRAY_SIZE] = {0};
    language = L"ja-JP";
    currency = L"MXN";
    hourCycle = L"hh:mm:ss";
    firstday = 1;
    measureSystem = 0;
    calendar = CAL_JAPAN;
    UErrorCode status = U_ZERO_ERROR;
    char* expectedValue = "ja-JP-u-ca-japanese-cu-mxn-fw-tue-hc-h12-ms-metric";
    
    if ( uprefs_getBCP47Tag(languageBuffer, ARRAY_SIZE, &status) != 51)
    {
        errln("Expected length to be 51, but got: %d\n",uprv_strlen(languageBuffer));
    }
    if ( uprv_strcmp(expectedValue, languageBuffer) != 0)
    {
        errln("Expected BCP47Tag to be %s, but got: %s\n", expectedValue, languageBuffer);
    }
}

void UPrefsTest::TestUseNeededBuffer()
{
    char languageBuffer[ARRAY_SIZE] = {0};
    language = L"ja-JP";
    currency = L"MXN";
    hourCycle = L"hh:mm:ss";
    firstday = 1;
    measureSystem = 0;
    calendar = CAL_THAI;
    UErrorCode status = U_ZERO_ERROR;
    char* expectedValue = "ja-JP-u-ca-buddhist-cu-mxn-fw-tue-hc-h12-ms-metric";

    int32_t neededBufferSize = uprefs_getBCP47Tag(nullptr, 0, &status);
    
    if ( uprefs_getBCP47Tag(languageBuffer, neededBufferSize, &status) != 51)
    {
        errln("Expected length to be 51, but got: %d\n",uprv_strlen(languageBuffer));
    }
    if ( uprv_strcmp(expectedValue, languageBuffer) != 0)
    {
        errln("Expected BCP47Tag to be %s, but got: %s\n", expectedValue, languageBuffer);
    }
}

void UPrefsTest::TestGetNeededBuffer()
{
    char languageBuffer[ARRAY_SIZE] = {0};
    language = L"zh-SG_stroke";
    currency = L"MXN";
    hourCycle = L"hh:mm:ss";
    firstday = 1;
    measureSystem = 0;
    calendar = CAL_THAI;
    UErrorCode status = U_ZERO_ERROR;
    char* expectedValue = "zh-SG-u-ca-buddhist-co-stroke-cu-mxn-fw-tue-hc-h12-ms-metric";

    int32_t neededBufferSize = uprefs_getBCP47Tag(nullptr, 0, &status);

    if ( neededBufferSize != 61)
    {
        errln("Expected buffer size to be 61, but got: %d\n",uprv_strlen(languageBuffer));
    }
    if ( uprefs_getBCP47Tag(languageBuffer, neededBufferSize, &status) != 61)
    {
        errln("Expected length to be 61, but got: %d\n",uprv_strlen(languageBuffer));
    }
    if ( uprv_strcmp(expectedValue, languageBuffer) != 0)
    {
        errln("Expected BCP47Tag to be %s, but got: %s\n", expectedValue, languageBuffer);
    }
}

void UPrefsTest::TestGetUnsupportedSorting()
{
    char languageBuffer[ARRAY_SIZE] = {0};
    language = L"hu-HU_technl";
    currency = L"MXN";
    hourCycle = L"hh:mm:ss";
    firstday = 1;
    measureSystem = 0;
    calendar = CAL_THAI;
    UErrorCode status = U_ZERO_ERROR;
    char* expectedValue = "hu-HU-u-ca-buddhist-cu-mxn-fw-tue-hc-h12-ms-metric";

    if ( uprefs_getBCP47Tag(languageBuffer, ARRAY_SIZE, &status) != 51)
    {
        errln("Expected length to be 51, but got: %d\n",uprv_strlen(languageBuffer));
    }
    if ( uprv_strcmp(expectedValue, languageBuffer) != 0)
    {
        errln("Expected BCP47Tag to be %s, but got: %s\n", expectedValue, languageBuffer);
    }
}

void UPrefsTest::Get24HourCycleMixed() 
{
    char languageBuffer[ARRAY_SIZE] = {0};
    language = L"ja-JP";
    currency = L"MXN";
    hourCycle = L"HHhh:mm:ss";
    firstday = 1;
    measureSystem = 0;
    calendar = CAL_THAI;
    UErrorCode status = U_ZERO_ERROR;
    char* expectedValue = "ja-JP-u-ca-buddhist-cu-mxn-fw-tue-hc-h23-ms-metric";

    if (uprefs_getBCP47Tag(languageBuffer, ARRAY_SIZE, &status) != 51)
    {
        errln("Expected length to be 51, but got: %d\n", uprv_strlen(languageBuffer));
    }
    if (uprv_strcmp(expectedValue, languageBuffer) != 0)
    {
        errln("Expected BCP47Tag to be %s, but got: %s\n", expectedValue, languageBuffer);
    }
}

void UPrefsTest::Get12HourCycleMixed()
{
    char languageBuffer[ARRAY_SIZE] = {0};
    language = L"ja-JP";
    currency = L"MXN";
    hourCycle = L"hHhH:mm:ss";
    firstday = 1;
    measureSystem = 0;
    calendar = CAL_THAI;
    UErrorCode status = U_ZERO_ERROR;
    char* expectedValue = "ja-JP-u-ca-buddhist-cu-mxn-fw-tue-hc-h12-ms-metric";

    if (uprefs_getBCP47Tag(languageBuffer, ARRAY_SIZE, &status) != 51)
    {
        errln("Expected length to be 51, but got: %d\n", uprv_strlen(languageBuffer));
    }
    if (uprv_strcmp(expectedValue, languageBuffer) != 0)
    {
        errln("Expected BCP47Tag to be %s, but got: %s\n", expectedValue, languageBuffer);
    }
}


void UPrefsTest::Get12HourCycleMixed2()
{
    char languageBuffer[ARRAY_SIZE] = {0};
    language = L"ja-JP";
    currency = L"MXN";
    hourCycle = L"hH''h'H'H:mm:ss";
    firstday = 1;
    measureSystem = 0;
    calendar = CAL_THAI;
    UErrorCode status = U_ZERO_ERROR;
    char* expectedValue = "ja-JP-u-ca-buddhist-cu-mxn-fw-tue-hc-h12-ms-metric";

    if (uprefs_getBCP47Tag(languageBuffer, ARRAY_SIZE, &status) != 51)
    {
        errln("Expected length to be 51, but got: %d\n", uprv_strlen(languageBuffer));
    }
    if (uprv_strcmp(expectedValue, languageBuffer) != 0)
    {
        errln("Expected BCP47Tag to be %s, but got: %s\n", expectedValue, languageBuffer);
    }
}

void UPrefsTest::Get12HourCycle()
{
    char languageBuffer[ARRAY_SIZE] = {0};
    language = L"ja-JP";
    currency = L"MXN";
    hourCycle = L"h'H'h:mm:ss";
    firstday = 1;
    measureSystem = 0;
    calendar = CAL_THAI;
    UErrorCode status = U_ZERO_ERROR;
    char* expectedValue = "ja-JP-u-ca-buddhist-cu-mxn-fw-tue-hc-h12-ms-metric";

    if (uprefs_getBCP47Tag(languageBuffer, ARRAY_SIZE, &status) != 51)
    {
        errln("Expected length to be 51, but got: %d\n", uprv_strlen(languageBuffer));
    }
    if (uprv_strcmp(expectedValue, languageBuffer) != 0)
    {
        errln("Expected BCP47Tag to be %s, but got: %s\n", expectedValue, languageBuffer);
    }
}

void UPrefsTest::Get12HourCycle2()
{
    char languageBuffer[ARRAY_SIZE] = {0};
    language = L"ja-JP";
    currency = L"MXN";
    hourCycle = L"'H'h'H'h:mm:ss";
    firstday = 1;
    measureSystem = 0;
    calendar = CAL_THAI;
    UErrorCode status = U_ZERO_ERROR;
    char* expectedValue = "ja-JP-u-ca-buddhist-cu-mxn-fw-tue-hc-h12-ms-metric";

    if (uprefs_getBCP47Tag(languageBuffer, ARRAY_SIZE, &status) != 51)
    {
        errln("Expected length to be 51, but got: %d\n", uprv_strlen(languageBuffer));
    }
    if (uprv_strcmp(expectedValue, languageBuffer) != 0)
    {
        errln("Expected BCP47Tag to be %s, but got: %s\n", expectedValue, languageBuffer);
    }
}
#endif //U_PLATFORM_USES_ONLY_WIN32_API && UCONFIG_USE_WINDOWS_PREFERENCES_LIBRARY