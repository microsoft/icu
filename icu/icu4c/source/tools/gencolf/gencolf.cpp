// © Microsoft Corporation. All rights reserved.

#include <iostream>
#include <array>
#include <map>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>
#include <fcntl.h>
#include <io.h>
#include "icu_cpp.h"
#include "icu_error.h"
#include "utf8.h"

#include "unicode/udata.h"

#include "uoptions.h"
#include "unewdata.h"
#include "ucmndata.h"
#include "cmemory.h"
#include "toolutil.h"

static char *progName;
static UOption options[] = {
    UOPTION_HELP_H,                                         /* 0 */
    UOPTION_HELP_QUESTION_MARK,                             /* 1 */
    UOPTION_VERBOSE,                                        /* 2 */
    {"out", NULL, NULL, NULL, 'o', UOPT_REQUIRES_ARG, 0},   /* 3 */
    UOPTION_ICUDATADIR,                                     /* 4 */
    UOPTION_DESTDIR,                                        /* 5 */
    UOPTION_COPYRIGHT,                                      /* 6 */
    UOPTION_QUIET,                                          /* 7 */
};

void usageAndDie(int retCode) {
    printf("Usage: %s [-v] [-options] -r coll-data-dir -o output-file\n", progName);
    printf("\tRead in collation data and write out collation folding binary data\n"
           "options:\n"
           "\t-h or -? or --help  this usage text\n"
           "\t-V or --version     show a version message\n"
           "\t-c or --copyright   include a copyright notice\n"
           "\t-v or --verbose     turn on verbose output\n"
           "\t-q or --quiet       do not display warnings and progress\n"
           "\t-i or --icudatadir  directory for locating any needed intermediate data files,\n"
           "\t                    followed by path, defaults to %s\n"
           "\t-d or --destdir     destination directory, followed by the path\n",
           u_getDataDirectory());
    exit(retCode);
}

//
//  Set up the ICU data header, defined in ucmndata.h
//
DataHeader dh = {
    {sizeof(DataHeader), // Struct MappedData
        0xda,
        0x27},

    {                               // struct UDataInfo
        sizeof(UDataInfo),          //     size
        0,                          //     reserved
        U_IS_BIG_ENDIAN,
        U_CHARSET_FAMILY,
        U_SIZEOF_UCHAR,
        0,                          //     reserved

        {0x43, 0x6c, 0x66, 0x20},   //     dataFormat="Clf "
        {0xff, 0, 0, 0},            //     formatVersion. Filled in later with values
                                    //     from the  builder. The values declared
                                    //     here should never appear in any real data.
        {15, 1, 0, 0}               //     dataVersion (Unicode version)
    }};

namespace
{
    void set_mode_or_throw(int fd, int mode)
    {
        int result{ _setmode(fd, mode) };

        if (result == -1)
        {
            throw std::system_error{ errno, std::generic_category() };
        }
    }

    constexpr char16_t get_hex_digit(uint8_t value) noexcept
    {
        if (value > 0xF)
        {
            assert(value <= 0xF); // assert(false) with a nicer error message
            value = static_cast<uint8_t>(value & 0x0F);
        }

        if (value < 0xA)
        {
            return u'0' + static_cast<char16_t>(value);
        }
        else
        {
            return u'A' + static_cast<char16_t>(value - 0xA);
        }
    }

    void add_hex_8(uint8_t value, std::u16string& result)
    {
        result.push_back(get_hex_digit(static_cast<uint8_t>((value & 0xF0) >> 4)));
        result.push_back(get_hex_digit(static_cast<uint8_t>(value & 0xF)));
    }

    void add_hex_16(uint16_t value, std::u16string& result)
    {
        add_hex_8(static_cast<uint8_t>((value & 0xFF00) >> 8), result);
        add_hex_8(static_cast<uint8_t>(value & 0xFF), result);
    }

    std::u16string get_collation_key_sequence(
        std::span<uint16_t> primary, std::span<uint8_t> secondary, std::span<uint8_t> tertiary)
    {
        std::u16string result{};
        result.reserve(2 + (primary.size() + secondary.size() + tertiary.size()) * 2);

        for (uint16_t item : primary)
        {
            add_hex_16(item, result);
        }

        if (!secondary.empty() || !tertiary.empty())
        {
            result.push_back(u' ');

            for (uint8_t item : secondary)
            {
                add_hex_8(item, result);
            }

            if (!tertiary.empty())
            {
                result.push_back(u' ');

                for (uint8_t item : tertiary)
                {
                    add_hex_8(item, result);
                }
            }
        }

        return result;
    }

