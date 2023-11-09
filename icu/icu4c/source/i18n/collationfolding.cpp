// 2023 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"
#include "unicode/utf16.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucollationfolding.h"
#include "unicode/collationfolding.h"
#include "unicode/coll.h"
#include "unicode/resbund.h"
#include "unicode/unistr.h"
#include "unicode/ustring.h"
#include "charstr.h"
#include "ustr_imp.h"
#include "uresimp.h"
#include "cstring.h"

U_NAMESPACE_BEGIN

constexpr const char* strength_to_string(UCollationStrength value) noexcept {
    switch (value) {
    case UCollationStrength::UCOL_PRIMARY:
        return "primary";
    case UCollationStrength::UCOL_SECONDARY:
        return "secondary";
    case UCollationStrength::UCOL_TERTIARY:
        return "tertiary";
    case UCollationStrength::UCOL_QUATERNARY:
        return "quaternary";
    case UCollationStrength::UCOL_IDENTICAL:
        return "identical";
    default:
        return "unknown";
    }
}

constexpr char16_t to_hex_digit(uint8_t value) noexcept {
    if (value > 0xF) {
        value = static_cast<uint8_t>(value & 0x0F);
    }
    if (value < 0xA) {
        return u'0' + static_cast<char16_t>(value);
    }
    else {
        return u'A' + static_cast<char16_t>(value - 0xA);
    }
}

UnicodeString toHexString(UChar32 codepoint, UErrorCode& status) {
    UnicodeString result;

    if (codepoint <= u'\uFFFF') {
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF000) >> 12));
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF00) >> 8));
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF0) >> 4));
        result += to_hex_digit(static_cast<uint8_t>(codepoint & 0xF));
    }
    else if (codepoint <= U'\U000FFFFF') {
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF0000) >> 16));
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF000) >> 12));
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF00) >> 8));
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF0) >> 4));
        result += to_hex_digit(static_cast<uint8_t>(codepoint & 0xF));
    }
    else if (codepoint <= U'\U0010FFFF') {
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF00000) >> 20));
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF0000) >> 16));
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF000) >> 12));
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF00) >> 8));
        result += to_hex_digit(static_cast<uint8_t>((codepoint & 0xF0) >> 4));
        result += to_hex_digit(static_cast<uint8_t>(codepoint & 0xF));
    }
    else {
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }

    return result;
}

CollationFolding::CollationFolding(const Locale& locale, UCollationStrength strength, UErrorCode& status)
    : fLocale(locale), fStrength(strength) {}

CollationFolding::~CollationFolding() {}

CollationFolding*
CollationFolding::createInstance(const Locale& locale, UCollationStrength strength, UErrorCode& status)
{
    if (U_FAILURE(status)) {
        return nullptr;
    }

    LocalPointer<CollationFolding> cf(new CollationFolding(locale, strength, status));
    if (U_FAILURE(status)) {
        return nullptr;
    }

    return cf.orphan();
}

int32_t
CollationFolding::fold(const UChar* source, int32_t sourceLength, UChar* destination, int32_t destinationCapacity, UErrorCode& status)
{
    // Check input parameters.
    if (U_FAILURE(status)) {
        return 0;
    }
    if ((sourceLength < -1 || (sourceLength != 0 && source == nullptr)) || 
        (destinationCapacity < 0 || (destinationCapacity != 0 && destination == nullptr))) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    // Lookup resource bundle.
    LocalUResourceBundlePointer localeBundle(ures_open(U_ICUDATA_COLF, fLocale.getName(), &status));
    if (U_FAILURE(status)) {
        return 0;
    }
    LocalUResourceBundlePointer colfBundle(ures_getByKey(localeBundle.getAlias(), "collationFoldings", nullptr, &status));
    if (U_FAILURE(status)) {
        return 0;
    }
    LocalUResourceBundlePointer mappingBundle(ures_getByKey(colfBundle.getAlias(), strength_to_string(fStrength), nullptr, &status));
    if (U_FAILURE(status)) {
        return 0;
    }
    
    // Determine the source length.
    if (sourceLength == -1) {
        sourceLength = u_strlen(source);
    }
    if (sourceLength <= 0) {
        return u_terminateUChars(destination, destinationCapacity, 0, &status);
    }
    
    // Walk the source string.
    UnicodeString result;
    int32_t sourceIndex = 0;
    while (sourceIndex < sourceLength) {
        UChar32 c;
        U16_NEXT(source, sourceIndex, sourceLength, c);
        UnicodeString hex = toHexString(c, status);
        
        char key[100];
        int32_t len = hex.extract(0, hex.length(), key, 100);
        
        const UChar *value = ures_getStringByKeyWithFallback(mappingBundle.getAlias(), key, &len, &status);
        if (status == U_MISSING_RESOURCE_ERROR) {
            // A missing collation folding mapping implies that the key maps to itself.
            result.append(c);
            status = U_ZERO_ERROR;
            continue;
        }
        else if (U_FAILURE(status)) {
            return 0;
        }

        // Mapping found!
        for (int32_t i = 0; i < u_strlen(value); i++) {
            result.append(value[i]);
        }
    }

    return result.extract(destination, destinationCapacity, status);
}

/*
*  C APIs for CollationFolding
*/ 
U_CAPI UCollationFolding* U_EXPORT2
ucolf_open(const char* locale, UCollationStrength strength, UErrorCode* status)
{
    if (U_FAILURE(*status)) {
        return nullptr;
    }

    UCollationFolding *result = nullptr;
    CollationFolding *coll = CollationFolding::createInstance(locale, strength, *status);
    if (U_SUCCESS(*status)) {
        result = coll->toUCollationFolding();
    }

    return result;
}

U_CAPI void U_EXPORT2 
ucolf_close(UCollationFolding* ucolf)
{
    if (ucolf != nullptr) {
        delete CollationFolding::fromUCollationFolding(ucolf);
    }
}

U_CAPI int32_t U_EXPORT2 
ucolf_fold(const UCollationFolding* ucolf, const UChar* source, int32_t sourceLength, UChar* destination, int32_t destinationCapacity, UErrorCode* status)
{
    if (U_FAILURE(*status)) {
        return 0;
    }

    const CollationFolding *cf = CollationFolding::fromUCollationFolding(ucolf);
    return const_cast<CollationFolding*>(cf)->fold(source, sourceLength, destination, destinationCapacity, *status);
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_COLLATION */