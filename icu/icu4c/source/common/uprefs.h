#include "unicode/utypes.h"
#include "unicode/umachine.h"
#include "unicode/ustring.h"

#define UPREFS_API U_CFUNC U_EXPORT

/**
* Gets the valid and canonical BCP47 tag with the user settings for Language, Calendar, Sorting, Currency,
* First day of week, Hour cycle, and Measurement system when available.
*
* @param uprefsBuffer Pointer to a buffer in which this function retrieves the requested locale information.
*                     This pointer is not used if bufferSize is set to 0.
* @param bufferSize Size, in characters, of the data buffer indicated by uprefsBuffer. Alternatively, the application
*                   can set this parameter to 0. In this case, the function does not use the uprefsBuffer parameter
*                   and returns the required buffer size, including the terminating null character.
* @param status: Pointer to a UErrorCode. The resulting value will be U_ZERO_ERROR if the call was successful or will
*                contain an error or warning code. If the status is U_USING_FALLBACK_WARNING, it means at least one of the
                 settings was not succesfully mapped between NLS and CLDR, so it will not be shown on the BCP47 tag.
* @return The needed buffer size, including the terminating \0 null character if the call was successful, should be ignored 
*         if status was not U_ZERO_ERROR.
*/
UPREFS_API size_t U_EXPORT2 uprefs_getBCP47Tag(char* uprefsBuffer, size_t bufferSize, UErrorCode* status);

/**
* Gets the Locale with script and region (if any) set in the user preferences.
*
* @param uprefsBuffer Pointer to a buffer in which this function retrieves the requested locale information.
*                     This pointer is not used if bufferSize is set to 0.
* @param bufferSize Size, in characters, of the data buffer indicated by uprefsBuffer. Alternatively, the application
*                   can set this parameter to 0. In this case, the function does not use the uprefsBuffer parameter
*                   and returns the required buffer size, including the terminating null character.
* @param status: Pointer to a UErrorCode. The resulting value will be U_ZERO_ERROR if the call was successful or will
*                contain an error or warning code. If the status is U_UNSUPPORTED_ERROR, it means a mapping was not possible
                 between NLS and CLDR
* @return The needed buffer size, including the terminating \0 null character if the call was successful, should be ignored
*         if status was not U_ZERO_ERROR.
*/
UPREFS_API size_t U_EXPORT2 uprefs_getLocaleBCP47Tag(char* uprefsBuffer, size_t bufferSize, UErrorCode* status);

/**
* Gets the calendar set in the user preferences.
*
* Note: This API does not return a valid BCP47 Tag, it only returns the user setting in a CLDR BCP47 U extensions format.
* Example: Instead of returning "en-US-u-ca-gregory", it will return only "gregory" 
*
* @param uprefsBuffer Pointer to a buffer in which this function retrieves the requested locale information.
*                     This pointer is not used if bufferSize is set to 0.
* @param bufferSize Size, in characters, of the data buffer indicated by uprefsBuffer. Alternatively, the application
*                   can set this parameter to 0. In this case, the function does not use the uprefsBuffer parameter
*                   and returns the required buffer size, including the terminating null character.
* @param status: Pointer to a UErrorCode. The resulting value will be U_ZERO_ERROR if the call was successful or will
*                contain an error or warning code. If the status is U_UNSUPPORTED_ERROR, it means a mapping was not possible
                 between NLS and CLDR
* @return The needed buffer size, including the terminating \0 null character if the call was successful, should be ignored
*         if status was not U_ZERO_ERROR.
*/
UPREFS_API size_t U_EXPORT2 uprefs_getCalendarSystem(char* uprefsBuffer, size_t bufferSize, UErrorCode* status);

/**
* Gets sorting system set in the user preferences (if any) 
* If there is no sorting system set, uprefsBuffer will be empty.
*
* Note: This API does not return a valid BCP47 Tag, it only returns the user setting in a CLDR BCP47 U extensions format.
* Example: Instead of returning "de-DE-u-co-phoneb", it will return only "phoneb" 
*
* @param uprefsBuffer Pointer to a buffer in which this function retrieves the requested locale information.
*                     This pointer is not used if bufferSize is set to 0.
* @param bufferSize Size, in characters, of the data buffer indicated by uprefsBuffer. Alternatively, the application
*                   can set this parameter to 0. In this case, the function does not use the uprefsBuffer parameter
*                   and returns the required buffer size, including the terminating null character.
* @param status: Pointer to a UErrorCode. The resulting value will be U_ZERO_ERROR if the call was successful or will
*                contain an error or warning code. If the status is U_UNSUPPORTED_ERROR, it means a mapping was not possible
                 between NLS and CLDR
* @return The needed buffer size, including the terminating \0 null character if the call was successful, should be ignored
*         if status was not U_ZERO_ERROR.
*/
UPREFS_API size_t U_EXPORT2 uprefs_getSortingSystem(char* uprefsBuffer, size_t bufferSize, UErrorCode* status);