    std::u16string get_collation_key_sequence(
        std::u16string_view text, const UCollator* collator, UCollationStrength strength)
    {
        unique_UCollationElements elements{ ucol_open_elements_cpp(collator, text) };

        std::vector<uint16_t> combinedPrimary{};
        std::vector<uint8_t> combinedSecondary{};
        std::vector<uint8_t> combinedTertiary{};

        int32_t value{};

        while (ucol_try_next_cpp(elements.get(), value))
        {
            uint16_t primary{ ucol_primary_order_cpp(value) };

            if (primary != 0)
            {
                combinedPrimary.push_back(primary);
            }

            if (strength < UCollationStrength::UCOL_SECONDARY)
            {
                continue;
            }

            uint8_t secondary{ ucol_secondary_order_cpp(value) };

            if (secondary != 0)
            {
                combinedSecondary.push_back(secondary);
            }

            if (strength < UCollationStrength::UCOL_TERTIARY)
            {
                continue;
            }

            uint8_t tertiary{ ucol_tertiary_order_cpp(value) };

            if (tertiary != 0)
            {
                combinedTertiary.push_back(tertiary);
            }
        }

        return get_collation_key_sequence(combinedPrimary, combinedSecondary, combinedTertiary);
    }

    void add_collation_item(std::u16string_view text, const UCollator* collator, UCollationStrength strength,
        std::unordered_multimap<std::u16string, std::u16string>& textByCollationKeySequence)
    {
        std::u16string collationKeySequence{ get_collation_key_sequence(text, collator, strength) };
        textByCollationKeySequence.emplace(collationKeySequence, text);
    }

    void add_collation_item(char32_t value, const UCollator* collator, UCollationStrength strength,
        std::unordered_multimap<std::u16string, std::u16string>& textByCollationKeySequence)
    {
        icu_utf32_to_utf16_converter valueUtf16{ value };

        if (!valueUtf16.has_surrogate_pair())
        {
            char16_t nonSurrogateValue{ valueUtf16.non_surrogate_value() };
            std::u16string_view buffer{ &nonSurrogateValue, 1 };
            add_collation_item(buffer, collator, strength, textByCollationKeySequence);
        }
        else
        {
            std::array<char16_t, 2> buffer{ valueUtf16.lead_surrogate(), valueUtf16.trail_surrogate() };
            std::u16string_view bufferView{ buffer.data(), buffer.size() };
            add_collation_item(bufferView, collator, strength, textByCollationKeySequence);
        }
    }

    void add_code_point_collation_items(const UCollator* collator, UCollationStrength strength,
        std::unordered_multimap<std::u16string, std::u16string>& textByCollationKeySequence)
    {
        for (char32_t item = u'\0'; item <= U'\U0010FFFF'; ++item)
        {
            if (item >= static_cast<char32_t>(0xD800) && item <= static_cast<char32_t>(0xDFFF))
            {
                continue;
            }

            add_collation_item(item, collator, strength, textByCollationKeySequence);
        }
    }

    void add_contraction_and_prefix_collation_items(const UCollator* collator, UCollationStrength strength,
        std::unordered_multimap<std::u16string, std::u16string>& textByCollationKeySequence)
    {
        unique_USet contractions{ uset_open_empty_cpp() };
        constexpr bool addPrefixes{ true };
        ucol_get_contractions_and_expansions_cpp(collator, contractions.get(), nullptr, addPrefixes);

        size_t itemCount{ uset_get_item_count_cpp(contractions.get()) };

        for (size_t contractionIndex = 0; contractionIndex < itemCount; ++contractionIndex)
        {
            USet_item_result contraction{ uset_get_item_cpp(contractions.get(), contractionIndex) };

            if (contraction.range.has_value())
            {
                std::terminate();
            }

            std::u16string contractionString{};
            contractionString.resize(contraction.string_size.value() + 1);
            contractionString.resize(uset_get_item_string_cpp(contractions.get(), contractionIndex, contractionString));
            fflush(stdout);
            add_collation_item(contractionString, collator, strength, textByCollationKeySequence);
        }
    }

    constexpr wchar_t to_hex_digit(uint32_t value) noexcept
    {
        if (value > 0xF)
        {
            std::terminate();
        }

        if (value < 0xA)
        {
            return L'0' + static_cast<wchar_t>(value);
        }
        else
        {
            return L'A' + static_cast<wchar_t>(value - 0xA);
        }
    }

