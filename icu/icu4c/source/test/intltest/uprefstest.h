// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
#ifndef UPREFSTEST_H
#define UPREFSTEST_H

#include "unicode/platform.h"
#if U_PLATFORM_USES_ONLY_WIN32_API && UCONFIG_USE_WINDOWS_PREFERENCES_LIBRARY
// We define UPREFS_TEST to use the mock version of GetLocaleInfoEx(), which
// allows us to simulate its behaviour and determine if the results given by the 
// API align with what we expect to receive
#define UPREFS_TEST 1


#include "windows.h"
#include "intltest.h"
#include "uprefs.h"

class UPrefsTest: public IntlTest {
private:
    static std::wstring language;
    static std::wstring currency;
    static std::wstring hourCycle;
    static int32_t firstday;
    static int32_t measureSystem;
    static CALID calendar;

public:
    UPrefsTest(){};
    virtual ~UPrefsTest(){};

    virtual void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par = NULL) override;
    int32_t MockGetLocaleInfoEx(LPCWSTR lpLocaleName, LCTYPE LCType, LPWSTR lpLCData, int cchData, UErrorCode* status);
    void TestGetDefaultLocaleAsBCP47Tag();
    void TestBCP47TagWithSorting();
    void TestBCP47TagChineseSimplified();
    void TestBCP47TagChineseSortingStroke();
    void TestBCP47TagJapanCalendar();
    void TestUseNeededBuffer();
    void TestGetNeededBuffer();
    void TestGetUnsupportedSorting();
    void Get24HourCycleMixed();
    void Get12HourCycleMixed();
    void Get12HourCycleMixed2();
    void Get12HourCycle();
    void Get12HourCycle2();   
};


#endif //U_PLATFORM_USES_ONLY_WIN32_API && UCONFIG_USE_WINDOWS_PREFERENCES_LIBRARY
#endif //UPREFSTEST_H