/**
* Gets the currency set in the user preferences.
*
* Note: This API does not return a valid BCP47 Tag, it only returns the user setting in a CLDR BCP47 U extensions format.
* The format for a BCP47 tag is for everything besides the country in the locale to be lowercased, so although the currencies
* are normally in uppercase, this API will return them as lowercase.
* Example: Instead of returning "en-US-u-cu-usd", it will return only "usd" 
*
* @param uprefsBuffer Pointer to a buffer in which this function retrieves the requested locale information.
*                     This pointer is not used if bufferSize is set to 0.
* @param bufferSize Size, in characters, of the data buffer indicated by uprefsBuffer. Alternatively, the application
*                   can set this parameter to 0. In this case, the function does not use the uprefsBuffer parameter
*                   and returns the required buffer size, including the terminating null character.
* @param status: Pointer to a UErrorCode. The resulting value will be U_ZERO_ERROR if the call was successful or will
*                contain an error or warning code. If the status is U_UNSUPPORTED_ERROR, it means a mapping was not possible
                 between NLS and CLDR
* @return The needed buffer size, including the terminating \0 null character if the call was successful, should be ignored
*         if status was not U_ZERO_ERROR.
*/
UPREFS_API size_t U_EXPORT2 uprefs_getCurrencyCode(char* uprefsBuffer, size_t bufferSize, UErrorCode* status);

/**
* Gets the first day of the week set in the user preferences.
*
* Note: This API does not return a valid BCP47 Tag, it only returns the user setting in a CLDR BCP47 U extensions format.
* Example: Instead of returning "en-US-u-fw-mon", it will return only "mon" 
*
* @param uprefsBuffer Pointer to a buffer in which this function retrieves the requested locale information.
*                     This pointer is not used if bufferSize is set to 0.
* @param bufferSize Size, in characters, of the data buffer indicated by uprefsBuffer. Alternatively, the application
*                   can set this parameter to 0. In this case, the function does not use the uprefsBuffer parameter
*                   and returns the required buffer size, including the terminating null character.
* @param status: Pointer to a UErrorCode. The resulting value will be U_ZERO_ERROR if the call was successful or will
*                contain an error or warning code. If the status is U_UNSUPPORTED_ERROR, it means a mapping was not possible
                 between NLS and CLDR
* @return The needed buffer size, including the terminating \0 null character if the call was successful, should be ignored
*         if status was not U_ZERO_ERROR.
*/
UPREFS_API size_t U_EXPORT2 uprefs_getFirstDayOfWeek(char* uprefsBuffer, size_t bufferSize, UErrorCode* status);

/**
* Gets the hour cycle set in the user preferences.
*
* Note: This API does not return a valid BCP47 Tag, it only returns the user setting in a CLDR BCP47 U extensions format.
* Example: Instead of returning "en-US-u-hc-h12", it will return only "h12" 
*
* @param uprefsBuffer Pointer to a buffer in which this function retrieves the requested locale information.
*                     This pointer is not used if bufferSize is set to 0.
* @param bufferSize Size, in characters, of the data buffer indicated by uprefsBuffer. Alternatively, the application
*                   can set this parameter to 0. In this case, the function does not use the uprefsBuffer parameter
*                   and returns the required buffer size, including the terminating null character.
* @param status: Pointer to a UErrorCode. The resulting value will be U_ZERO_ERROR if the call was successful or will
*                contain an error or warning code. If the status is U_UNSUPPORTED_ERROR, it means a mapping was not possible
                 between NLS and CLDR
* @return The needed buffer size, including the terminating \0 null character if the call was successful, should be ignored
*         if status was not U_ZERO_ERROR.
*/
UPREFS_API size_t U_EXPORT2 uprefs_getHourCycle(char* uprefsBuffer, size_t bufferSize, UErrorCode* status);

/**
* Gets the measurement system set in the user preferences.
*
* Note: This API does not return a valid BCP47 Tag, it only returns the user setting in a CLDR BCP47 U extensions format.
* Example: Instead of returning "en-US-u-ms-ussystem", it will return only "ussystem" 
*
* @param uprefsBuffer Pointer to a buffer in which this function retrieves the requested locale information.
*                     This pointer is not used if bufferSize is set to 0.
* @param bufferSize Size, in characters, of the data buffer indicated by uprefsBuffer. Alternatively, the application
*                   can set this parameter to 0. In this case, the function does not use the uprefsBuffer parameter
*                   and returns the required buffer size, including the terminating null character.
* @param status: Pointer to a UErrorCode. The resulting value will be U_ZERO_ERROR if the call was successful or will
*                contain an error or warning code. If the status is U_UNSUPPORTED_ERROR, it means a mapping was not possible
                 between NLS and CLDR
* @return The needed buffer size, including the terminating \0 null character if the call was successful, should be ignored
*         if status was not U_ZERO_ERROR.
*/
UPREFS_API size_t U_EXPORT2 uprefs_getMeasureSystem(char* uprefsBuffer, size_t bufferSize, UErrorCode* status);
