# Changelog

## ICU 64.2.0.1

Changes/modifications compared to the upstream `maint/maint-64` branch.

#### General changes:
- Microsoft internal data changes.
- Additional locales from CLDR to improve parity with existing Windows NLS locale support.
- Update timezone data files to tz2019a
  - Uses files from https://github.com/unicode-org/icu-data/tree/master/tzdata/icunew/2019a/44.

#### Windows OS build-related:
- Changes needed for the Windows OS build of ICU.
- Make u_cleanup a no-op for Windows OS ICU
- Conditionally set data file name based on ICU_DATA_DIR_WINDOWS to support Windows OS build with only a single data file.
- Don't use the extended ICU data package for Windows OS build.
- Add macro (FORCE_DISABLE_UMUTEX_CONSTEXPR) to control toggling the constexpr of the UMutex class.

#### Changes cherry-picked from upstream tickets/PRs:

Build Clean in VS doesn't fully clean everything that it should
- https://unicode-org.atlassian.net/browse/ICU-13456
- https://github.com/unicode-org/icu/commit/71ee05ed17e83d62f097467632c228e2447b5fde

UInitOnce, make sure all instances are initialized.
- https://unicode-org.atlassian.net/browse/ICU-20570
- https://github.com/unicode-org/icu/commit/a97cfb01b917125fdb9705cf79786188cebb6e2e

Locale::getKeywords() and Locale::getUnicodeKeywords() segfault on empty
- https://unicode-org.atlassian.net/browse/ICU-20573
- https://github.com/unicode-org/icu/commit/711e7e003a4e0a38d58baa07e64ecb52e9a71776

ICU headers cannot be built within extern "C" scope
- https://unicode-org.atlassian.net/browse/ICU-20530
- https://github.com/unicode-org/icu/commit/0aa19c0d22af72a9b708b6285bbb1deb6cebcb62

UMutex, static construction & destruction
- https://unicode-org.atlassian.net/browse/ICU-20520
- https://github.com/unicode-org/icu/commit/b772241b52fcec07b1909d44d156fbcd0cbfb20c

Need to make TimeZone::AdoptDefault thread safe
- https://unicode-org.atlassian.net/browse/ICU-20595
- https://github.com/unicode-org/icu/commit/cb40d8b1a5eb6dd01e3972db5e1a05b672202f80

ICU4C: Guard C++ public headers with C_SHOW_CPLUSPLUS_API
- https://unicode-org.atlassian.net/browse/ICU-20578
- https://github.com/unicode-org/icu/commit/a5bbd505d7a5f350409b42ebe075e24a4f2c63ce

Missing </ClCompile> closing tag in the file "intltest.vcxproj.filters"
- https://unicode-org.atlassian.net/browse/ICU-20613
- https://github.com/unicode-org/icu/commit/b7ffd7b6d0d5fa7b8fbd850362b7165a89c6f3c2

u_cleanup() should close OS level mutexes.
- https://unicode-org.atlassian.net/browse/ICU-20588
- https://github.com/unicode-org/icu/commit/afa9b9b48e0a50f2298e5d47c101ac3325629d8c

DateTimePatternGenerator test fails when running with Valgrind
- https://unicode-org.atlassian.net/browse/ICU-20625
- https://github.com/unicode-org/icu/commit/f9ea5351b0a308b57ab951eea6b1136134a610fd

Top-level .gitignore contains incorrect entries
- https://unicode-org.atlassian.net/browse/ICU-20527
- https://github.com/unicode-org/icu/commit/5b4befd67c2b99bc805091b6efa120c3fdd50268

RegexCompile::compile crash with (?<![?&&?]?)
- https://unicode-org.atlassian.net/browse/ICU-20544
- https://github.com/unicode-org/icu/commit/7053363323ca3bf2a8853058f20c5e1c6b7e5024
- https://github.com/unicode-org/icu/commit/bdb1806580702f05d7bac627f367c12d45183796

Regex Failures with nested look-around expressions
- https://unicode-org.atlassian.net/browse/ICU-20391
- https://github.com/unicode-org/icu/commit/d685cacd9b020569bfad61869cea87a7d45bda87

Fix Windows build failures with long paths: Use PowerShell when command length exceeds CMD's limit.
- https://unicode-org.atlassian.net/browse/ICU-20555
- https://github.com/unicode-org/icu/commit/c28505caaaefe6848be7fc760da04da2383a4f57

Fix typo in API doc comments about unumf_openForSkeletonAndLocale.
- https://unicode-org.atlassian.net/browse/ICU-20721
- https://github.com/unicode-org/icu/pull/713

UPRV_UNREACHABLE called in code that is easily reachable, crashes production code
- https://unicode-org.atlassian.net/browse/ICU-20680
- https://github.com/unicode-org/icu/pull/780
- https://github.com/unicode-org/icu/pull/787

Fix typo in API doc comments in ucurr.h
- https://unicode-org.atlassian.net/browse/ICU-20794
- https://github.com/unicode-org/icu/pull/785

Fix memory leak in SimpleDateFormat::adoptCalendar, delete calendarToAdopt upon error
- https://unicode-org.atlassian.net/browse/ICU-20799
- https://github.com/unicode-org/icu/pull/790

OOM not handled in NumberFormatterImpl::macrosToMicroGenerator
- https://unicode-org.atlassian.net/browse/ICU-20368
- https://github.com/unicode-org/icu/pull/795
- https://github.com/unicode-org/icu/commit/543495da740441b4c89e585d55c8eb532b4cc595

OOM not handled in uloc_openKeywordList, need to use LocalMemory
- https://unicode-org.atlassian.net/browse/ICU-20802
- https://github.com/unicode-org/icu/pull/796
- https://github.com/unicode-org/icu/pull/805

OOM not handled in selectForMask
- https://unicode-org.atlassian.net/browse/ICU-20804
- https://github.com/unicode-org/icu/pull/811

Disable optimization for MSVC on ARM64 on versions below 16.4 to fix crashes
- https://unicode-org.atlassian.net/browse/ICU-20907
- https://github.com/unicode-org/icu/pull/926

ICU-20845 UMutex not trivially but constexpr constructible #870 
- https://unicode-org.atlassian.net/browse/ICU-20845
- https://github.com/unicode-org/icu/pull/870
- https://github.com/unicode-org/icu/commit/e5381c956b73b236c5d7866b2a6b65f879e770ea

ICU-20958 Prevent SEGV_MAPERR in UnicodeString::doAppend()
- https://unicode-org.atlassian.net/browse/ICU-20958
- https://github.com/unicode-org/icu/pull/971
- https://github.com/unicode-org/icu/commit/b7d08bc04a4296982fcef8b6b8a354a9e4e7afca

Warn on global/static constructors
- https://unicode-org.atlassian.net/browse/ICU-20493
- https://github.com/unicode-org/icu/pull/708
- https://github.com/unicode-org/icu/commit/fc487bf32b880298890161446e26d1077b672f74

Initialization/deinitialization order of UMutex global statics is not defined with VS2017
- https://unicode-org.atlassian.net/browse/ICU-21075 
- https://github.com/unicode-org/icu/pull/1110

