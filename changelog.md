# Changelog

## ICU 68.1.0.1

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
