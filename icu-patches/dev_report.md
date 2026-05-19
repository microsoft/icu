# ICU Upgrade Dev Report

Generated: 2026-05-19 09:01:27

## 1. Version Summary

| Field | Value |
|-------|-------|
| ICU Version | 78.3.0.0 |
| Upstream Hash | `21d1eb0f30...` |

## 2. Source Changes

Diff from `icu72-pre-swap` to HEAD (icu/ directory only):

```
 6919 files changed, 693820 insertions(+), 545058 deletions(-)
```

### Top file types changed

| Extension | Count |
|-----------|-------|
| .txt | 5117 |
| .cpp | 799 |
| .h | 552 |
| .c | 105 |
| .vcxproj | 77 |
| .json | 46 |
| .filters | 30 |
| .xml | 29 |
| .java | 28 |
| .in | 25 |

## 3. Patch Audit Results

*No audit report found in `icu-patches`. Final patch records are summarized from `icu-patches/patches` instead.*

- Latest patch-record commit: `aebb85791c4 Refresh ICU 78 patch records`
- Patch files present: 19
- Refreshed ICU 78 patch records: `002`, `017`, `018`, `020`, `022`
- Validation used for refreshed records: `git apply --cached --check --reverse` against the final ICU index

## 4. Build Results

| Config | Status | Log |
|--------|--------|-----|
| Debug-ARM64 | ✅ PASS | build-Debug-ARM64.log |
| Debug-Win32 | ✅ PASS | build-Debug-Win32.log |
| Debug-x64 | ✅ PASS | build-Debug-x64.log |
| Release-ARM64 | ✅ PASS | build-Release-ARM64.log |
| Release-Win32 | ✅ PASS | build-Release-Win32.log |
| Release-x64 | ✅ PASS | build-Release-x64.log |

## 5. Test Results

| Suite | Status | Log |
|-------|--------|-----|
| cintltst-Release-x64 | ✅ PASS | test-cintltst-Release-x64.log |
| intlRelease-x64 | ✅ PASS | test-intltest-Release-x64.log |
| ioRelease-x64 | ✅ PASS | test-iotest-Release-x64.log |

## 6. Action Items

- [x] Patch conflicts resolved and ICU 78 patch records refreshed
- [x] CLDR data rebuild consumed by ICU data generation
- [x] Timezone data check/regeneration completed
- [x] Full Stage 8 build/test pass completed
- [x] Refreshed dev report generated
- [ ] Create PR/PR stack (owner-handled)