    std::wstring to_utf32_debug_string(std::u16string_view text)
    {
        std::u32string textUtf32{ u_str_to_utf32_cpp(text) };
        std::wstring result{};
        result.reserve(textUtf32.size() * 7); // rough lower bound; " U+XXXX" per character

        for (size_t index = 0; index < textUtf32.size(); ++index)
        {
            if (index > 0)
            {
                result.push_back(L' ');
            }

            result.push_back(L'U');
            result.push_back(L'+');
            char32_t item{ textUtf32[index] };

            if (item <= u'\uFFFF')
            {
                result.push_back(to_hex_digit((item & 0xF000) >> 12));
                result.push_back(to_hex_digit((item & 0xF00) >> 8));
                result.push_back(to_hex_digit((item & 0xF0) >> 4));
                result.push_back(to_hex_digit(item & 0xF));
            }
            else if (item <= U'\U000FFFFF')
            {
                result.push_back(to_hex_digit((item & 0xF0000) >> 16));
                result.push_back(to_hex_digit((item & 0xF000) >> 12));
                result.push_back(to_hex_digit((item & 0xF00) >> 8));
                result.push_back(to_hex_digit((item & 0xF0) >> 4));
                result.push_back(to_hex_digit(item & 0xF));
            }
            else if (item <= U'\U0010FFFF')
            {
                result.push_back(to_hex_digit((item & 0xF00000) >> 20));
                result.push_back(to_hex_digit((item & 0xF0000) >> 16));
                result.push_back(to_hex_digit((item & 0xF000) >> 12));
                result.push_back(to_hex_digit((item & 0xF00) >> 8));
                result.push_back(to_hex_digit((item & 0xF0) >> 4));
                result.push_back(to_hex_digit(item & 0xF));
            }
            else
            {
                std::terminate();
            }
        }

        return result;
    }

    bool is_lowercase(std::u16string_view value)
    {
        icu_utf16_to_utf32_view view{ value };

        for (icu_utf16_to_utf32_item item : view)
        {
            if (!item.is_code_point())
            {
                // An unpaired surrogate is not lowercase.
                return false;
            }

            if (u_charType(item.code_point()) != UCharCategory::U_LOWERCASE_LETTER)
            {
                return false;
            }
        }

        // A string is lowercase unless it has at least one non-lowercase character (or invalid code unit sequence).
        // (i.e., an empty string is considered lowercase)
        return true;
    }

    size_t get_canonical_item_index(std::vector<std::u16string_view>& equivalenceClass)
    {
        if (equivalenceClass.empty())
        {
            assert(false);
            return std::u16string_view::npos;
        }

        // Find the item with the lowest code unit (or sum of code unit values), except always preferring entirely
        // lowercase strings.
        size_t result{ 0 };
        bool resultIsLowercase{ is_lowercase(equivalenceClass[0]) };

        for (size_t index = 1; index < equivalenceClass.size(); ++index)
        {
            std::u16string_view item{ equivalenceClass[index] };

            if (!resultIsLowercase && is_lowercase(item))
            {
                result = index;
                resultIsLowercase = true;
            }
        }

        return result;
    }

    void add_collation_folding_map_items(
        std::vector<std::u16string_view>& equivalenceClass, std::unordered_map<std::u16string, std::u16string>& result)
    {
        if (equivalenceClass.empty())
        {
            assert(false);
            return;
        }

        if (equivalenceClass.size() == 1)
        {
            return;
        }

        size_t canonicalItemIndex{ get_canonical_item_index(equivalenceClass) };
        std::u16string_view canonicalItem{ equivalenceClass[canonicalItemIndex] };

        for (size_t index = 0; index < equivalenceClass.size(); ++index)
        {
            if (index == canonicalItemIndex)
            {
                continue;
            }

            result.emplace(equivalenceClass[index], canonicalItem);
        }
    }

    std::unordered_map<std::u16string, std::u16string> create_collation_folding_map(
        const std::unordered_multimap<std::u16string, std::u16string>& textByCollationKeySequence)
    {
        std::unordered_map<std::u16string, std::u16string> result{};
        std::vector<std::u16string_view> equivalenceClass{};

        // Iterating all unique keys: https://stackoverflow.com/a/59689244
        for (auto iterator = textByCollationKeySequence.cbegin(); iterator != textByCollationKeySequence.cend();)
        {
            equivalenceClass.clear();
            const std::u16string& key{ iterator->first };
            equivalenceClass.push_back(std::u16string_view{ iterator->second });

            while (++iterator != textByCollationKeySequence.cend() &&
                   textByCollationKeySequence.key_eq()(iterator->first, key))
            {
                equivalenceClass.push_back(std::u16string_view{ iterator->second });
            }

            add_collation_folding_map_items(equivalenceClass, result);
        }

        std::ignore = textByCollationKeySequence;

        return result;
    }

