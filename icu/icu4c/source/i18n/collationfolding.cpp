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
#include "charstr.h"
#include "ustr_imp.h"

U_NAMESPACE_BEGIN

constexpr const char* strength_to_string(UCollationStrength value) noexcept
{
    switch (value)
    {
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
    if (U_FAILURE(status)) {
        return 0;
    }

    if ((sourceLength < -1 || (sourceLength != 0 && source == nullptr)) || 
        (destinationCapacity < 0 || (destinationCapacity != 0 && destination == nullptr))) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    LocalUResourceBundlePointer localeBundle(ures_open(nullptr, fLocale.getName(), &status));
    if (U_FAILURE(status)) {
        return 0;
    }
    LocalUResourceBundlePointer colfBundle(
        ures_getByKey(localeBundle.getAlias(), strength_to_string(fStrength), nullptr, &status));
    if (U_FAILURE(status)) {
        return 0;
    }

    int32_t destIndex = 0;

    //CharString result;
    for (int32_t i = 0; i < sourceLength; i++) {
        CharString key;
        UChar32 c;
        U16_NEXT(source, i, sourceLength, c);
        key.appendNumber(c, status);
        if (U_FAILURE(status)) {
            return 0;
        }

        int32_t len = key.length();
        const UChar* mappedValue = ures_getStringByKey(colfBundle.getAlias(), key.data(), &len, &status);
        if (U_FAILURE(status)) {
            return 0;
        }

        // Simple case for initial test
        if (destIndex < destinationCapacity) {
            destination[destIndex++] = *mappedValue;
        } else if (destIndex == destinationCapacity) {
            status = U_STRING_NOT_TERMINATED_WARNING;
        } else {
            status = U_BUFFER_OVERFLOW_ERROR;
        }
    }

    return u_terminateUChars(destination, destinationCapacity, destIndex, &status);

    //result.extract(destination, destinationCapacity, status);
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