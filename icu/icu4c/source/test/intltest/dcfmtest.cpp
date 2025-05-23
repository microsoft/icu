// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 2002-2014, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

//
//   dcfmtest.cpp
//
//     Decimal Formatter tests, data driven. 
//

#include "intltest.h"

#if !UCONFIG_NO_FORMATTING && !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "unicode/regex.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/unistr.h"
#include "unicode/dcfmtsym.h"
#include "unicode/decimfmt.h"
#include "unicode/locid.h"
#include "cmemory.h"
#include "dcfmtest.h"
#include "util.h"
#include "cstring.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(__GLIBCXX__)
namespace std { class type_info; } // WORKAROUND: http://llvm.org/bugs/show_bug.cgi?id=13364
#endif

#include <string>
#include <iostream>

//---------------------------------------------------------------------------
//
//  Test class boilerplate
//
//---------------------------------------------------------------------------
DecimalFormatTest::DecimalFormatTest()
{
}


DecimalFormatTest::~DecimalFormatTest()
{
}



void DecimalFormatTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite DecimalFormatTest: ");
    switch (index) {

#if !UCONFIG_NO_FILE_IO
        case 0: name = "DataDrivenTests";
            if (exec) DataDrivenTests();
            break;
#else
        case 0: name = "skip";
            break;
#endif

        default: name = "";
            break; //needed to end loop
    }
}


//---------------------------------------------------------------------------
//
//   Error Checking / Reporting macros used in all of the tests.
//
//---------------------------------------------------------------------------
#define DF_CHECK_STATUS UPRV_BLOCK_MACRO_BEGIN { \
    if (U_FAILURE(status)) { \
        dataerrln("DecimalFormatTest failure at line %d.  status=%s", \
                  __LINE__, u_errorName(status)); \
        return 0; \
    } \
} UPRV_BLOCK_MACRO_END

#define DF_ASSERT(expr) UPRV_BLOCK_MACRO_BEGIN { \
    if ((expr)==false) { \
        errln("DecimalFormatTest failure at line %d.\n", __LINE__); \
    } \
} UPRV_BLOCK_MACRO_END

#define DF_ASSERT_FAIL(expr, errcode) UPRV_BLOCK_MACRO_BEGIN { \
    UErrorCode status=U_ZERO_ERROR; \
    (expr); \
    if (status!=errcode) { \
        dataerrln("DecimalFormatTest failure at line %d.  Expected status=%s, got %s", \
                  __LINE__, u_errorName(errcode), u_errorName(status)); \
    } \
} UPRV_BLOCK_MACRO_END

#define DF_CHECK_STATUS_L(line) UPRV_BLOCK_MACRO_BEGIN { \
    if (U_FAILURE(status)) { \
        errln("DecimalFormatTest failure at line %d, from %d.  status=%d\n",__LINE__, (line), status); \
    } \
} UPRV_BLOCK_MACRO_END

#define DF_ASSERT_L(expr, line) UPRV_BLOCK_MACRO_BEGIN { \
    if ((expr)==false) { \
        errln("DecimalFormatTest failure at line %d, from %d.", __LINE__, (line)); \
        return; \
    } \
} UPRV_BLOCK_MACRO_END



//
//  InvariantStringPiece
//    Wrap a StringPiece around the extracted invariant data of a UnicodeString.
//    The data is guaranteed to be nul terminated.  (This is not true of StringPiece
//    in general, but is true of InvariantStringPiece)
//
class InvariantStringPiece: public StringPiece {
  public:
    InvariantStringPiece(const UnicodeString &s);
    ~InvariantStringPiece() {}
  private:
    MaybeStackArray<char, 20>  buf;
};

InvariantStringPiece::InvariantStringPiece(const UnicodeString &s) {
    int32_t  len = s.length();
    if (len+1 > buf.getCapacity()) {
        buf.resize(len+1);
    }
    // Buffer size is len+1 so that s.extract() will nul-terminate the string.
    s.extract(0, len, buf.getAlias(), len+1, US_INV);
    this->set(buf.getAlias(), len);
}


