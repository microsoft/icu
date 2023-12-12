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

constexpr int32_t c_maxKeyLength = 4;
constexpr int32_t c_maxKeyLengthInHex = c_maxKeyLength * 5; // "XXXX " per codepoint, null-terminated.

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
    : fLocale(locale), fStrength(strength) {
    // Lookup resource bundle.
    fBundle = ures_open(U_ICUDATA_COLFOLD, fLocale.getName(), &status);
    if (U_FAILURE(status)) {
        ures_close(fBundle);
        return;
    }
    fBundle = ures_getByKeyWithFallback(fBundle, "collationFoldings", fBundle, &status);
    if (U_FAILURE(status)) {
        ures_close(fBundle);
        return;
    }
    fBundle = ures_getByKeyWithFallback(fBundle, strength_to_string(fStrength), fBundle, &status);
    if (U_FAILURE(status)) {
        ures_close(fBundle);
        return;
    }

    // Don't call unorm2_close() when using unorm2_getNFDInstance().
    fNFDNormalizer = unorm2_getNFDInstance(&status);
    if (U_FAILURE(status)) {
        return;
    }
}

CollationFolding::~CollationFolding() {
    ures_close(fBundle);
}

UnicodeString CollationFolding::replace_discontiguous_contraction(StringCharacterIterator& iter, UnicodeString hex, UErrorCode& status)
{
    // Find discontiguous contraction.
    UnicodeString finalValue;
    UChar32 curr = iter.current32();
    uint8_t currCC = u_getCombiningClass(curr);
    uint8_t maxCC = 0;
    int32_t startIndex = iter.getIndex();
    while (iter.hasNext()) {
        UChar32 next = iter.next32PostInc();
        if (next == CharacterIterator::DONE) {
            iter.setIndex32(startIndex);
            break;
        }

        uint8_t nextCC = u_getCombiningClass(next);
        if (nextCC != 0) {
            // S2.1.1: next is a non-starter codepoint: process it.
            if (nextCC > maxCC) {
                // S2.1.2: next is an unblocked non-starter. Find if there is a collation folding match with next appended.
                maxCC = nextCC;
                UnicodeString nextHexKey = hex + UnicodeString(" ") + toHexString(next, status);
                if (U_FAILURE(status)) {
                    break;
                }
                
                char key[c_maxKeyLengthInHex];
                nextHexKey.extract(0, nextHexKey.length(), key, sizeof(key));
                int32_t len{};
                const UChar* value = ures_getStringByKeyWithFallback(fBundle, key, &len, &status);
                if (status == U_MISSING_RESOURCE_ERROR) {
                    // No match with next appended.
                    status = U_ZERO_ERROR;
                    continue;
                }

                finalValue = UnicodeString(value);
                
                UnicodeString currText;
                iter.getText(currText);
                int32_t currIndex = iter.getIndex();
                // S2.1.3: Replace S with S + C. Remove C.
                UnicodeString newText = UnicodeString(currText, 0, startIndex) + 
                                        UnicodeString(next) + 
                                        UnicodeString(currText, startIndex, currIndex - startIndex - 1) +
                                        UnicodeString(currText, currIndex, currText.length() - currIndex);
                iter.setText(newText);
                startIndex++; // Move index past C.
                iter.setIndex32(startIndex);
                hex = nextHexKey;
                break; // TODO: could be contraction with > 1 combining chars.
            }
        } else {
            iter.setIndex32(startIndex);
            break;
        }
    }
    return finalValue;
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

    // Determine the source length.
    if (sourceLength == -1) {
        sourceLength = u_strlen(source);
    }
    if (sourceLength <= 0) {
        return u_terminateUChars(destination, destinationCapacity, 0, &status);
    }

    // Normalize source string.
    int32_t nfdLen = unorm2_normalize(fNFDNormalizer, source, sourceLength, nullptr, 0, &status);
    LocalArray<UChar> nfdSource(new UChar[nfdLen + 1]);
    if (status == U_BUFFER_OVERFLOW_ERROR) {
        status = U_ZERO_ERROR;
        unorm2_normalize(fNFDNormalizer, source, sourceLength, nfdSource.getAlias(), nfdLen + 1, &status);
        if (U_FAILURE(status)) {
            return 0;
        }
    } else if (U_FAILURE(status)) {
        return 0;
    }

    UnicodeString result;
    StringCharacterIterator iter(UnicodeString(nfdSource.getAlias()));
    bool isPrevCGJ = false;
    while (iter.hasNext()) {
        // Build up hex key.
        UnicodeString hex;
        UChar32 firstCodepoint;
        int32_t maxKeyLength;
        for (maxKeyLength = 0; maxKeyLength < std::min(c_maxKeyLength, iter.getLength()); maxKeyLength++) {
            UChar32 c = iter.next32PostInc();
            if (c == CharacterIterator::DONE) {
                // The length of the remainder of the string < maxKeyLength.
                // Attempt to get the longest match of the remaining string.
                break;
            }
            if (maxKeyLength == 0) {
                hex = toHexString(c, status);
                firstCodepoint = c;
            } else {
                hex += UnicodeString(" ") + toHexString(c, status);
            }
            if (U_FAILURE(status)) {
                return 0;
            }
        }

        // Check for longest matching key.
        int32_t keyLength;
        for (keyLength = maxKeyLength; keyLength > 0; keyLength--) {
            char key[c_maxKeyLengthInHex];
            hex.extract(0, hex.length(), key, sizeof(key));
            int32_t len{};
            const UChar* value = ures_getStringByKeyWithFallback(fBundle, key, &len, &status);
            if (status == U_MISSING_RESOURCE_ERROR) {
                status = U_ZERO_ERROR;
                if (keyLength > 1) {
                    // No mapping exists for this key length. Try smaller key length.
                    hex = hex.remove(hex.lastIndexOf(UnicodeString(" ")));
                    continue;
                }

                // A missing collation folding mapping for a key of length 1 implies that the key maps to itself.

                // Move iterator back to the last non-matching index.
                iter.move32(-(maxKeyLength-keyLength), CharacterIterator::EOrigin::kCurrent);
                
                // Need to check for discontiguous contraction here.
                UnicodeString finalValue = replace_discontiguous_contraction(iter, hex, status);
                if (U_FAILURE(status)) {
                    return 0;
                }
                if (finalValue.length() == 0) {
                    finalValue = firstCodepoint;
                }

                for (int32_t i = 0; i < finalValue.length(); i++) {
                    if (!isPrevCGJ || finalValue[i] != u'\x034F') {
                        result.append(finalValue[i]);
                    }
                    isPrevCGJ = (finalValue[i] == u'\x034F');
                }

                break;
            }
            else if (U_FAILURE(status)) {
                return 0;
            }

            // Mapping found at current key length.

            // Move iterator back to the last non-matching index.
            iter.move32(-(maxKeyLength-keyLength), CharacterIterator::EOrigin::kCurrent);
            
            // Need to check for discontiguous contraction here.
            UnicodeString finalValue = replace_discontiguous_contraction(iter, hex, status);
            if (U_FAILURE(status)) {
                return 0;
            }
            if (finalValue.length() == 0) {
                finalValue = UnicodeString(value);
            }

            // Consecutive CGJ characters (U+034F) are ignored after the first one.
            for (int32_t i = 0; i < finalValue.length(); i++) {
                if (!isPrevCGJ || finalValue[i] != u'\x034F') {
                    result.append(finalValue[i]);
                }
                isPrevCGJ = (finalValue[i] == u'\x034F');
            }
            break;
        }
    }

    return result.extract(destination, destinationCapacity, status);
}

