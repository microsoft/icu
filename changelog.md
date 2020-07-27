# Changelog

## ICU 67.1.0.2

Data changes:
- Microsoft data changes for the Taiwan calendar. [#8](https://github.com/microsoft/icu/pull/8).

## ICU 67.1.0.1

Changes/modifications compared to the upstream `maint/maint-67` branch.

#### General changes:
- Microsoft internal data changes.
- Additional locales from CLDR to improve parity with existing Windows NLS locale support.
- Update timezone data files to tz2020a
  - Uses files from https://github.com/unicode-org/icu-data/tree/master/tzdata/icunew/2020a/44

#### Windows OS build-related:
- Changes needed for the Windows OS build of ICU.
- Make `u_cleanup` a no-op for Windows OS ICU.
- Conditionally set data file name based on `ICU_DATA_DIR_WINDOWS` to support Windows OS build with only a single data file.
- Don't use the extended ICU data package for Windows OS build.
- Add macro (`FORCE_DISABLE_UMUTEX_CONSTEXPR`) to control toggling the constexpr of the UMutex class.

#### Changes cherry-picked from upstream tickets/PRs:

ICU-21078 Adding script and updating docs for CLDR jars
- https://unicode-org.atlassian.net/browse/ICU-21078
- https://github.com/unicode-org/icu/commit/b0fb4839c24b743a6786b8c6b3a7e066478f0bad
- https://github.com/unicode-org/icu/pull/1113

ICU-21078 Adding missing copyright notice (sorry!)
- https://unicode-org.atlassian.net/browse/ICU-21078
- https://github.com/unicode-org/icu/commit/bf9421f8e48abf5455cc9fa11d69c015c34eebb4
- https://github.com/unicode-org/icu/pull/1121

ICU-21089 Ignoring incomplete alt path mappings.
- https://unicode-org.atlassian.net/browse/ICU-21089
- https://github.com/unicode-org/icu/commit/ef91cc3673d69a5e00407cda94f39fcda3131451
- https://github.com/unicode-org/icu/pull/1130

ICU-21099 udat_toCalendarDateField, handle all UDateFormatFields and out of range
- https://unicode-org.atlassian.net/browse/ICU-21099
- https://github.com/unicode-org/icu/commit/d39899350daf6d1dc03b4bb9f9dc530c718420f2
- https://github.com/unicode-org/icu/pull/1135 

ICU-21102 Fix broken builds on Windows when using a pre-built data file.
- https://unicode-org.atlassian.net/browse/ICU-21102
- https://github.com/unicode-org/icu/commit/82a5959b863ac647fce3bcc5804ed906fb3b4bc0
- https://github.com/unicode-org/icu/pull/1139

ICU-21081 Make U_ASSERT C++14 compatible
- https://unicode-org.atlassian.net/browse/ICU-21081
- https://github.com/unicode-org/icu/commit/715d254a02b0b22681cb6f861b0921ae668fa7d6
- https://github.com/unicode-org/icu/pull/1132

ICU-21134 Copy additional data when toNumberFormatter is used
- https://unicode-org.atlassian.net/browse/ICU-21134
- https://github.com/unicode-org/icu/commit/3ff6627ce68e5849aff9129b416197a22676654a
- https://github.com/unicode-org/icu/pull/1156

ICU-21140 Make UTF-8 explicit for all file access.
- https://unicode-org.atlassian.net/browse/ICU-21140
- https://github.com/unicode-org/icu/commit/a29369b5867947a3a3d9423903b810c39e1a40bd
- https://github.com/unicode-org/icu/pull/1163 

ICU-21075 Initialization/deinitialization order of UMutex global statics is not defined with VS2017
- https://unicode-org.atlassian.net/browse/ICU-21075
- https://github.com/unicode-org/icu/pull/1110
