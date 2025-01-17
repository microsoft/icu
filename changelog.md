# Changelog
## ICU 72.1.0.3

- Disable the dynamic plug-in loading due to security concerns
- Add changes to emit debugging information to PDB/object files.
- Add changes to enable COMDAT folding and elimination when linking.

## ICU 72.1.0.2

- Restore NNBSP with ASCII space for English locales
- Add en_CA official date format
- Revert treating of @ as ALetter for word break

## ICU 72.1.0.1

Updated collations for zh locale required for GB18030-2022 certification.

## ICU 72.1.0.0

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