// © Microsoft Corporation. All rights reserved.

#include <algorithm>
#include <array>
#include <functional>
#include <map>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include <filesystem>
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
    struct collation_element final
    {
        uint16_t primary;
        uint8_t secondary;
        uint8_t tertiary;
    };

    struct collation_key_sequence final
    {
        std::vector<collation_element> items;

        constexpr bool operator==(const collation_key_sequence& other) const noexcept
        {
            if (items.size() != other.items.size())
            {
                return false;
            }

            for (size_t index = 0; index < items.size(); ++index)
            {
                if (items[index].primary != other.items[index].primary)
                {
                    return false;
                }
            }

            for (size_t index = 0; index < items.size(); ++index)
            {
                if (items[index].secondary != other.items[index].secondary)
                {
                    return false;
                }
            }

            for (size_t index = 0; index < items.size(); ++index)
            {
                if (items[index].tertiary != other.items[index].tertiary)
                {
                    return false;
                }
            }

            return true;
        }

        constexpr bool operator<(const collation_key_sequence& other) const noexcept
        {
            if (items.size() != other.items.size())
            {
                return items.size() < other.items.size();
            }

            for (size_t index = 0; index < items.size(); ++index)
            {
                if (items[index].primary != other.items[index].primary)
                {
                    return items[index].primary < other.items[index].primary;
                }
            }

            for (size_t index = 0; index < items.size(); ++index)
            {
                if (items[index].secondary != other.items[index].secondary)
                {
                    return items[index].secondary < other.items[index].secondary;
                }
            }

            for (size_t index = 0; index < items.size(); ++index)
            {
                if (items[index].tertiary != other.items[index].tertiary)
                {
                    return items[index].tertiary < other.items[index].tertiary;
                }
            }

            return false;
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
            constexpr size_t prime = 1099511628211ULL;
#else
            constexpr size_t seed = 2166136261U;
            constexpr size_t prime = 16777619U;
#endif

            size_t hash{ seed };

            for (collation_element item : value.items)
            {
                hash ^= static_cast<size_t>(item.primary);
                hash *= prime;
            }

            for (collation_element item : value.items)
            {
                hash ^= static_cast<size_t>(item.secondary);
                hash *= prime;
            }

            for (collation_element item : value.items)
            {
                hash ^= static_cast<size_t>(item.tertiary);
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

    constexpr bool is_ignorable_collation_element(const collation_element &value)
    {
        return value.primary == 0 && value.secondary == 0 && value.tertiary == 0;
    }

    void remove_duplicate_ignorable_collation_elements(std::vector<collation_element> &items)
    {
        auto iterator = items.begin();
        ++iterator;

        for (size_t index = 1; index < items.size(); ++index) {
            if (is_ignorable_collation_element(items[index]) &&
                is_ignorable_collation_element(items[index - 1])) {
                --index;
                items.erase(iterator);
            } else {
                ++iterator;
            }
        }
    }


    collation_key_sequence get_collation_key_sequence(
        std::u16string_view text, const UCollator* collator, UCollationStrength strength)
    {
        unique_UCollationElements elements{ ucol_open_elements_cpp(collator, text) };

        std::vector<collation_element> items{};

        int32_t value{};

        while (ucol_try_next_cpp(elements.get(), value))
        {
            uint16_t primary{ ucol_primary_order_cpp(value) };
            uint8_t secondary{ strength < UCollationStrength::UCOL_SECONDARY ? 0u : ucol_secondary_order_cpp(value) };
            uint8_t tertiary{ strength < UCollationStrength::UCOL_TERTIARY ? 0u : ucol_tertiary_order_cpp(value) };
            items.emplace_back(primary, secondary, tertiary);
        }

        remove_duplicate_ignorable_collation_elements(items);

        return collation_key_sequence{ items };
    }

    void add_collation_key_sequence_item(std::u16string_view text, const UCollator* collator,
        UCollationStrength strength,
        std::unordered_map<collation_key_sequence, std::vector<std::u16string>>& textByCollationKeySequence)
    {
        textByCollationKeySequence[get_collation_key_sequence(text, collator, strength)].emplace_back(text);
    }

    void add_collation_key_sequence_item(char32_t value, const UCollator* collator, UCollationStrength strength,
        std::unordered_map<collation_key_sequence, std::vector<std::u16string>>& textByCollationKeySequence)
    {
        icu_utf32_to_utf16_converter valueUtf16{ value };

        if (!valueUtf16.has_surrogate_pair())
        {
            char16_t nonSurrogateValue{ valueUtf16.non_surrogate_value() };
            std::u16string_view buffer{ &nonSurrogateValue, 1 };
            add_collation_key_sequence_item(buffer, collator, strength, textByCollationKeySequence);
        }
        else
        {
            std::array<char16_t, 2> buffer{ valueUtf16.lead_surrogate(), valueUtf16.trail_surrogate() };
            std::u16string_view bufferView{ buffer.data(), buffer.size() };
            add_collation_key_sequence_item(bufferView, collator, strength, textByCollationKeySequence);
        }
    }

    void add_code_point_collation_key_sequence_items(const UCollator* collator, UCollationStrength strength,
        std::unordered_map<collation_key_sequence, std::vector<std::u16string>>& textByCollationKeySequence)
    {
        for (char32_t item = u'\0'; item <= U'\U0010FFFF'; ++item)
        {
            if (item >= static_cast<char32_t>(0xD800) && item <= static_cast<char32_t>(0xDFFF))
            {
                continue;
            }

            add_collation_key_sequence_item(item, collator, strength, textByCollationKeySequence);
        }
    }

    void add_contraction_and_prefix_collation_key_sequence_items(const UCollator* collator, UCollationStrength strength,
        std::unordered_map<collation_key_sequence, std::vector<std::u16string>>& textByCollationKeySequence)
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
            add_collation_key_sequence_item(contractionString, collator, strength, textByCollationKeySequence);
        }
    }

    std::unordered_map<collation_key_sequence, std::vector<std::u16string>> create_collation_key_sequence_map(
        const UCollator* collator, UCollationStrength strength)
    {
        std::unordered_map<collation_key_sequence, std::vector<std::u16string>> result{};
        add_code_point_collation_key_sequence_items(collator, strength, result);
        add_contraction_and_prefix_collation_key_sequence_items(collator, strength, result);
        return result;
    }

    enum class canonical_case_class
    {
        all_lowercase,
        some_lowercase,
        mixed_case,
        some_uppercase,
        all_uppercase,
        invalid
    };

    canonical_case_class get_canonical_case_class(std::u16string_view value)
    {
        size_t size{};
        size_t lowercase{};
        size_t uppercase{};
        icu_utf16_to_utf32_view view{ value };

        for (icu_utf16_to_utf32_item item : view)
        {
            if (!item.is_code_point())
            {
                // An unpaired surrogate is the least preferrable case class.
                return canonical_case_class::invalid;
            }

            ++size;
            int8_t type{ u_charType(item.code_point()) };

            if (type == UCharCategory::U_LOWERCASE_LETTER)
            {
                ++lowercase;
            }
            else if (type == UCharCategory::U_UPPERCASE_LETTER || type == UCharCategory::U_TITLECASE_LETTER)
            {
                ++uppercase;
            }
        }

        if (lowercase == size)
        {
            // A string is lowercase unless it has at least one non-lowercase character (or invalid code unit sequence).
            // (i.e., an empty string is considered lowercase)
            return canonical_case_class::all_lowercase;
        }
        else if (uppercase == size)
        {
            return canonical_case_class::all_uppercase;
        }
        else if (lowercase > 0 && uppercase == 0)
        {
            return canonical_case_class::some_lowercase;
        }
        else if (lowercase == 0 && uppercase < size)
        {
            return canonical_case_class::some_uppercase;
        }
        else
        {
            return canonical_case_class::mixed_case;
        }
    }

    // Technically, returns code point size + count of invalid code units.
    size_t get_code_point_size(std::u16string_view value)
    {
        size_t result{};
        icu_utf16_to_utf32_view view{ value };

        for (icu_utf16_to_utf32_iterator iterator = view.begin(); iterator != view.end(); ++iterator)
        {
            ++result;
        }

        return result;
    }

    bool has_cjk_compatibilty_code_point(std::u16string_view value)
    {
        icu_utf16_to_utf32_view view{ value };

        for (const auto& item : view)
        {
            if (!item.is_code_point())
            {
                continue;
            }

            char32_t codePoint{ item.code_point() };
            constexpr char32_t block1Start{ u'\uF900' };
            constexpr char32_t block1End{ u'\uFAFF' };
            constexpr char32_t block2Start{ U'\U0002F800' };
            constexpr char32_t block2End{ U'\U0002FA1F' };

            if (codePoint >= block1Start && codePoint <= block1End)
            {
                return true;
            }
            else if (codePoint >= block2Start && codePoint <= block2End)
            {
                return true;
            }
        }

        return false;
    }

    bool is_better_canonical_item(std::u16string_view left, std::u16string_view right)
    {
        size_t leftCodePointSize{ get_code_point_size(left) };
        size_t rightCodePointSize{ get_code_point_size(right) };

        if (leftCodePointSize != rightCodePointSize)
        {
            return leftCodePointSize > rightCodePointSize;
        }

        canonical_case_class leftCaseClass{ get_canonical_case_class(left) };
        canonical_case_class rightCaseClass{ get_canonical_case_class(right) };

        if (leftCaseClass != rightCaseClass)
        {
            return leftCaseClass < rightCaseClass;
        }

        bool leftCjkCompatibilty{ has_cjk_compatibilty_code_point(left) };
        bool rightCjkCompatibility{ has_cjk_compatibilty_code_point(right) };

        if (leftCjkCompatibilty != rightCjkCompatibility)
        {
            return !leftCjkCompatibilty;
        }

        icu_utf16_to_utf32_view leftView{ left };
        icu_utf16_to_utf32_view rightView{ right };
        auto rightIterator = rightView.begin();

        for (auto leftIterator = leftView.begin(); leftIterator != leftView.end() && rightIterator != rightView.end();
             ++leftIterator, ++rightIterator)
        {
            auto leftItem = *leftIterator;
            auto rightItem = *rightIterator;

            bool leftIsCodePoint{ leftItem.is_code_point() };
            bool rightIsCodePoint{ rightItem.is_code_point() };

            if (leftIsCodePoint != rightIsCodePoint)
            {
                return leftIsCodePoint;
            }

            if (!leftIsCodePoint)
            {
                continue;
            }

            char32_t leftCodePoint{ leftItem.code_point() };
            char32_t rightCodePoint{ rightItem.code_point() };

            if (leftCodePoint != rightCodePoint)
            {
                return leftCodePoint < rightCodePoint;
            }
        }

        return false;
    }

    std::u16string get_canonical_item(const std::vector<std::u16string>& equivalenceClass)
    {
        if (equivalenceClass.empty())
        {
            assert(false);
            return {};
        }

        std::u16string result{ equivalenceClass[0] };

        for (size_t index = 1; index < equivalenceClass.size(); ++index)
        {
            const std::u16string_view& item{ equivalenceClass[index] };

            if (is_better_canonical_item(item, result))
            {
                result = item;
            }
        }

        return result;
    }

    void add_single_collation_element_folding(
        std::unordered_map<collation_key_sequence, std::vector<std::u16string>>& textByCollationKeySequence,
        std::unordered_map<collation_key_sequence, std::u16string>& canonicalTextByCollationKeySequence,
        std::unordered_map<std::u16string, std::u16string>& collationFolding)
    {
        for (const auto& pair : textByCollationKeySequence)
        {
            if (pair.first.items.size() != 1)
            {
                continue;
            }

            const std::vector<std::u16string>& equivalenceClass{ pair.second };

            std::u16string canonicalText{ get_canonical_item(equivalenceClass) };
            canonicalTextByCollationKeySequence.emplace(pair.first, canonicalText);

            if (equivalenceClass.size() == 1)
            {
                continue;
            }

            for (const auto& text : equivalenceClass)
            {
                if (text == canonicalText)
                {
                    continue;
                }

                collationFolding.emplace(text, canonicalText);
            }
        }
    }

    std::u16string combine(const std::u16string first, const std::u16string& second)
    {
        std::u16string result{};
        result.reserve(first.size() + second.size());
        result.append(first);
        result.append(second);
        return result;
    }

    std::u16string combine(const std::u16string first, const std::u16string& second, const std::u16string& third)
    {
        std::u16string result{};
        result.reserve(first.size() + second.size() + third.size());
        result.append(first);
        result.append(second);
        result.append(third);
        return result;
    }

    bool add_if_find_all_and_not_exists(
        const std::unordered_map<collation_key_sequence, std::u16string>& canonicalTextByCollationKeySequence,
        const std::vector<collation_key_sequence>& sections, std::vector<std::u16string>& equivalenceClass)
    {
        std::vector<collation_key_sequence> filteredSections{ sections };

        for (collation_key_sequence& sequence : filteredSections)
        {
            remove_duplicate_ignorable_collation_elements(sequence.items);
        }

        for (auto iterator = filteredSections.begin(); iterator != filteredSections.end();)
        {
            if (iterator->items.empty())
            {
                filteredSections.erase(iterator);
            }
            else
            {
                ++iterator;
            }
        }

        std::u16string canonicalText{};

        for (const auto& item : filteredSections)
        {
            auto findResult = canonicalTextByCollationKeySequence.find(item);

            if (findResult == canonicalTextByCollationKeySequence.end())
            {
                return false;
            }

            canonicalText.append(findResult->second);
        }

        if (std::find(equivalenceClass.begin(), equivalenceClass.end(), canonicalText) == equivalenceClass.end())
        {
            equivalenceClass.emplace_back(canonicalText);
        }

        return true;
    }

    void add_double_collation_element_folding(
        std::unordered_map<collation_key_sequence, std::vector<std::u16string>>& textByCollationKeySequence,
        std::unordered_map<collation_key_sequence, std::u16string>& canonicalTextByCollationKeySequence,
        std::unordered_map<std::u16string, std::u16string>& collationFolding)
    {
        for (auto& pair : textByCollationKeySequence)
        {
            const collation_key_sequence& collationKeySequence{ pair.first };
            std::vector<std::u16string>& equivalenceClass{ pair.second };

            if (collationKeySequence.items.size() != 2)
            {
                continue;
            }

            // element1, element2
            collation_element first{ collationKeySequence.items[0] };
            collation_element second{ collationKeySequence.items[1] };
            collation_key_sequence section1{ std::vector<collation_element>{ first } };
            collation_key_sequence section2{ std::vector<collation_element>{ second } };

            add_if_find_all_and_not_exists(
                canonicalTextByCollationKeySequence, { section1, section2 }, equivalenceClass);

            std::u16string canonicalText{ get_canonical_item(equivalenceClass) };
            canonicalTextByCollationKeySequence.emplace(pair.first, canonicalText);

            if (equivalenceClass.size() == 1)
            {
                continue;
            }

            for (const auto& text : equivalenceClass)
            {
                if (text == canonicalText)
                {
                    continue;
                }

                collationFolding.emplace(text, canonicalText);
            }
        }
    }

    void add_triple_collation_element_folding(
        std::unordered_map<collation_key_sequence, std::vector<std::u16string>>& textByCollationKeySequence,
        std::unordered_map<collation_key_sequence, std::u16string>& canonicalTextByCollationKeySequence,
        std::unordered_map<std::u16string, std::u16string>& collationFolding)
    {
        for (auto& pair : textByCollationKeySequence)
        {
            const collation_key_sequence& collationKeySequence{ pair.first };
            std::vector<std::u16string>& equivalenceClass{ pair.second };

            if (collationKeySequence.items.size() != 3)
            {
                continue;
            }

            collation_element first{ collationKeySequence.items[0] };
            collation_element second{ collationKeySequence.items[1] };
            collation_element third{ collationKeySequence.items[2] };

            // element1, element2, element3
            {
                collation_key_sequence section1{ std::vector<collation_element>{ first } };
                collation_key_sequence section2{ std::vector<collation_element>{ second } };
                collation_key_sequence section3{ std::vector<collation_element>{ third } };

                add_if_find_all_and_not_exists(
                    canonicalTextByCollationKeySequence, { section1, section2, section3 }, equivalenceClass);
            }

            // element1+element2, element3
            {
                collation_key_sequence section1{ std::vector<collation_element>{ first, second } };
                collation_key_sequence section2{ std::vector<collation_element>{ third } };
                add_if_find_all_and_not_exists(
                    canonicalTextByCollationKeySequence, { section1, section2 }, equivalenceClass);
            }

            // element1, element2+element3
            {
                collation_key_sequence section1{ std::vector<collation_element>{ first } };
                collation_key_sequence section2{ std::vector<collation_element>{ second, third } };
                add_if_find_all_and_not_exists(
                    canonicalTextByCollationKeySequence, { section1, section2 }, equivalenceClass);
            }

            std::u16string canonicalText{ get_canonical_item(equivalenceClass) };
            canonicalTextByCollationKeySequence.emplace(pair.first, canonicalText);

            if (equivalenceClass.size() == 1)
            {
                continue;
            }

            for (const auto& text : equivalenceClass)
            {
                if (text == canonicalText)
                {
                    continue;
                }

                collationFolding.emplace(text, canonicalText);
            }
        }
    }

    void add_quadruple_collation_element_folding(
        std::unordered_map<collation_key_sequence, std::vector<std::u16string>>& textByCollationKeySequence,
        std::unordered_map<collation_key_sequence, std::u16string>& canonicalTextByCollationKeySequence,
        std::unordered_map<std::u16string, std::u16string>& collationFolding)
    {
        for (auto& pair : textByCollationKeySequence)
        {
            const collation_key_sequence& collationKeySequence{ pair.first };
            std::vector<std::u16string>& equivalenceClass{ pair.second };

            if (collationKeySequence.items.size() != 4)
            {
                continue;
            }

            collation_element first{ collationKeySequence.items[0] };
            collation_element second{ collationKeySequence.items[1] };
            collation_element third{ collationKeySequence.items[2] };
            collation_element fourth{ collationKeySequence.items[3] };

            // element1, element2, element3, element4
            {
                collation_key_sequence section1{ std::vector<collation_element>{ first } };
                collation_key_sequence section2{ std::vector<collation_element>{ second } };
                collation_key_sequence section3{ std::vector<collation_element>{ third } };
                collation_key_sequence section4{ std::vector<collation_element>{ fourth } };

                add_if_find_all_and_not_exists(
                    canonicalTextByCollationKeySequence, { section1, section2, section3, section4 }, equivalenceClass);
            }

            // element1+element2, element3, element4
            {
                collation_key_sequence section1{ std::vector<collation_element>{ first, second } };
                collation_key_sequence section2{ std::vector<collation_element>{ third } };
                collation_key_sequence section3{ std::vector<collation_element>{ fourth } };

                add_if_find_all_and_not_exists(
                    canonicalTextByCollationKeySequence, { section1, section2, section3 }, equivalenceClass);
            }

            // element1, element2+element3, element4
            {
                collation_key_sequence section1{ std::vector<collation_element>{ first } };
                collation_key_sequence section2{ std::vector<collation_element>{ second, third } };
                collation_key_sequence section3{ std::vector<collation_element>{ fourth } };

                add_if_find_all_and_not_exists(
                    canonicalTextByCollationKeySequence, { section1, section2, section3 }, equivalenceClass);
            }

            // element1, element2, element3+element4
            {
                collation_key_sequence section1{ std::vector<collation_element>{ first } };
                collation_key_sequence section2{ std::vector<collation_element>{ second } };
                collation_key_sequence section3{ std::vector<collation_element>{ third, fourth } };

                add_if_find_all_and_not_exists(
                    canonicalTextByCollationKeySequence, { section1, section2, section3 }, equivalenceClass);
            }

            // element1, element2+element3+element4
            {
                collation_key_sequence section1{ std::vector<collation_element>{ first } };
                collation_key_sequence section2{ std::vector<collation_element>{ second, third, fourth } };

                add_if_find_all_and_not_exists(
                    canonicalTextByCollationKeySequence, { section1, section2 }, equivalenceClass);
            }

            // element1+element2+element3, element4
            {
                collation_key_sequence section1{ std::vector<collation_element>{ first, second, third } };
                collation_key_sequence section2{ std::vector<collation_element>{ fourth } };
                
                add_if_find_all_and_not_exists(
                    canonicalTextByCollationKeySequence, { section1, section2 }, equivalenceClass);
            }

            // element1+element2, element3+element4
            {
                collation_key_sequence section1{ std::vector<collation_element>{ first, second } };
                collation_key_sequence section2{ std::vector<collation_element>{ third, fourth } };
                
                add_if_find_all_and_not_exists(
                    canonicalTextByCollationKeySequence, { section1, section2 }, equivalenceClass);
            }

            std::u16string canonicalText{ get_canonical_item(equivalenceClass) };
            canonicalTextByCollationKeySequence.emplace(pair.first, canonicalText);

            if (equivalenceClass.size() == 1)
            {
                continue;
            }

            for (const auto& text : equivalenceClass)
            {
                if (text == canonicalText)
                {
                    continue;
                }

                collationFolding.emplace(text, canonicalText);
            }
        }
    }

    void add_remaining_collation_element_folding(size_t minimumSize,
        std::unordered_map<collation_key_sequence, std::vector<std::u16string>>& textByCollationKeySequence,
        std::unordered_map<collation_key_sequence, std::u16string>& canonicalTextByCollationKeySequence,
        std::unordered_map<collation_key_sequence, std::vector<std::u16string>>& incomplete,
        std::unordered_map<std::u16string, std::u16string>& collationFolding)
    {
        for (auto& pair : textByCollationKeySequence)
        {
            const collation_key_sequence& collationKeySequence{ pair.first };
            std::vector<std::u16string>& equivalenceClass{ pair.second };

            if (collationKeySequence.items.size() < minimumSize)
            {
                continue;
            }

            bool itemIncomplete{ true };

            std::vector<collation_element> elements{};
            elements.reserve(collationKeySequence.items.size());

            for (const auto& item : collationKeySequence.items)
            {
                elements.push_back(item);
            }

            // Only handles the simplest pattern:
            // element1, element2, element3, ..., elementN
            // A more general version of the full logic in add_single/double/triple/etc/collation_element_folding is not
            // yet written.
            {
                std::vector<collation_key_sequence> sections{};
                sections.reserve(collationKeySequence.items.size());

                for (const auto& element : elements)
                {
                    sections.emplace_back(collation_key_sequence{ { element } });
                }

                if (add_if_find_all_and_not_exists(canonicalTextByCollationKeySequence, sections, equivalenceClass))
                {
                    itemIncomplete = false;
                }
            }

            if (equivalenceClass.size() == 1)
            {
                continue;
            }

            if (itemIncomplete)
            {
                incomplete.emplace(collationKeySequence, equivalenceClass);
            }

            std::u16string canonicalText{ get_canonical_item(equivalenceClass) };
            canonicalTextByCollationKeySequence.emplace(pair.first, canonicalText);

            for (const auto& text : equivalenceClass)
            {
                if (text == canonicalText)
                {
                    continue;
                }

                collationFolding.emplace(text, canonicalText);
            }
        }
    }

    std::unordered_map<std::u16string, std::u16string> create_collation_folding_map(
        std::unordered_map<collation_key_sequence, std::vector<std::u16string>>& textByCollationKeySequence,
        std::unordered_map<collation_key_sequence, std::vector<std::u16string>>& incomplete)
    {
        std::unordered_map<collation_key_sequence, std::u16string> canonicalTextByCollationKeySequence{};
        std::unordered_map<std::u16string, std::u16string> result{};
        add_single_collation_element_folding(textByCollationKeySequence, canonicalTextByCollationKeySequence, result);
        add_double_collation_element_folding(textByCollationKeySequence, canonicalTextByCollationKeySequence, result);
        add_triple_collation_element_folding(textByCollationKeySequence, canonicalTextByCollationKeySequence, result);
        add_quadruple_collation_element_folding(
            textByCollationKeySequence, canonicalTextByCollationKeySequence, result);
        add_remaining_collation_element_folding(
            5, textByCollationKeySequence, canonicalTextByCollationKeySequence, incomplete, result);
        return result;
    }

    std::unordered_map<std::u16string, std::u16string> create_collation_folding_map(const UCollator* collator,
        UCollationStrength strength,
        std::unordered_map<collation_key_sequence, std::vector<std::u16string>>& incomplete)
    {
        std::unordered_map<collation_key_sequence, std::vector<std::u16string>> textByCollationKeySequence{
            create_collation_key_sequence_map(collator, strength)
        };
        return create_collation_folding_map(textByCollationKeySequence, incomplete);
    }

    std::map<collation_key_sequence, std::vector<std::u16string>> to_map(
        const std::unordered_map<collation_key_sequence, std::vector<std::u16string>>& value)
    {
        std::map<collation_key_sequence, std::vector<std::u16string>> result{};

        for (const auto& pair : value)
        {
            result.emplace(pair.first, pair.second);
        }

        return result;
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

    constexpr char16_t to_hex_digit(uint8_t value) noexcept
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

    std::u16string to_utf32_debug_string(std::u16string_view text)
    {
        std::u32string textUtf32{ u_str_to_utf32_cpp(text) };
        std::u16string result{};
        result.reserve(textUtf32.size() * 7); // rough lower bound; " U+XXXX" per character

        for (size_t index = 0; index < textUtf32.size(); ++index)
        {
            if (index > 0)
            {
                result.push_back(L' ');
            }

            //result.push_back(L'U');
            //result.push_back(L'+');
            char32_t item{ textUtf32[index] };

            if (item <= u'\uFFFF')
            {
                result.push_back(to_hex_digit(static_cast<uint8_t>((item & 0xF000) >> 12)));
                result.push_back(to_hex_digit(static_cast<uint8_t>((item & 0xF00) >> 8)));
                result.push_back(to_hex_digit(static_cast<uint8_t>((item & 0xF0) >> 4)));
                result.push_back(to_hex_digit(static_cast<uint8_t>(item & 0xF)));
            }
            else if (item <= U'\U000FFFFF')
            {
                result.push_back(to_hex_digit(static_cast<uint8_t>((item & 0xF0000) >> 16)));
                result.push_back(to_hex_digit(static_cast<uint8_t>((item & 0xF000) >> 12)));
                result.push_back(to_hex_digit(static_cast<uint8_t>((item & 0xF00) >> 8)));
                result.push_back(to_hex_digit(static_cast<uint8_t>((item & 0xF0) >> 4)));
                result.push_back(to_hex_digit(static_cast<uint8_t>(item & 0xF)));
            }
            else if (item <= U'\U0010FFFF')
            {
                result.push_back(to_hex_digit(static_cast<uint8_t>((item & 0xF00000) >> 20)));
                result.push_back(to_hex_digit(static_cast<uint8_t>((item & 0xF0000) >> 16)));
                result.push_back(to_hex_digit(static_cast<uint8_t>((item & 0xF000) >> 12)));
                result.push_back(to_hex_digit(static_cast<uint8_t>((item & 0xF00) >> 8)));
                result.push_back(to_hex_digit(static_cast<uint8_t>((item & 0xF0) >> 4)));
                result.push_back(to_hex_digit(static_cast<uint8_t>(item & 0xF)));
            }
            else
            {
                std::terminate();
            }
        }

        return result;
    }

    void add_hex_8(uint8_t value, std::u16string& result)
    {
        result.push_back(to_hex_digit(static_cast<uint8_t>((value & 0xF0) >> 4)));
        result.push_back(to_hex_digit(static_cast<uint8_t>(value & 0xF)));
    }

    void add_hex_16(uint16_t value, std::u16string& result)
    {
        add_hex_8(static_cast<uint8_t>((value & 0xFF00) >> 8), result);
        add_hex_8(static_cast<uint8_t>(value & 0xFF), result);
    }

    std::u16string to_string(const collation_key_sequence& value)
    {
        std::u16string result{};
        result.reserve(2 + (value.items.size() * 3) * 2);

        for (collation_element collationElement : value.items)
        {
            add_hex_16(collationElement.primary, result);
        }

        result.push_back(u' ');

        for (collation_element collationElement : value.items)
        {
            add_hex_8(collationElement.secondary, result);
        }

        result.push_back(u' ');

        for (collation_element collationElement : value.items)
        {
            add_hex_8(collationElement.tertiary, result);
        }

        return result;
    }

    void print_collation_key_sequence_map(
        FILE* output, const std::map<collation_key_sequence, std::vector<std::u16string>>& value)
    {
        for (const auto& sequencePair : value)
        {
            const collation_key_sequence& from{ sequencePair.first };

            for (const std::u16string& to : sequencePair.second)
            {
                std::u16string toDisplay{ to };

                // The console apparently treats this character as EOF (or an error that triggers EOF-like
                // behavior).
                if (toDisplay.size() == 1 && toDisplay[0] == u'\uFFFF')
                {
                    toDisplay = u"";
                }

                fwprintf(output, L"%s => %s (%s)\n", reinterpret_cast<const wchar_t*>(to_string(from).c_str()),
                    reinterpret_cast<const wchar_t*>(to.c_str()),
                    reinterpret_cast<const wchar_t*>(to_utf32_debug_string(to).c_str()));
                fflush(output);
            }
        }
    }

    void print_collation_folding_map(FILE *output, const std::map<std::u16string, std::u16string>& value, UCollationStrength strength,
        const std::unordered_map<UCollationStrength, std::unordered_map<std::u16string, std::u16string>> &rootCollationFoldingMap, bool isRoot)
    {
        bool printStrengthOpenBracket = true;
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

            if (!isRoot)
            {
                // Collation folding already exists in root.
                if (rootCollationFoldingMap.count(strength) && rootCollationFoldingMap.at(strength).count(fromDisplay) &&
                    rootCollationFoldingMap.at(strength).at(fromDisplay) == to)
                {
                    continue;
                }
            }

            // At least one mapping exists for the current strength level.
            // We may not get here for certains locales if it matches root data at the current strength level.
            if (printStrengthOpenBracket)
            {
                fwprintf(output, L"\t%s{\n", strength_to_string(strength));
                fflush(output);
                printStrengthOpenBracket = false;
            }

            // Escape the following characters.
            size_t backslashIndex = to.find(u"\\");
            size_t quoteIndex = to.find(u"\"");
            if (backslashIndex != std::u16string::npos)
            {
                to.insert(backslashIndex, u"\\");
            }
            if (quoteIndex != std::u16string::npos)
            {
                to.insert(quoteIndex, u"\\");
            }

            fwprintf(output, L"\t\t%s{\"%s\"}\n",
                     reinterpret_cast<const wchar_t*>(to_utf32_debug_string(fromDisplay).c_str()),
                     reinterpret_cast<const wchar_t*>(to.c_str()));
            fflush(output);
        }

        // Mapping at current strength level existed.
        if (!printStrengthOpenBracket)
        {
            fwprintf(output, L"\t}\n");
            fflush(output);
        }
    }

    bool supports_search_collation(const char* locale)
    {
        icu_resource_search_result searchResult{};
        unique_UEnumeration keywords = ucol_getKeywordValuesForLocale_cpp("collation", locale, false, searchResult);
        if (searchResult != icu_resource_search_result::exact_match)
        {
            // root always has 'search' collation type, we only care about whether the specific locale has additional data.
            return false;
        }

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

    bool run_locale(std::string locale, bool printKeySequenceMap, const char* outDir, 
        std::unordered_map<UCollationStrength, std::unordered_map<std::u16string, std::u16string>>& rootCollationFoldingMap)
    {
        std::string searchLocale(locale);
        searchLocale.append("-u-co-search");
        icu_resource_search_result searchResult{};
        unique_UCollator collator{ ucol_open_cpp(searchLocale.c_str(), searchResult) };
        if (searchResult != icu_resource_search_result::exact_match)
        {
            // Specific locale does not support 'search' collation type, and a fallback locale would be used. Skip.
            printf("SKIPPING. Locale uses fallback data in ucol_open: %s.\n", locale.c_str());
            return true;
        }
        printf("Generating collation folding data for locale: %s.\n", locale.c_str());

        UErrorCode status = U_ZERO_ERROR;
        uprv_mkdir(outDir, &status);
        if (U_FAILURE(status))
        {
            fprintf(stderr, "Error creating colf directory: %s\n\n", outDir);
            exit(-1);
        }

        FILE* output;
        std::string filename(outDir);
        filename.append("/").append(locale).append(".txt");
        output = fopen(filename.c_str(), "w+,ccs=UTF-8");
        if (output == nullptr)
        {
            fprintf(stderr, "Cannot open file \"%s\"\n\n", filename.c_str());
            exit(-1);
        }

        // TODO: Include copyright?
        fwprintf(output, L"// Generated using gencolf.exe, built from icu4c/source/tools/gencolf.\n");
        fflush(output);
        
        // ICU locales only include ASCII letters and the following symbols: -, _, @, =, and ;
        // Convert to wstring.
        std::wstring loc(locale.begin(), locale.end());
        fwprintf(output, L"%s{\n", loc.c_str());
        fflush(output);

        bool hasIncomplete = false;
        for (UCollationStrength strength : colfStrengths)
        {
            ucol_setStrength(collator.get(), strength);

            if (printKeySequenceMap)
            {
                wprintf(L"Collation key sequences:\n");
                fflush(stdout);

                std::unordered_map<collation_key_sequence, std::vector<std::u16string>> collationKeySequenceMap{
                    create_collation_key_sequence_map(collator.get(), strength)
                };
                print_collation_key_sequence_map(stdout, to_map(collationKeySequenceMap));
            }
            else
            {
                wprintf(L"Generating %s collation folding map... ", strength_to_string(strength));
                fflush(stdout);

                std::unordered_map<collation_key_sequence, std::vector<std::u16string>> incomplete{};
                std::unordered_map<std::u16string, std::u16string> collationFoldingMap{ create_collation_folding_map(
                    collator.get(), strength, incomplete) };

                // Save rootCollationFoldingMap to consolidate data when writing to output file.
                bool isRoot = (locale == "root");
                if (isRoot)
                {
                    rootCollationFoldingMap[strength] = collationFoldingMap;
                }

                wprintf(L"[done]\n");
                fflush(stdout);

                hasIncomplete = !incomplete.empty();

                if (hasIncomplete)
                {
                    fwprintf(
                        stderr, L"WARNING: The following items may have incomplete collation folding data generated:\n");
                    fflush(stderr);
                    print_collation_key_sequence_map(stderr, to_map(incomplete));
                }
                else
                {
                    wprintf(L"Successfully generated collation folding for all items.\n");
                }

                print_collation_folding_map(output, to_map(collationFoldingMap), strength, rootCollationFoldingMap, isRoot);
                
                if (hasIncomplete)
                {
                    break;
                }
            }
        }

        fwprintf(output, L"}\n");
        fflush(output);
        fclose(output);

        return !hasIncomplete;
    }

    bool run(const char* inDir, const char* outDir)
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
        constexpr bool printKeySequenceMap{ false };
        std::unordered_map<UCollationStrength, std::unordered_map<std::u16string, std::u16string>>
            rootCollationFoldingMap{};
        bool completed = run_locale("root", printKeySequenceMap, outDir, rootCollationFoldingMap);

        for (const auto& locale : locales)
        {
            // Only generate collation folding data on locales that support the 'search' collation type.
            if (!supports_search_collation(locale.c_str()))
            {
                printf("SKIPPING. Locale uses fallback data in ucol_getKeywordValuesForLocale: %s.\n", locale.c_str());
                continue;
            }

            // de__PHONEBOOK and es__TRADITIONAL explicitly target non-'search' collation types.
            if (locale == "de__PHONEBOOK" || locale == "es__TRADITIONAL")
            {
                continue;
            }

            completed = run_locale(locale, printKeySequenceMap, outDir, rootCollationFoldingMap);
            if (!completed)
            {
                break;
            }
        }

        return completed;
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
        if (!run(inDir, outDir))
        {
            return 2;
        }
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
