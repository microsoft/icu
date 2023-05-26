# Changelog
## ICU 68.2.0.9
#### Misc changes:
- Migrate from PackageES build agents to public Microsoft hosted agents [#113](https://github.com/microsoft/icu/pull/113)
- Fix crash if ICU default locale has BCP47 extensions. Fix ures_openDirect crash with NULL locale. [#110](https://github.com/microsoft/icu/pull/110)
- Fix LocaleBuilder errors when ICU default locale has BCP47 Unicode extension tags. [#111](https://github.com/microsoft/icu/pull/111)

## ICU 68.2.0.8
#### Data changes:
- Update ICU data files. [#106](https://github.com/microsoft/icu/pull/106)

#### Misc changes:
- Update ant from 1.10.9 to 1.10.11 [#107](https://github.com/microsoft/icu/pull/107)

## ICU 68.2.0.7
#### Misc changes:
- Port work-around for Ubuntu 18.04 Azure CI build bot breaking builds. [#103](https://github.com/microsoft/icu/pull/103)

#### Data changes:
- Add South Sudan (Juba) to Windows time zone mapping. [#102](https://github.com/microsoft/icu/pull/102)

#### Changes cherry-picked from upstream tickets/PRs:
ICU-21591 Release lock in SimpleDateFormat::tzFormat in case of failure
- https://unicode-org.atlassian.net/browse/ICU-21591
- https://github.com/unicode-org/icu/pull/1701

## ICU 68.2.0.6
#### Misc changes:
- Migration to ESRP Code Signing from PackageES Code Signing. [#93](https://github.com/microsoft/icu/pull/93)
- Add Valgrind to CI build pipeline. [#95](https://github.com/microsoft/icu/pull/95)

#### Changes cherry-picked from upstream tickets/PRs:
ICU-21587 Fix memory bug w/ baseName
- https://unicode-org.atlassian.net/browse/ICU-21587
- https://github.com/unicode-org/icu/pull/1698

## ICU 68.2.0.5
#### Data changes:
- Add extra locales from CLDR-MS based on NLS "MS-Only" culture data. [#91](https://github.com/microsoft/icu/pull/91)
- Update ff-Latn-MR region name. [#88](https://github.com/microsoft/icu/pull/88)

#### Misc changes:
- Update guava from 27.0-jre to 30.0-jre [#87](https://github.com/microsoft/icu/pull/87)

## ICU 68.2.0.4
#### Data changes:
- Update number grouping separator for French locales. [#80](https://github.com/microsoft/icu/pull/80)
- Picked up additional tzdata fixes. [#81](https://github.com/microsoft/icu/pull/81)
#### Changes cherry-picked from upstream tickets/PRs:

ICU-21041 Fix fuzzer memory read error.
- https://unicode-org.atlassian.net/browse/ICU-21041
- https://github.com/unicode-org/icu/pull/1622

ICU-21537 Fix invalid free by long locale name
- https://unicode-org.atlassian.net/browse/ICU-21537
- https://github.com/unicode-org/icu/pull/1656

## ICU 68.2.0.3
#### General changes:
- Add support for prebuilt Linux ARM64 binaries in the MS-ICU Nuget package. [#70](https://github.com/microsoft/icu/pull/70)
- Picked up tzdata 2021a updates. [#76](https://github.com/microsoft/icu/pull/76)

#### Changes cherry-picked from upstream tickets/PRs:

CLDR-14476 fa, remove explicit <LRM> or replace with \u200E; ko, remove a bogus <
- https://unicode-org.atlassian.net/browse/CLDR-14476
- https://github.com/unicode-org/cldr/pull/1025

ICU-21492 Fix regex compile assertion failure.
- https://unicode-org.atlassian.net/browse/ICU-21492
- https://github.com/unicode-org/icu/pull/1577

ICU-21461 uprops.h: remove unused gc macros
- https://unicode-org.atlassian.net/browse/ICU-21461
- https://github.com/unicode-org/icu/pull/1555

ICU-21521 Fix cast of uprv_strcmp
- https://unicode-org.atlassian.net/browse/ICU-21521
- https://github.com/unicode-org/icu/pull/1618

## ICU 68.2.0.2
#### Changes cherry-picked from upstream tickets/PRs:

ICU-21465 Windows Time Zone offset is wrong when Automatic DST setting is OFF
- https://unicode-org.atlassian.net/browse/ICU-21465
- https://github.com/unicode-org/icu/pull/1539

ICU-21449 Infinite loop can occur with locale IDs that contain RES_PATH_SEPARATOR
- https://unicode-org.atlassian.net/browse/ICU-21449
- https://github.com/unicode-org/icu/pull/1549

## ICU 68.2.0.1

Changes/modifications compared to the upstream `maint/maint-68` branch.

#### General changes:
- Microsoft internal data changes.
- Additional locales from CLDR to improve parity with existing Windows NLS locale support.

#### Windows OS build-related:
- Changes needed for the Windows OS build of ICU.
- Make `u_cleanup` a no-op for Windows OS ICU.
- Conditionally set data file name based on `ICU_DATA_DIR_WINDOWS` to support Windows OS build with only a single data file.
- Don't use the extended ICU data package for Windows OS build.

#### Changes cherry-picked from upstream tickets/PRs:

ICU-21427 Don't ignore already checked-in files under "tools/cldr/lib"
- https://unicode-org.atlassian.net/browse/ICU-21427
- https://github.com/unicode-org/icu/pull/1500

ICU-21118 check that dst and src are not null in uprv_memcpy
- https://unicode-org.atlassian.net/browse/ICU-21118
- https://github.com/unicode-org/icu/pull/1489
