// © Microsoft Corporation. All rights reserved.

#pragma once

#include <exception>
#include <string>
#include <icu.h>

struct icu_error final : std::exception
{
    icu_error() noexcept : m_code{}, m_source{} {}

    explicit icu_error(UErrorCode code) :
        m_code{ code }, m_source{},
        // MSVC std::exception extension; immediately copies the c_str and supports noexcept exception copies
        std::exception{ get_error_message(code) }
    {
    }

    // The caller is responsible for providing a source pointer that is guaranteed to live as long as the exception
    // (for example, a compile-time string literal).
    icu_error(UErrorCode code, const char* source) :
        m_code{ code }, m_source{ source },
        // MSVC std::exception extension; immediately copies the c_str and supports noexcept exception copies
        std::exception{ get_combined_error_message(code, source).c_str() }
    {
    }

    icu_error(const icu_error&) = default;
    icu_error(icu_error&&) noexcept = default;

    icu_error& operator=(const icu_error&) = default;
    icu_error& operator=(icu_error&&) noexcept = default;

    ~icu_error() noexcept = default;

    [[nodiscard]] constexpr UErrorCode code() const noexcept { return m_code; }

    [[nodiscard]] const char* name() const noexcept { return u_errorName(m_code); };

    [[nodiscard]] constexpr const char* source() const noexcept { return m_source; }

