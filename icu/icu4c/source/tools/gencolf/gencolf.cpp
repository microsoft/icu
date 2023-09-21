// © Microsoft Corporation. All rights reserved.

#include <iostream>
#include <array>
#include <functional>
#include <map>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include <filesystem>
#include <algorithm>
#include <fcntl.h>
#include <io.h>
#include "icu_cpp.h"
#include "icu_error.h"
#include "utf8.h"

#include "uoptions.h"
#include "unewdata.h"
#include "ucmndata.h"
#include "cmemory.h"
#include "toolutil.h"

static char *progName;
static UOption options[] = {
    UOPTION_HELP_H,              /* 0 */
    UOPTION_HELP_QUESTION_MARK,  /* 1 */
    UOPTION_SOURCEDIR,           /* 2 */
    UOPTION_DESTDIR,             /* 3 */
};

void usageAndDie(int retCode) {
    printf("Usage: %s [-v] [-options] -r coll-data-dir -o output-file\n", progName);
    printf("\tCalls ICU Collation APIs to write out collation folding data under icu4c/source/data/colf/*.txt.\n"
           "options:\n"
           "\t-h or -? or --help  this usage text\n"
           "\t-s or --sourcedir   source directory, followed by the path\n"
           "\t-d or --destdir     destination directory, followed by the path\n");
    exit(retCode);
}

// Collation folding data only includes primary, secondary, and tertiary strengths.
static std::vector<UColAttributeValue> colfStrengths = {
    UCollationStrength::UCOL_PRIMARY,
    UCollationStrength::UCOL_SECONDARY,
    UCollationStrength::UCOL_TERTIARY
};

namespace
{
    struct collation_key_sequence
    {
        std::vector<uint16_t> primary;
        std::vector<uint8_t> secondary;
        std::vector<uint8_t> tertiary;

        constexpr bool operator==(const collation_key_sequence& other) const noexcept
        {
            if (primary.size() != other.primary.size())
            {
                return false;
            }

            if (secondary.size() != other.secondary.size())
            {
                return false;
            }
            
            if (tertiary.size() != other.tertiary.size())
            {
                return false;
            }

            for (size_t index = 0; index < primary.size(); ++index)
            {
                if (primary[index] != other.primary[index])
                {
                    return false;
                }
            }

            for (size_t index = 0; index < secondary.size(); ++index)
            {
                if (secondary[index] != other.secondary[index])
                {
                    return false;
                }
            }

            for (size_t index = 0; index < tertiary.size(); ++index)
            {
                if (tertiary[index] != other.tertiary[index])
                {
                    return false;
                }
            }

            return true;
        }
    };
}