//  UnicodeStringPiece
//    Wrap a StringPiece around the extracted (to the default charset) data of
//    a UnicodeString.  The extracted data is guaranteed to be nul terminated.
//    (This is not true of StringPiece in general, but is true of UnicodeStringPiece)
//
class UnicodeStringPiece: public StringPiece {
  public:
    UnicodeStringPiece(const UnicodeString &s);
    ~UnicodeStringPiece() {}
  private:
    MaybeStackArray<char, 20>  buf;
};

UnicodeStringPiece::UnicodeStringPiece(const UnicodeString &s) {
    int32_t  len = s.length();
    int32_t  capacity = buf.getCapacity();
    int32_t requiredCapacity = s.extract(0, len, buf.getAlias(), capacity) + 1;
    if (capacity < requiredCapacity) {
        buf.resize(requiredCapacity);
        capacity = requiredCapacity;
        s.extract(0, len, buf.getAlias(), capacity);
    }
    this->set(buf.getAlias(), requiredCapacity - 1);
}



//---------------------------------------------------------------------------
//
//      DataDrivenTests  
//             The test cases are in a separate data file,
//
//---------------------------------------------------------------------------

// Translate a Formattable::type enum value to a string, for error message formatting.
static const char *formattableType(Formattable::Type typ) {
    static const char *types[] = {"kDate",
                                  "kDouble",
                                  "kLong",
                                  "kString",
                                  "kArray",
                                  "kInt64",
                                  "kObject"
                                  };
    if (typ<0 || typ>Formattable::kObject) {
        return "Unknown";
    }
    return types[typ];
}

const char *
DecimalFormatTest::getPath(char *buffer, const char *filename) {
    UErrorCode status=U_ZERO_ERROR;
    const char *testDataDirectory = IntlTest::getSourceTestData(status);
    DF_CHECK_STATUS;

    strcpy(buffer, testDataDirectory);
    strcat(buffer, filename);
    return buffer;
}