    std::unordered_map<std::u16string, std::u16string> create_collation_folding_map(
        const UCollator* collator, UCollationStrength strength)
    {
        std::unordered_multimap<std::u16string, std::u16string> textByCollationKeySequence{};
        add_code_point_collation_items(collator, strength, textByCollationKeySequence);
        add_contraction_and_prefix_collation_items(collator, strength, textByCollationKeySequence);
        return create_collation_folding_map(textByCollationKeySequence);
    }

    std::map<std::u16string, std::u16string> to_map(const std::unordered_map<std::u16string, std::u16string>& value)
    {
        std::map<std::u16string, std::u16string> result{};

        for (const auto& pair : value)
        {
            result.emplace(pair.first, pair.second);
        }

        return result;
    }

    void print_collation_folding_map(const std::map<std::u16string, std::u16string>& value, FILE* stream)
    {
        for (const auto& pair : value)
        {
            std::u16string from{ pair.first };
            std::u16string to{ pair.second };
            std::u16string fromDisplay{ from };

            // The console apparently treats this character as EOF (or an error that triggers EOF-like
            // behavior).
            if (fromDisplay.size() == 1 && fromDisplay[0] == u'\uFFFF')
            {
                fromDisplay = u"";
            }

            fwprintf(stream, L"\t\t%s{\"%s\"}\n", reinterpret_cast<const wchar_t*>(fromDisplay.c_str()), reinterpret_cast<const wchar_t*>(to.c_str()));
            fflush(stdout);
        }
    }

    constexpr const wchar_t* strength_to_string(UCollationStrength value) noexcept
    {
        switch (value)
        {
        case UCollationStrength::UCOL_PRIMARY:
            return L"primary";
        case UCollationStrength::UCOL_SECONDARY:
            return L"secondary";
        case UCollationStrength::UCOL_TERTIARY:
            return L"tertiary";
        case UCollationStrength::UCOL_QUATERNARY:
            return L"quaternary";
        case UCollationStrength::UCOL_IDENTICAL:
            return L"identical";
        default:
            return L"unknown";
        }
    }

    void run_locale(const char* locale, UCollationStrength strength, FILE* stream)
    {
        fwprintf(stream, L"%s locale\n", locale != nullptr ? from_utf8(locale).c_str() : L"root");

        unique_UCollator collator{ ucol_open_cpp(locale, icu_resource_search_mode::exact_match) };
        ucol_setStrength(collator.get(), strength);

        {
            icu_version version{ ucol_get_version_cpp(collator.get()) };
            fwprintf(stream, L"Collator Version %u.%u.%u.%u\n", version.major, version.minor,
                     version.milli, version.micro);

            icu_version ucaVersion{ ucol_get_uca_version_cpp(collator.get()) };
            fwprintf(stream, L"Collator UCA Version %u.%u.%u.%u\n", ucaVersion.major, ucaVersion.minor,
                     ucaVersion.milli,
                ucaVersion.micro);

            UCollationStrength actualStrength{ ucol_getStrength(collator.get()) };
            fwprintf(stream, L"%s strength\n", strength_to_string(actualStrength));
        }

        fflush(stdout);

        fwprintf(stream, L"Collation folding map:\n");
        fflush(stdout);

        std::unordered_map<std::u16string, std::u16string> collationFoldingMap{ create_collation_folding_map(
            collator.get(), strength) };
        print_collation_folding_map(to_map(collationFoldingMap), stream);
    }