    // Similar to .what(), except it never includes a "{source()} failed: " prefix.
    [[nodiscard]] const char* message() const noexcept { return get_error_message(m_code); };

private:
    static constexpr const char* get_known_error_description(UErrorCode code) noexcept
    {
        // The string literals below come from the comments on each UErrorCode item in icu.h.
        // Warnings are included here, as a caller may want to treat warnings as errors.
        // U_ZERO_ERROR is intentionally excluded, as it is neither a warning nor an error.
        switch (code)
        {
        case U_USING_FALLBACK_WARNING:
            return "A resource bundle lookup returned a fallback result (not an error)";
        case U_USING_DEFAULT_WARNING:
            return "A resource bundle lookup returned a result from the root locale (not an error)";
        case U_SAFECLONE_ALLOCATED_WARNING:
            return "A SafeClone operation required allocating memory (informational only)";
        case U_STATE_OLD_WARNING:
            return "ICU has to use compatibility layer to construct the service. Expect performance/memory usage "
                   "degradation. Consider upgrading";
        case U_STRING_NOT_TERMINATED_WARNING:
            return "An output string could not be NUL-terminated because output length==destCapacity";
        case U_SORT_KEY_TOO_SHORT_WARNING:
            return "Number of levels requested in getBound is higher than the number of levels in the sort key";
        case U_AMBIGUOUS_ALIAS_WARNING:
            return "This converter alias can go to different converter implementations";
        case U_DIFFERENT_UCA_VERSION:
            return "ucol_open encountered a mismatch between UCA version and collator image version, so the collator "
                   "was constructed from rules. No impact to further function";
        case U_PLUGIN_CHANGED_LEVEL_WARNING:
            return "A plugin caused a level change. May not be an error, but later plugins may not load.";
        case U_ILLEGAL_ARGUMENT_ERROR:
            return "Illegal argument"; // Custom text; icu.h text is about start of error code range.
        case U_MISSING_RESOURCE_ERROR:
            return "The requested resource cannot be found";
        case U_INVALID_FORMAT_ERROR:
            return "Data format is not what is expected";
        case U_FILE_ACCESS_ERROR:
            return "The requested file cannot be found";
        case U_INTERNAL_PROGRAM_ERROR:
            return "Indicates a bug in the library code";
        case U_MESSAGE_PARSE_ERROR:
            return "Unable to parse a message (message format)";
        case U_MEMORY_ALLOCATION_ERROR:
            return "Memory allocation error";
        case U_INDEX_OUTOFBOUNDS_ERROR:
            return "Trying to access the index that is out of bounds";
        case U_PARSE_ERROR:
            return "Equivalent to Java ParseException";
        case U_INVALID_CHAR_FOUND:
            return "Character conversion: Unmappable input sequence. In other APIs: Invalid character.";
        case U_TRUNCATED_CHAR_FOUND:
            return "Character conversion: Incomplete input sequence.";
        case U_ILLEGAL_CHAR_FOUND:
            return "Character conversion: Illegal input sequence/combination of input units.";
        case U_INVALID_TABLE_FORMAT:
            return "Conversion table file found, but corrupted";
        case U_INVALID_TABLE_FILE:
            return "Conversion table file not found";
        case U_BUFFER_OVERFLOW_ERROR:
            return "A result would not fit in the supplied buffer";
        case U_UNSUPPORTED_ERROR:
            return "Requested operation not supported in current context";
        case U_RESOURCE_TYPE_MISMATCH:
            return "an operation is requested over a resource that does not support it";
        case U_ILLEGAL_ESCAPE_SEQUENCE:
            return "ISO-2022 illegal escape sequence";
        case U_UNSUPPORTED_ESCAPE_SEQUENCE:
            return "ISO-2022 unsupported escape sequence";
        case U_NO_SPACE_AVAILABLE:
            return "No space available for in-buffer expansion for Arabic shaping";
        case U_CE_NOT_FOUND_ERROR:
            return "Currently used only while setting variable top, but can be used generally";
        case U_PRIMARY_TOO_LONG_ERROR:
            return "User tried to set variable top to a primary that is longer than two bytes";
        case U_STATE_TOO_OLD_ERROR:
            return "ICU cannot construct a service from this state, as it is no longer supported";
        case U_TOO_MANY_ALIASES_ERROR:
            return "There are too many aliases in the path to the requested resource.\nIt is very possible that a "
                   "circular alias definition has occurred";
        case U_ENUM_OUT_OF_SYNC_ERROR:
            return "UEnumeration out of sync with underlying collection";
        case U_INVARIANT_CONVERSION_ERROR:
            return "Unable to convert a UChar* string to char* with the invariant converter.";
        case U_INVALID_STATE_ERROR:
            return "Requested operation can not be completed with ICU in its current state";
        case U_COLLATOR_VERSION_MISMATCH:
            return "Collator version is not compatible with the base version";
        case U_USELESS_COLLATOR_ERROR:
            return "Collator is options only and no base is specified";
        case U_NO_WRITE_PERMISSION:
            return "Attempt to modify read-only or constant data.";
        case U_BAD_VARIABLE_DEFINITION:
            return "Missing '$' or duplicate variable name";
        case U_MALFORMED_RULE:
            return "Elements of a rule are misplaced";
        case U_MALFORMED_SET:
            return "A UnicodeSet pattern is invalid";
        case U_MALFORMED_UNICODE_ESCAPE:
            return "A Unicode escape pattern is invalid";
        case U_MALFORMED_VARIABLE_DEFINITION:
            return "A variable definition is invalid";
        case U_MALFORMED_VARIABLE_REFERENCE:
            return "A variable reference is invalid";
        case U_MISPLACED_ANCHOR_START:
            return "A start anchor appears at an illegal position";
        case U_MISPLACED_CURSOR_OFFSET:
            return "A cursor offset occurs at an illegal position";
        case U_MISPLACED_QUANTIFIER:
            return "A quantifier appears after a segment close delimiter";
        case U_MISSING_OPERATOR:
            return "A rule contains no operator";
        case U_MULTIPLE_ANTE_CONTEXTS:
            return "More than one ante context";
        case U_MULTIPLE_CURSORS:
            return "More than one cursor";
        case U_MULTIPLE_POST_CONTEXTS:
            return "More than one post context";
        case U_TRAILING_BACKSLASH:
            return "A dangling backslash";
        case U_UNDEFINED_SEGMENT_REFERENCE:
            return "A segment reference does not correspond to a defined segment";
        case U_UNDEFINED_VARIABLE:
            return "A variable reference does not correspond to a defined variable";
        case U_UNQUOTED_SPECIAL:
            return "A special character was not quoted or escaped";
        case U_UNTERMINATED_QUOTE:
            return "A closing single quote is missing";
        case U_RULE_MASK_ERROR:
            return "A rule is hidden by an earlier more general rule";
        case U_MISPLACED_COMPOUND_FILTER:
            return "A compound filter is in an invalid location";
        case U_MULTIPLE_COMPOUND_FILTERS:
            return "More than one compound filter";
        case U_INVALID_RBT_SYNTAX:
            return "A \"::id\" rule was passed to the RuleBasedTransliterator parser";
        case U_MALFORMED_PRAGMA:
            return "A 'use' pragma is invalid";
        case U_UNCLOSED_SEGMENT:
            return "A closing ')' is missing";
        case U_VARIABLE_RANGE_EXHAUSTED:
            return "Too many stand-ins generated for the given variable range";
        case U_VARIABLE_RANGE_OVERLAP:
            return "The variable range overlaps characters used in rules";
        case U_ILLEGAL_CHARACTER:
            return "A special character is outside its allowed context";
        case U_INTERNAL_TRANSLITERATOR_ERROR:
            return "Internal transliterator system error";
        case U_INVALID_ID:
            return "A \"::id\" rule specifies an unknown transliterator";
        case U_INVALID_FUNCTION:
            return "A \" & fn()\" rule specifies an unknown transliterator";
        case U_UNEXPECTED_TOKEN:
            return "Syntax error in format pattern";
        case U_MULTIPLE_DECIMAL_SEPARATORS:
            return "More than one decimal separator in number pattern";
        case U_MULTIPLE_EXPONENTIAL_SYMBOLS:
            return "More than one exponent symbol in number pattern";
        case U_MALFORMED_EXPONENTIAL_PATTERN:
            return "Grouping symbol in exponent pattern";
        case U_MULTIPLE_PERCENT_SYMBOLS:
            return "More than one percent symbol in number pattern";
        case U_MULTIPLE_PERMILL_SYMBOLS:
            return "More than one permill symbol in number pattern";
        case U_MULTIPLE_PAD_SPECIFIERS:
            return "More than one pad symbol in number pattern";
        case U_PATTERN_SYNTAX_ERROR:
            return "Syntax error in format pattern";
        case U_ILLEGAL_PAD_POSITION:
            return "Pad symbol misplaced in number pattern";
        case U_UNMATCHED_BRACES:
            return "Braces do not match in message pattern";
        case U_ARGUMENT_TYPE_MISMATCH:
            return "Argument name and argument index mismatch in MessageFormat functions";
        case U_DUPLICATE_KEYWORD:
            return "Duplicate keyword in PluralFormat";
        case U_UNDEFINED_KEYWORD:
            return "Undefined Plural keyword";
        case U_DEFAULT_KEYWORD_MISSING:
            return "Missing DEFAULT rule in plural rules";
        case U_DECIMAL_NUMBER_SYNTAX_ERROR:
            return "Decimal number syntax error";
        case U_FORMAT_INEXACT_ERROR:
            return "Cannot format a number exactly and rounding mode is ROUND_UNNECESSARY";
#if (NTDDI_VERSION >= NTDDI_WIN10_VB)
        case U_NUMBER_ARG_OUTOFBOUNDS_ERROR:
            return "The argument to a NumberFormatter helper method was out of bounds; the bounds are usually 0 to "
                   "999.";
        case U_NUMBER_SKELETON_SYNTAX_ERROR:
            return "The number skeleton passed to C++ NumberFormatter or C UNumberFormatter was invalid or contained a "
                   "syntax error.";
#endif // (NTDDI_VERSION >= NTDDI_WIN10_VB)
        case U_BRK_INTERNAL_ERROR:
            return "An internal error (bug) was detected.";
        case U_BRK_HEX_DIGITS_EXPECTED:
            return "Hex digits expected as part of a escaped char in a rule.";
        case U_BRK_SEMICOLON_EXPECTED:
            return "Missing ';' at the end of a RBBI rule.";
        case U_BRK_RULE_SYNTAX:
            return "Syntax error in RBBI rule.";
        case U_BRK_UNCLOSED_SET:
            return "UnicodeSet writing an RBBI rule missing a closing ']'.";
        case U_BRK_ASSIGN_ERROR:
            return "Syntax error in RBBI rule assignment statement.";
        case U_BRK_VARIABLE_REDFINITION:
            return "RBBI rule $Variable redefined.";
        case U_BRK_MISMATCHED_PAREN:
            return "Mis-matched parentheses in an RBBI rule.";
        case U_BRK_NEW_LINE_IN_QUOTED_STRING:
            return "Missing closing quote in an RBBI rule.";
        case U_BRK_UNDEFINED_VARIABLE:
            return "Use of an undefined $Variable in an RBBI rule.";
        case U_BRK_INIT_ERROR:
            return "Initialization failure.  Probable missing ICU Data.";
        case U_BRK_RULE_EMPTY_SET:
            return "Rule contains an empty Unicode Set.";
        case U_BRK_UNRECOGNIZED_OPTION:
            return "!!option in RBBI rules not recognized.";
        case U_BRK_MALFORMED_RULE_TAG:
            return "The {nnn} tag on a rule is malformed";
        case U_REGEX_INTERNAL_ERROR:
            return "An internal error (bug) was detected.";
        case U_REGEX_RULE_SYNTAX:
            return "Syntax error in regexp pattern.";
        case U_REGEX_INVALID_STATE:
            return "RegexMatcher in invalid state for requested operation";
        case U_REGEX_BAD_ESCAPE_SEQUENCE:
            return "Unrecognized backslash escape sequence in pattern";
        case U_REGEX_PROPERTY_SYNTAX:
            return "Incorrect Unicode property";
        case U_REGEX_UNIMPLEMENTED:
            return "Use of regexp feature that is not yet implemented.";
        case U_REGEX_MISMATCHED_PAREN:
            return "Incorrectly nested parentheses in regexp pattern.";
        case U_REGEX_NUMBER_TOO_BIG:
            return "Decimal number is too large.";
        case U_REGEX_BAD_INTERVAL:
            return "Error in {min,max} interval";
        case U_REGEX_MAX_LT_MIN:
            return "In {min,max}, max is less than min.";
        case U_REGEX_INVALID_BACK_REF:
            return "Back-reference to a non-existent capture group.";
        case U_REGEX_INVALID_FLAG:
            return "Invalid value for match mode flags.";
        case U_REGEX_LOOK_BEHIND_LIMIT:
            return "Look-Behind pattern matches must have a bounded maximum length.";
        case U_REGEX_SET_CONTAINS_STRING:
            return "Regexps cannot have UnicodeSets containing strings.";
        case U_REGEX_MISSING_CLOSE_BRACKET:
            return "Missing closing bracket on a bracket expression.";
        case U_REGEX_INVALID_RANGE:
            return "In a character range [x-y], x is greater than y.";
        case U_REGEX_STACK_OVERFLOW:
            return "Regular expression backtrack stack overflow.";
        case U_REGEX_TIME_OUT:
            return "Maximum allowed match time exceeded";
        case U_REGEX_STOPPED_BY_CALLER:
            return "Matching operation aborted by user callback fn.";
        case U_REGEX_PATTERN_TOO_BIG:
            return "Pattern exceeds limits on size or complexity.";
        case U_REGEX_INVALID_CAPTURE_GROUP_NAME:
            return "Invalid capture group name.";
        case U_PLUGIN_TOO_HIGH:
            return "The plugin's level is too high to be loaded right now.";
        case U_PLUGIN_DIDNT_SET_LEVEL:
            return "The plugin didn't call uplug_setPlugLevel in response to a QUERY";
        default:
            return nullptr;
        }
    }

    static const char* get_error_message(UErrorCode code) noexcept
    {
        const char* description{ get_known_error_description(code) };

        // Prefer a human-readable error description for .what(), like other exception types do (system_error).
        if (description != nullptr)
        {
            return description;
        }

        // If we don't have a human-readable error description, fall back to the text symbol for the error code (it's
        // far better than nothing).
        return u_errorName(code);
    }

    static std::string get_combined_error_message(UErrorCode code, const char* source)
    {
        std::string result{ source };
        result.append(" failed: ");
        result.append(get_error_message(code));
        return result;
    }

    UErrorCode m_code;
    const char* m_source;
};