void DecimalFormatTest::DataDrivenTests() {
    char tdd[2048];
    const char *srcPath;
    UErrorCode  status  = U_ZERO_ERROR;
    int32_t     lineNum = 0;

    //
    //  Open and read the test data file.
    //
    srcPath=getPath(tdd, "dcfmtest.txt");
    if(srcPath==NULL) {
        return; /* something went wrong, error already output */
    }

    int32_t    len;
    UChar *testData = ReadAndConvertFile(srcPath, len, "utf-8", status);
    if (U_FAILURE(status)) {
        return; /* something went wrong, error already output */
    }

    //
    //  Put the test data into a UnicodeString
    //
    UnicodeString testString(false, testData, len);

    RegexMatcher    parseLineMat(UnicodeString(
            "(?i)\\s*parse\\s+"
            "\"([^\"]*)\"\\s+"           // Capture group 1: input text
            "([ild])\\s+"                // Capture group 2: expected parsed type
            "\"([^\"]*)\"\\s+"           // Capture group 3: expected parsed decimal
            "\\s*(?:#.*)?"),             // Trailing comment
         0, status);

    RegexMatcher    formatLineMat(UnicodeString(
            "(?i)\\s*format\\s+"
            "(\\S+)\\s+"                 // Capture group 1: pattern
            "(ceiling|floor|down|up|halfeven|halfdown|halfup|default|unnecessary)\\s+"  // Capture group 2: Rounding Mode
            "\"([^\"]*)\"\\s+"           // Capture group 3: input
            "\"([^\"]*)\""               // Capture group 4: expected output
            "\\s*(?:#.*)?"),             // Trailing comment
         0, status);

    RegexMatcher    commentMat    (UNICODE_STRING_SIMPLE("\\s*(#.*)?$"), 0, status);
    RegexMatcher    lineMat(UNICODE_STRING_SIMPLE("(?m)^(.*?)$"), testString, 0, status);

    if (U_FAILURE(status)){
        dataerrln("Construct RegexMatcher() error.");
        delete [] testData;
        return;
    }

    //
    //  Loop over the test data file, once per line.
    //
    while (lineMat.find()) {
        lineNum++;
        if (U_FAILURE(status)) {
            dataerrln("File dcfmtest.txt, line %d: ICU Error \"%s\"", lineNum, u_errorName(status));
        }

        status = U_ZERO_ERROR;
        UnicodeString testLine = lineMat.group(1, status);
        // printf("%s\n", UnicodeStringPiece(testLine).data());
        if (testLine.length() == 0) {
            continue;
        }

        //
        // Parse the test line.  Skip blank and comment only lines.
        // Separate out the three main fields - pattern, flags, target.
        //

        commentMat.reset(testLine);
        if (commentMat.lookingAt(status)) {
            // This line is a comment, or blank.
            continue;
        }


        //
        //  Handle "parse" test case line from file
        //
        parseLineMat.reset(testLine);
        if (parseLineMat.lookingAt(status)) {
            execParseTest(lineNum,
                          parseLineMat.group(1, status),    // input
                          parseLineMat.group(2, status),    // Expected Type
                          parseLineMat.group(3, status),    // Expected Decimal String
                          status
                          );
            continue;
        }

        //
        //  Handle "format" test case line
        //
        formatLineMat.reset(testLine);
        if (formatLineMat.lookingAt(status)) {
            execFormatTest(lineNum,
                           formatLineMat.group(1, status),    // Pattern
                           formatLineMat.group(2, status),    // rounding mode
                           formatLineMat.group(3, status),    // input decimal number
                           formatLineMat.group(4, status),    // expected formatted result
                           kFormattable,
                           status);

            execFormatTest(lineNum,
                           formatLineMat.group(1, status),    // Pattern
                           formatLineMat.group(2, status),    // rounding mode
                           formatLineMat.group(3, status),    // input decimal number
                           formatLineMat.group(4, status),    // expected formatted result
                           kStringPiece,
                           status);
            continue;
        }

        //
        //  Line is not a recognizable test case.
        //
        errln("Badly formed test case at line %d.\n%s\n", 
             lineNum, UnicodeStringPiece(testLine).data());

    }

    delete [] testData;
}



void DecimalFormatTest::execParseTest(int32_t lineNum,
                                     const UnicodeString &inputText,
                                     const UnicodeString &expectedType,
                                     const UnicodeString &expectedDecimal,
                                     UErrorCode &status) {
    
    if (U_FAILURE(status)) {
        return;
    }

    DecimalFormatSymbols symbols(Locale::getUS(), status);
    UnicodeString pattern = UNICODE_STRING_SIMPLE("####");
    DecimalFormat format(pattern, symbols, status);
    Formattable   result;
    if (U_FAILURE(status)) {
        dataerrln("file dcfmtest.txt, line %d: %s error creating the formatter.",
            lineNum, u_errorName(status));
        return;
    }

    ParsePosition pos;
    int32_t expectedParseEndPosition = inputText.length();

    format.parse(inputText, result, pos);

    if (expectedParseEndPosition != pos.getIndex()) {
        errln("file dcfmtest.txt, line %d: Expected parse position afeter parsing: %d. Actual parse position: %d", lineNum, expectedParseEndPosition, pos.getIndex());
        return;
    }

    char   expectedTypeC[2];
    expectedType.extract(0, 1, expectedTypeC, 2, US_INV);
    Formattable::Type expectType = Formattable::kDate;
    switch (expectedTypeC[0]) {
      case 'd': expectType = Formattable::kDouble; break;
      case 'i': expectType = Formattable::kLong;   break;
      case 'l': expectType = Formattable::kInt64;  break;
      default:
          errln("file dcfmtest.tx, line %d: unrecognized expected type \"%s\"",
              lineNum, InvariantStringPiece(expectedType).data());
          return;
    }
    if (result.getType() != expectType) {
        errln("file dcfmtest.txt, line %d: expectedParseType(%s) != actual parseType(%s)",
             lineNum, formattableType(expectType), formattableType(result.getType()));
        return;
    }

    StringPiece decimalResult = result.getDecimalNumber(status);
    if (U_FAILURE(status)) {
        errln("File %s, line %d: error %s.  Line in file dcfmtest.txt:  %d:",
            __FILE__, __LINE__, u_errorName(status), lineNum);
        return;
    }

    InvariantStringPiece expectedResults(expectedDecimal);
    if (decimalResult != expectedResults) {
        errln("file dcfmtest.txt, line %d: expected \"%s\", got \"%s\"",
            lineNum, expectedResults.data(), decimalResult.data());
    }
    
    return;
}