    void run(FILE* stream)
    {
        set_mode_or_throw(_fileno(stdout), _O_U8TEXT);
        set_mode_or_throw(_fileno(stderr), _O_U8TEXT);

        icu_version icuVersion{ u_get_version_cpp() };
        fwprintf(stream, L"// © 2023 and later: Unicode, Inc. and others.\n");
        fwprintf(stream, L"// License & terms of use: http://www.unicode.org/copyright.html\n");
        fwprintf(stream, L"// Generated using source/tools/gencolf\n");
        fflush(stdout);

        // icu_version cldrVersion{ ulocdata_get_cldr_version_cpp() };
        // fwprintf(
        //     L"CLDR Version %u.%u.%u.%u\n", cldrVersion.major, cldrVersion.minor, cldrVersion.milli, cldrVersion.micro);
        // fflush(stdout);

        run_locale("root-u-co-search", UCollationStrength::UCOL_PRIMARY, stream);

        /*
        size_t bytesWritten;
        UNewDataMemory *pData;
        pData = udata_create(outDir, NULL, outFileName, &(dh.info), copyright, &status);
        if (U_FAILURE(status)) {
            fprintf(stderr, "gencfu: Could not open output file \"%s\", \"%s\"\n", outFileName,
                    u_errorName(status));
            exit(status);
        }

        //  Write the data itself.
        udata_writeBlock(pData, outData, outDataSize);
        // finish up
        bytesWritten = udata_finish(pData, &status);
        if (U_FAILURE(status)) {
            fprintf(stderr, "gencfu: Error %d writing the output file\n", status);
            exit(status);
        }

        if (bytesWritten != outDataSize) {
            fprintf(stderr, "gencfu: Error writing to output file \"%s\"\n", outFileName);
            exit(-1);
        }
        */
    }
}

int main(/*int argc, char **argv*/) {
    UErrorCode status = U_ZERO_ERROR;
    // const char *collDataDir = nullptr;
    // const char *outFileName = nullptr;
    // const char *outDir = nullptr;
    // const char *copyright = nullptr;

    // //
    // // Pick up and check the command line arguments,
    // // using the standard ICU tool utils option handling.
    // //
    // U_MAIN_INIT_ARGS(argc, argv);
    // progName = argv[0];
    // argc = u_parseArgs(argc, argv, UPRV_LENGTHOF(options), options);
    // if (argc < 0) {
    //     // Unrecognized option
    //     fprintf(stderr, "error in command line argument \"%s\"\n", argv[-argc]);
    // }
    // if (options[0].doesOccur || options[1].doesOccur) {
    //     //  -? or -h for help.
    //     usageAndDie(0);
    // }
    // if (!options[3].doesOccur) {
    //     fprintf(stderr, "output file must all be specified.\n");
    //     usageAndDie(U_ILLEGAL_ARGUMENT_ERROR);
    // }
    // outFileName = options[3].value;

    // if (options[4].doesOccur) {
    //     u_setDataDirectory(options[4].value);
    // }
    // /* Combine the directory with the file name */
    // if (options[5].doesOccur) {
    //     outDir = options[5].value;
    // }
    // if (options[6].doesOccur) {
    //     copyright = U_COPYRIGHT_STRING;
    // }

    // //
    // //  Create the output file
    // //
    // size_t bytesWritten;
    // UNewDataMemory *pData;
    // pData = udata_create(outDir, NULL, outFileName, &(dh.info), copyright, &status);
    // if (U_FAILURE(status)) {
    //     fprintf(stderr, "gencolf: Could not open output file \"%s\", \"%s\"\n", outFileName,
    //             u_errorName(status));
    //     exit(status);
    // }

    // char msg[1024];

    // /* write message with just the name */
    // snprintf(msg, sizeof(msg), "gencolf writes dummy %s, see uconfig.h", outFileName);
    // fprintf(stderr, "%s\n", msg);

    // //  Write the data itself.
    // udata_writeBlock(pData, msg, strlen(msg));
    // // finish up
    // bytesWritten = udata_finish(pData, &status);
    // if (U_FAILURE(status)) {
    //     fprintf(stderr, "gencolf: Error %d writing the output file\n", status);
    //     exit(status);
    // }

    // fwprintf(stderr, L"gencolf: written output file");
    // /*
    // if (bytesWritten != outDataSize) {
    //     fprintf(stderr, "gencfu: Error writing to output file \"%s\"\n", outFileName);
    //     exit(-1);
    // }
    // */

    const char* colfDir = "colf";
    uprv_mkdir(colfDir, &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "Error creating colf directory: %s\n", colfDir);
        exit(-1);
    }

    FILE *stream;
    std::string filename = colfDir + std::string("/root.txt");
    stream = fopen(filename.c_str(), "w");
    if (stream == nullptr) {
        fprintf(stderr, "Cannot open file \"%s\"\n\n", filename.c_str());
        exit(-1);
    }

    try
    {
        run(stream);
        fclose(stream);
    }
    catch (const icu_error& exception)
    {
        fwprintf(stderr, L"%s\n", from_utf8(exception.what()).c_str());
        return exception.code();
    }
    catch (const std::exception& exception)
    {
        fwprintf(stderr, L"%s\n", from_utf8(exception.what()).c_str());
        return 1;
    }
    catch (...)
    {
        fwprintf(stderr, L"Unexpected exception\n");
        return 1;
    }
}