/*
*  C APIs for CollationFolding
*/
U_CAPI UCollationFolding* U_EXPORT2
ucolfold_open(const char* locale, UCollationStrength strength, UErrorCode* status) {
    if (U_FAILURE(*status)) {
        return nullptr;
    }

    CollationFolding *collationFolding = new CollationFolding(locale, strength, *status);
    if (collationFolding == nullptr) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return nullptr;
    }
    if (U_FAILURE(*status)) {
        delete collationFolding;
        return nullptr;
    }

    return collationFolding->toUCollationFolding();
}

U_CAPI void U_EXPORT2 
ucolfold_close(UCollationFolding* ucolfold) {
    if (ucolfold != nullptr) {
        delete CollationFolding::fromUCollationFolding(ucolfold);
    }
}

U_CAPI int32_t U_EXPORT2 
ucolfold_fold(const UCollationFolding* ucolfold, const UChar* source, int32_t sourceLength, UChar* destination, int32_t destinationCapacity, UErrorCode* status) {
    if (U_FAILURE(*status)) {
        return 0;
    }

    const CollationFolding *collationFolding = CollationFolding::fromUCollationFolding(ucolfold);
    return const_cast<CollationFolding*>(collationFolding)->fold(source, sourceLength, destination, destinationCapacity, *status);
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_COLLATION */