// 2023 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef UCOLLATIONFOLDING_H
#define UCOLLATIONFOLDING_H

#include "unicode/utypes.h"
#include "unicode/ucol.h"
#include "unicode/udata.h"

#define U_ICUDATA_COLFOLD U_ICUDATA_NAME U_TREE_SEPARATOR_STRING "colfold"

/** A collation folding instance.
*   For usage in C programs.
*/
struct UCollationFolding;
/** structure representing a collation folding object instance 
 *  @draft ICU 75
 */
typedef struct UCollationFolding UCollationFolding;

/** 
 * Constructs a UCollationFolding instance for the given locale and strength. 
 * 
 * @param locale The locale to use for collation rules. Only locales that have data 
 *               for string search are supported. An implicit "-u-co-search" is
 *               appended to the locale name before looking up the data.
 *
 *               Special values for locales can be passed in - if NULL is passed
 *               for the locale, the default locale will be used. If an empty
 *               string ("") or "root" is passed, the root collator will be used.  
 * 
 * @param strength The collation strength; only UCOL_PRIMARY, UCOL_SECONDARY, 
 *                 UCOL_TERTIARY, and UCOL_DEFAULT are supported. UCOL_DEFAULT 
 *                 means UCOL_PRIMARY. Other collation strengths are not supported.  
 * 
 * @param status The error code, set if an error occurred while creating the  
 *               UCollationFolding instance. The values U_USING_FALLBACK_WARNING and 
 *               U_USING_DEFAULT_WARNING are reported to indicate whether specialized 
 *               string search data for the given input locale (-u-co-search) exists. 
 *
 * @return The newly created UCollationFolding instance.
 * @see UCollationStrength
 * @draft ICU 75 
 */ 
U_CAPI UCollationFolding* U_EXPORT2
ucolfold_open(const char* locale, UCollationStrength strength, UErrorCode* status);
/** 
 * Close a UCollationFolding instance, releasing the memory used.
 * Once closed, it should not be used. 
 * 
 * @param ucolfold The UCollationFolding instance to close. 
 * @draft ICU 75 
 */ 
U_CAPI void U_EXPORT2
ucolfold_close(UCollationFolding* ucolfold); 

/** 
 * Return the equivalence class string that the source string folds to, for the 
 * input UCollationFolding instance. 
 * 
 * @param ucolfold The UCollationFolding instance to use. 
 * @param source The source string. 
 * @param sourceLength The length of the source string, or -1 if NULL-terminated. 
 * @param destination A pointer to a buffer to receive the NULL-terminated output. If
 *                 the output fits into destination but cannot be NULL-terminated
 *                 (length == destinationCapacity) then the error code is set to
 *                 U_STRING_NOT_TERMINATED_WARNING. If the output doesn't fit into
 *                 destination then the error code is set to U_BUFFER_OVERFLOW_ERROR.
 * @param destinationCapacity The maximum size of the destination buffer. 
 * @return The actual buffer size needed for the destination string. If greater
 *    	 than destinationCapacity, the returned destination string will be
 *    	 truncated and an error code will be returned. 
 * @draft ICU 75 
 */ 
U_CAPI int32_t U_EXPORT2
ucolfold_fold(const UCollationFolding* ucolfold, const UChar* source, int32_t sourceLength, UChar* destination, int32_t destinationCapacity, UErrorCode* status); 

#if U_SHOW_CPLUSPLUS_API
#include "unicode/localpointer.h"

U_NAMESPACE_BEGIN 
 
/** 
 * \class LocalUCollationFoldingPointer 
 * "Smart pointer" class, closes a UCollationFolding via ucolfold_close(). 
 * For most methods see the LocalPointerBase base class. 
 * 
 * @see LocalPointerBase 
 * @see LocalPointer 
 * @draft ICU 75 
 */ 
U_DEFINE_LOCAL_OPEN_POINTER(LocalUCollationFoldingPointer, UCollationFolding, ucolfold_close); 

U_NAMESPACE_END 
 
#endif // U_SHOW_CPLUSPLUS_API

#endif // UCOLLATIONFOLDING_H