namespace std
{
    template <>
    struct hash<collation_key_sequence>
    {
        constexpr size_t operator()(const collation_key_sequence& value) const noexcept
        {
            // See MSVC's type_traits (_FNV_offset_basis, _FNV_prime and the related _Fnv1a_append_bytes).
#if defined(_WIN64)
            constexpr size_t seed = 14695981039346656037ULL;
            constexpr size_t prime= 1099511628211ULL;
#else
            constexpr size_t seed = 2166136261U;
            constexpr size_t prime = 16777619U;
#endif

            size_t hash{ seed };

            for (uint16_t item : value.primary)
            {
                hash ^= item;
                hash *= prime;
            }

            for (uint8_t item : value.secondary)
            {
                hash ^= item;
                hash *= prime;
            }

            for (uint8_t item : value.tertiary)
            {
                hash ^= item;
                hash *= prime;
            }

            return hash;
        }
    };
}


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

    collation_key_sequence get_collation_key_sequence(
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

        return collation_key_sequence{combinedPrimary, combinedSecondary, combinedTertiary};
    }

    void add_collation_item(std::u16string_view text, const UCollator* collator, UCollationStrength strength,
        std::unordered_multimap<collation_key_sequence, std::u16string> &textByCollationKeySequence)
    {
        collation_key_sequence collationKeySequence{ get_collation_key_sequence(text, collator, strength) };
        textByCollationKeySequence.emplace(collationKeySequence, text);
    }

    void add_collation_item(char32_t value, const UCollator* collator, UCollationStrength strength,
        std::unordered_multimap<collation_key_sequence, std::u16string> &textByCollationKeySequence)
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
        std::unordered_multimap<collation_key_sequence, std::u16string> &textByCollationKeySequence)
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
        std::unordered_multimap<collation_key_sequence, std::u16string> &textByCollationKeySequence)
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
        const std::unordered_multimap<collation_key_sequence, std::u16string>& textByCollationKeySequence)
    {
        std::unordered_map<std::u16string, std::u16string> result{};
        std::vector<std::u16string_view> equivalenceClass{};

        // Iterating all unique keys: https://stackoverflow.com/a/59689244
        for (auto iterator = textByCollationKeySequence.cbegin(); iterator != textByCollationKeySequence.cend();)
        {
            equivalenceClass.clear();
            const collation_key_sequence& key{ iterator->first };
            equivalenceClass.push_back(std::u16string_view{ iterator->second });

            while (++iterator != textByCollationKeySequence.cend() &&
                   textByCollationKeySequence.key_eq()(iterator->first, key))
            {
                equivalenceClass.push_back(std::u16string_view{ iterator->second });
            }

            add_collation_folding_map_items(equivalenceClass, result);
        }

        return result;
    }

    std::unordered_map<std::u16string, std::u16string> create_collation_folding_map(
        const UCollator* collator, UCollationStrength strength,
        std::unordered_map<UCollationStrength, std::unordered_map<std::u16string, std::u16string>>& rootCollationFoldingMap)
    {
        std::unordered_multimap<collation_key_sequence, std::u16string> textByCollationKeySequence{};
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

    void print_collation_folding_map(const std::map<std::u16string, std::u16string>& value, UCollationStrength strength, FILE* stream)
    {
        fwprintf(stream, L"\t%s{\n", strength_to_string(strength));
        fflush(stream);
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
            fflush(stream);
        }
        fwprintf(stream, L"\t}\n");
        fflush(stream);
    }

    bool supports_search_collation(const char* locale)
    {
        unique_UEnumeration keywords = ucol_getKeywordValuesForLocale_cpp("collation", locale, false);
        int32_t count = uenum_count_cpp(keywords.get());
        bool searchCollationFound = false;
        for (int32_t i = 0; i < count; i++)
        {
            int32_t resultLength = 0;
            const char* collationKeyword = uenum_next_cpp(keywords.get(), &resultLength);

            if (strcmp(collationKeyword, "search") == 0)
            {
                return true;
            }
        }
        return false;
    }

    void run_locale(std::string locale, const char* outDir, 
        std::unordered_map<UCollationStrength, std::unordered_map<std::u16string, std::u16string>>& rootCollationFoldingMap)
    {
        std::string searchLocale(locale);
        searchLocale.append("-u-co-search");
        icu_resource_search_result searchResult{};
        unique_UCollator collator{ ucol_open_cpp(searchLocale.c_str(), searchResult) };
        if (searchResult != icu_resource_search_result::exact_match)
        {
            // Specific locale does not support 'search' collation type, and a fallback locale would be used. Skip.
            printf("SKIPPING. Locale uses fallback data: %s.\n", locale.c_str());
            return;
        }
        printf("Generating collation folding data for locale: %s.\n", locale.c_str());

        UErrorCode status = U_ZERO_ERROR;
        uprv_mkdir(outDir, &status);
        if (U_FAILURE(status))
        {
            fprintf(stderr, "Error creating colf directory: %s\n\n", outDir);
            exit(-1);
        }

        FILE* stream;
        std::string filename(outDir);
        filename.append("/").append(locale).append(".txt");
        stream = fopen(filename.c_str(), "w+,ccs=UTF-8");
        if (stream == nullptr)
        {
            fprintf(stderr, "Cannot open file \"%s\"\n\n", filename.c_str());
            exit(-1);
        }

        // TODO: Include copyright
        fwprintf(stream, L"// Generated using gencolf.exe located under icu4c/source/tools/gencolf.\n");
        fflush(stream);
        
        // ICU locales only include ASCII letters and the following symbols: -, _, @, =, and ;
        // Convert to wstring.
        std::wstring loc(locale.begin(), locale.end());
        fwprintf(stream, L"%s{\n", loc.c_str());
        fflush(stream);

        for (UCollationStrength strength : colfStrengths)
        {
            ucol_setStrength(collator.get(), strength);
            std::unordered_map<std::u16string, std::u16string> collationFoldingMap{ create_collation_folding_map(
                collator.get(), strength, rootCollationFoldingMap) };
            
            if (locale == "root")
            {
                rootCollationFoldingMap.insert( {strength, collationFoldingMap} );
            }

            print_collation_folding_map(to_map(collationFoldingMap), strength, stream);
        }

        fwprintf(stream, L"}\n");
        fflush(stream);
        fclose(stream);
    }

    void run(const char* inDir, const char* outDir)
    {
        // Get collation locales.
        std::set<std::string> locales;
        for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(inDir))
        {
            std::string fileName = dirEntry.path().string();
            std::replace(fileName.begin(), fileName.end(), '\\', '/');
            fileName = fileName.substr(fileName.find_last_of('/') + 1);

            size_t end = fileName.find(".txt");
            if (end == std::string::npos)
            {
                continue;
            }
            std::string locale = fileName.substr(0, end);

            // Collation folding data for 'root' locale is generated separately, prior to other locales.
            if (locale != "root")
            {
                locales.insert(locale);
            }
        }

        // Generate 'root' collation folding data first.
        std::unordered_map<UCollationStrength, std::unordered_map<std::u16string, std::u16string>>
            rootCollationFoldingMap{};
        run_locale("root", outDir, rootCollationFoldingMap);

        // Only generate collation folding data on locales that support the 'search' collation type.
        for (const auto& locale : locales)
        {
            if (!supports_search_collation(locale.c_str()))
            {
                fprintf(stderr, "No 'search' collation type for locale: %s.\n", locale.c_str());
                continue;
            }
            run_locale(locale, outDir, rootCollationFoldingMap);
        }
    }
}

int main(int argc, char **argv)
{
    UErrorCode status = U_ZERO_ERROR;
    const char *inDir = nullptr;
    const char *outDir = nullptr;
    const char *copyright = nullptr;

    //
    // Pick up and check the command line arguments,
    // using the standard ICU tool utils option handling.
    //
    U_MAIN_INIT_ARGS(argc, argv);
    progName = argv[0];
    argc = u_parseArgs(argc, argv, UPRV_LENGTHOF(options), options);
    if (argc < 0)
    {
        // Unrecognized option
        fprintf(stderr, "error in command line argument \"%s\"\n", argv[-argc]);
    }
    if (options[0].doesOccur || options[1].doesOccur)
    {
        //  -? or -h for help.
        usageAndDie(0);
    }
    if (!(options[2].doesOccur && options[3].doesOccur))
    {
        fprintf(stderr, "locale and outDir must be specified.\n");
        usageAndDie(U_ILLEGAL_ARGUMENT_ERROR);
    }
    inDir = options[2].value;
    outDir = options[3].value;

    try
    {
        run(inDir, outDir);
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