void DecimalFormatTest::execFormatTest(int32_t lineNum,
                           const UnicodeString &pattern,     // Pattern
                           const UnicodeString &round,       // rounding mode
                           const UnicodeString &input,       // input decimal number
                           const UnicodeString &expected,    // expected formatted result
                           EFormatInputType inType,          // input number type
                           UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }

    DecimalFormatSymbols symbols(Locale::getUS(), status);
    // printf("Pattern = %s\n", UnicodeStringPiece(pattern).data());
    DecimalFormat fmtr(pattern, symbols, status);
    if (U_FAILURE(status)) {
        dataerrln("file dcfmtest.txt, line %d: %s error creating the formatter.",
            lineNum, u_errorName(status));
        return;
    }
    if (round=="ceiling") {
        fmtr.setRoundingMode(DecimalFormat::kRoundCeiling);
    } else if (round=="floor") {
        fmtr.setRoundingMode(DecimalFormat::kRoundFloor);
    } else if (round=="down") {
        fmtr.setRoundingMode(DecimalFormat::kRoundDown);
    } else if (round=="up") {
        fmtr.setRoundingMode(DecimalFormat::kRoundUp);
    } else if (round=="halfeven") {
        fmtr.setRoundingMode(DecimalFormat::kRoundHalfEven);
    } else if (round=="halfdown") {
        fmtr.setRoundingMode(DecimalFormat::kRoundHalfDown);
    } else if (round=="halfup") {
        fmtr.setRoundingMode(DecimalFormat::kRoundHalfUp);
    } else if (round=="default") {
        // don't set any value.
    } else if (round=="unnecessary") {
        fmtr.setRoundingMode(DecimalFormat::kRoundUnnecessary);
    } else {
        fmtr.setRoundingMode(DecimalFormat::kRoundFloor);
        errln("file dcfmtest.txt, line %d: Bad rounding mode \"%s\"",
                lineNum, UnicodeStringPiece(round).data());
    }

    const char *typeStr = "Unknown";
    UnicodeString result;
    UnicodeStringPiece spInput(input);

    switch (inType) {
    case kFormattable:
        {
            typeStr = "Formattable";
            Formattable fmtbl;
            fmtbl.setDecimalNumber(spInput, status);
            fmtr.format(fmtbl, result, NULL, status);
        }
        break;
    case kStringPiece:
        typeStr = "StringPiece";
        fmtr.format(spInput, result, NULL, status);
        break;
    }

    if ((status == U_FORMAT_INEXACT_ERROR) && (result == "") && (expected == "Inexact")) {
        // Test succeeded.
        status = U_ZERO_ERROR;
        return;
    }

    if (U_FAILURE(status)) {
        errln("[%s] file dcfmtest.txt, line %d: format() returned %s.",
            typeStr, lineNum, u_errorName(status));
        status = U_ZERO_ERROR;
        return;
    }
    
    if (result != expected) {
        errln("[%s] file dcfmtest.txt, line %d: expected \"%s\", got \"%s\", %s",
            typeStr, lineNum, UnicodeStringPiece(expected).data(), UnicodeStringPiece(result).data(),
            u_errorName(status));
    }
}


#endif  /* !UCONFIG_NO_REGULAR_EXPRESSIONS  */

