// 2023 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef COLLATIONFOLDING_H
#define COLLATIONFOLDING_H

#include "unicode/utypes.h"
#include "unicode/ucol.h"
#include "unicode/resbund.h"
#include "cmemory.h"

U_NAMESPACE_BEGIN

class U_I18N_API CollationFolding : public UMemory {
  public:
    CollationFolding(const Locale& locale, UCollationStrength strength, UErrorCode& status);
    ~CollationFolding();

    int32_t fold(const UChar* source, int32_t sourceLength, UChar* destination, int32_t destinationCapacity, UErrorCode& status);
    
#ifndef U_HIDE_INTERNAL_API
    /** @internal */
    static inline CollationFolding *fromUCollationFolding(UCollationFolding *uc) {
        return reinterpret_cast<CollationFolding *>(uc);
    }
    /** @internal */
    static inline const CollationFolding *fromUCollationFolding(const UCollationFolding *uc) {
        return reinterpret_cast<const CollationFolding *>(uc);
    }
    /** @internal */
    inline UCollationFolding *toUCollationFolding() {
        return reinterpret_cast<UCollationFolding *>(this);
    }
    /** @internal */
    inline const UCollationFolding *toUCollationFolding() const {
        return reinterpret_cast<const UCollationFolding *>(this);
    }
#endif  // U_HIDE_INTERNAL_API

  private:
      Locale fLocale;
      UCollationStrength fStrength = UCOL_PRIMARY;
      UResourceBundle* fMappingBundle;
};

U_NAMESPACE_END

#endif // COLLATIONFOLDING_H