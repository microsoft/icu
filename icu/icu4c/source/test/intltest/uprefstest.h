#ifndef UPREFSTEST_H
#define UPREFSTEST_H

#include "unicode/platform.h"
#if U_PLATFORM_USES_ONLY_WIN32_API
#define UPREFS_TEST 1


#include "windows.h"
#include "intltest.h"
#include "uprefs.h"

class UPrefsTest: public IntlTest {
public:
    static std::wstring language;
    static std::wstring currency;
    static std::wstring hourCycle;
    static int32_t firstday;
    static int32_t measureSystem;
    static CALID calendar;
    UPrefsTest(){};
    virtual ~UPrefsTest(){};

    virtual void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par = NULL) override;
    int32_t MockGetLocaleInfoEx(LPCWSTR lpLocaleName, LCTYPE LCType, LPWSTR lpLCData, int cchData, UErrorCode* status);
    void TestGetBCP47Tag1();
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


#endif //U_PLATFORM_USES_ONLY_WIN32_API
#endif //UPREFSTEST_H