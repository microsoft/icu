# Changelog

## ICU 72.1.0.1

Changes/modifications compared to the upstream `maint/maint-72` branch.

#### General changes:
- Microsoft internal data changes.
- Additional locales from CLDR to improve parity with existing Windows NLS locale support.

#### Windows OS build-related:
- Changes needed for the Windows OS build of ICU.
- Make `u_cleanup` a no-op for Windows OS ICU.
- Conditionally set data file name based on `ICU_DATA_DIR_WINDOWS` to support Windows OS build with only a single data file.
- Don't use the extended ICU data package for Windows OS build.
- Add uprefs library to ICU to obtain the default locale as a full BCP47 tag
- Update tzdb to 2022g