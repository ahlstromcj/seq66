/*
 *  This file is part of seq66.
 *
 *  seq24 is free software; you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option) any later
 *  version.
 *
 *  seq24 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with seq24; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          strfunctions.cpp
 *
 *    Provides the implementations for safe replacements for the various C
 *    file functions.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-24
 * \updates       2023-10-22
 * \version       $Revision$
 *
 *    We basically include only the functions we need for Seq66, not
 *    much more than that.  These functions are adapted from our xpc_basic
 *    project.
 */

#include <cctype>                       /* std::toupper() function          */
#include <climits>                      /* INT_MAX and its ilk              */
#include <cmath>                        /* std::floor(), std::pow()         */
#include <cstring>                      /* std::memcmp() function           */
#include <stdexcept>                    /* std::invalid_argument            */

#include "util/strfunctions.hpp"        /* free functions in seq66 n'space  */

#if defined SEQ66_PLATFORM_WINDOWS
#include <windows.h>                    /* ::MultiByteToWideChar()          */
#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Returns double quotes as a string.
 */

const std::string &
double_quotes ()
{
    static const std::string s_double_quotes = std::string("\"\"");
    return s_double_quotes;
}

/**
 *  Returns "" as a string for external callers.
 */

std::string
empty_string ()
{
    return double_quotes();
}

/**
 *  Provides a way to work with a visible empty string (e.g. in a
 *  configuration file).  This function returns true if the string really is
 *  empty, or just contains two double quotes ("").
 *  See the add_quotes() and double_quotes() functions.
 */

bool
is_empty_string (const std::string & item)
{
    return item.empty() || item == double_quotes();
}

const std::string &
questionable_string ()
{
    static const std::string s_question_mark = std::string("?");
    return s_question_mark;
}

bool
is_questionable_string (const std::string & item)
{
    return item == questionable_string();
}

bool
is_missing_string (const std::string & item)
{
    return is_empty_string(item) || is_questionable_string(item);
}

/**
 *  Indicates if one string can be found within another.  Doesn't force the
 *  caller to use size_type.
 */

bool
contains (const std::string & original, const std::string & target)
{
    auto pos = original.find(target);
    return pos != std::string::npos;
}

/**
 *  Strips the comments from a string.  The algorithm moves to the "#"
 *  character, backs up to the first non-space before that character, and
 *  removes all characters after that character.  We also want to be able to
 *  have "#" in quotes be preserved.
 *
 * \param item
 *      Provides the string to be comment-stripped.
 *
 * \return
 *      Returns the string data from the first non-space to the last
 *      non-space before the hash-tag.
 */

std::string
strip_comments (const std::string & item)
{
    std::string result = item;
    auto hashpos = result.find_first_of("#");
    auto qpos = result.find_first_of("\"'");
    if (qpos != std::string::npos)
    {
        char quotechar[2] = { 'x', 0 };
        quotechar[0] = result[qpos];

        auto qpos2 = result.find_first_of(quotechar, qpos + 1);
        if (qpos2 != std::string::npos)
        {
            if (hashpos > qpos2)
                result = result.substr(0, hashpos);
        }
        else
            result = result.substr(0, hashpos);
    }
    else
    {
        if (hashpos != std::string::npos)
            result = result.substr(0, hashpos);
    }
    result = trim(result, SEQ66_TRIM_CHARS);
    return result;
}

/**
 *  Gets the next quoted string.
 */

std::string
next_quoted_string (const std::string & source, std::string::size_type pos)
{
    std::string result;
    auto lpos = source.find_first_of(double_quotes(), pos);
    if (lpos != std::string::npos)
    {
        auto rpos = source.find_first_of(double_quotes(), lpos + 1);
        if (rpos != std::string::npos)
        {
            size_t len = size_t(rpos - lpos - 1);
            if (len > 0)
                result = source.substr(lpos + 1, len);
        }
    }
    return result;
}

/**
 *  Gets the next bracket string.  It looks for sqaure brackets instead of
 *  double quotes, and it trims space at each end of the string.
 */

std::string
next_bracketed_string
(
    const std::string & source,
    std::string::size_type pos
)
{
    std::string result;
    auto lpos = source.find_first_of("[", pos);
    if (lpos != std::string::npos)
    {
        auto rpos = source.find_first_of("]", lpos + 1);
        if (rpos != std::string::npos)
        {
            size_t len = size_t(rpos - lpos - 1);
            if (len > 0)
            {
                result = trim(source.substr(lpos + 1, len));
            }
        }
    }
    return result;
}

/**
 *  Strips single- or double-quotes from a string.  Meant mainly for removing
 *  quotes around a file-name, so it works only if the first character is a
 *  quote, and the last character is a quote.
 *
 * \param item
 *      The string to be massaged.
 *
 * \return
 *      The string without quotes.  If it didn't have any, the string
 *      should be unchanged.
 */

std::string
strip_quotes (const std::string & item)
{
    std::string result;
    if (! item.empty())
    {
        result = item;
        auto fpos = result.find_first_of("\"");
        if (fpos == 0)
        {
            auto lpos = result.find_last_of("\"");
            auto end_index = result.length() - 1;
            if (lpos != std::string::npos && lpos == end_index)
                result = result.substr(1, end_index - 1);
        }
        else
        {
            fpos = result.find_first_of("'");
            if (fpos == 0)
            {
                auto lpos = result.find_last_of("'");
                auto end_index = result.length() - 1;
                if (lpos != std::string::npos && lpos == end_index)
                    result = result.substr(1, end_index - 1);
            }
        }
    }
    return result;
}

/**
 *  Wraps a string in double-quotes.  It works only if the first character is
 *  not a quote, and the last character is not a quote.
 *
 * \param item
 *      The string to be massaged.
 *
 * \return
 *      The string with double-quotes.  If it already had them, the string
 *      should be unchanged.  If the string was empty, a string consisting of
 *      two double-quotes is returned.
 */

std::string
add_quotes (const std::string & item)
{
    std::string result = item;
    if (result.empty())
    {
        result = double_quotes();
    }
    else
    {
        bool quoted = false;
        auto pos0 = result.find_first_of("\"");
        auto pos1 = result.find_last_of("\"");
        if (pos0 != std::string::npos && pos1 != std::string::npos)
        {
            if (pos1 != pos0)
                quoted = pos0 == 0 && pos1 == result.length() - 1;
        }
        if (! quoted)
            result = "\"" + item + "\"";
    }
    return result;
}

/**
 *  A simple test for std::string::npos.  Why?  I dunno, less typing for the
 *  developer.  Not used in this module, however.
 */

bool
not_npos (std::string::size_type p)
{
    return p != std::string::npos;
}

/**
 *  Compares two strings for equality up to the given length.  Unlike
 *  std::string::compare(), it returns only a boolean.  Meant to be a simpler
 *  alternative, and analogous to strncmp().  The comparison function was
 *  std::string::compare(), but it's semantics don't quite work here.
 *
 *  We want to see if the "comparing" string matches the "compared" string up
 *  to n characters or to the full length of the compared string.
 *
 * \param a
 *      Provides the "compared" string. False is returned if it is empty.  It
 *      is the "target", the string whose contents we want to match.  It is
 *      generally the shorter of the two strings, but no promises.
 *
 * \param b
 *      Provides the "comparing" string. False is returned if it is empty.  It
 *      is the "source" string, the string whose contents we want to determine
 *      if it matches the "target" string.
 *
 * \param n
 *      Provides the number of characters in the "comparing" string (parameter
 *      \a a) that must match.  If equal to 0 (the default value), then the
 *      minimum of the lengths of a and b is used.
 *
 * \return
 *      Returns true if the strings compare identically for the first \a n
 *      characters, but see \a n above. Returns false otherwise, including
 *      when n > a.length().
 */

bool
strncompare (const std::string & a, const std::string & b, size_t n)
{
    bool result = ! a.empty() && ! b.empty();
    if (result)
    {
        if (n == 0)
            n = std::min(a.length(), b.length());

        if (n <= a.length() && b.length() >= n)
        {
#if defined USE_SLOW_CODE
            for (size_t i = 0; i < n; ++i)
            {
                if (a[i] != b[i])
                {
                    result = false;
                    break;
                }
            }
#else
            result = std::memcmp(a.data(), b.data(), n) == 0;
#endif
        }
        else
            result = false;
    }
    return result;
}

/**
 *  Performs a case-insensitive comparison of two characters.
 *
 * \return
 *      Returns true if the characters match via the std::toupper() call.
 */

static inline bool
casecompare (char a, char b)
{
    return
    (
        std::toupper(static_cast<unsigned char>(a)) ==
            std::toupper(static_cast<unsigned char>(b))
    );
}

/**
 *  Performs a case-insensitive comparison of two strings.
 *
 * \return
 *      Returns true if the strings match via the casecompare() call.  The
 *      strings must be the same length.
 */

bool
strcasecompare (const std::string & a, const std::string & b)
{
    return
    (
        (a.size() == b.size()) &&
            std::equal(a.begin(), a.end(), b.begin(), casecompare)
    );
}

/*
 * There are implementations of these functions using the std::find_if()
 * algorithm, but the implementations here seem simpler and more flexible.
 */

/**
 *  Left-trims a set of characters from the string.
 *
 * \param str
 *      The prospective string to be trimmed.  It must be a reference, and
 *      must be non-const.
 *
 * \param chars
 *      The set of characters to be trimmed.  Defaults to SEQ66_TRIM_CHARS
 *      (" \t\n\v\f\r").  Another macro available is SEQ66_TRIM_CHARS_QUOTES
 *      (" \t\n\v\f\r\"'") which adds the double- and single-quote
 *      characters.
 *
 * \return
 *      Returns a reference to the string that was left-trimmed.
 */

std::string &
ltrim (std::string & str, const std::string & chars)
{
    str.erase(0, str.find_first_not_of(chars));
    return str;
}

/**
 *  Right-trims a set of characters from the string.  Similar to the ltrim()
 *  function.
 *
 * \param str
 *      The prospective string to be trimmed.  It must be a reference, and
 *      must be non-const.
 *
 * \param chars
 *      The set of characters to be trimmed.
 *
 * \return
 *      Returns a reference to the string that was right-trimmed.
 */

std::string &
rtrim (std::string & str, const std::string & chars)
{
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}

/**
 *  Left- and right-trims a set of characters from the string.  Similar to the
 *  ltrim() and rtrim() functions combined.
 *
 * \param str
 *      The prospective string to be trimmed.  It must be a reference, and
 *      must be non-const.
 *
 * \param chars
 *      The set of characters to be trimmed.  The default is SEQ66_TRIM_CHARS,
 *      which is white-space characters.
 *
 * \return
 *      Returns a reference to the string that was left-right-trimmed.
 */

std::string
trim (const std::string & str, const std::string & chars)
{
    std::string result = str;
    (void) ltrim(rtrim(result, chars), chars);
    return result;
}

/**
 *  Replaces the first n occurrences of a sub-string with the specified string.
 *
 * \param source
 *      Provides the original string to be modified.
 *
 * \param target
 *      Provides the sub-string to be replaced.
 *
 * \param replacement
 *      Provides the replacement for the substring.
 *
 * \param n
 *      Indicates how many occurrences to replace.  The default value is -1,
 *      which means to replace all of them.
 *
 * \return
 *      Returns the modified copy of the string, with the replacement made.
 *      If the target string was not found in the source, the source string
 *      should be unchanged.
 */

std::string
string_replace
(
    const std::string & source,
    const std::string & target,
    const std::string & replacement,
    int n
)
{
    std::string result = source;
    auto targetsize = target.size();
    auto targetloc = result.find(target);
    while (targetloc != std::string::npos)
    {
        (void) result.replace(targetloc, targetsize, replacement);
        targetloc = result.find(target);
        if (n > 0)
        {
            --n;
            if (n == 0)
                break;
        }
    }
    return result;
}

#if defined USE_HAS_DIGIT                   /* we now use try/catch         */

static bool
has_digit (const std::string & s, bool floating = false)
{
    bool result = false;
    if (! s.empty())
    {
        int count = 0;
        for (const auto c : s)
        {
            if (c == '-' || c == '+')
            {
                if (count++ == 0)
                    continue;               /* skip leading sign character  */
                else
                    break;                  /* end the search               */
            }
            if (floating && c == '.')
            {
                if (count++ == 0)
                    continue;               /* skip leading decimal point   */
                else
                    break;                  /* end the search               */
            }
            if (std::isdigit(c))
            {
                result = true;
                break;
            }
            else if (std::isspace(c))
            {
                count = 0;
                continue;
            }
            else
                break;
        }
    }
    return result;
}

#endif

/**
 *  A useful function for the midi_bytes-to-from-string functions below.
 *
 * \param c
 *      Provides a hexadecimal digit. This must be a number or a lower-case
 *      hex digit.
 *
 * \return
 *      Returns the value of the hexadecimal digit.  If not a hex digit,
 *      then (-1) is returned.
 */

int
hex_digit (char c)
{
    static std::string s_hex_digits = "0123456789abcdef";
    int result = (-1);
    auto pos = s_hex_digits.find_first_of(c);
    if (pos != std::string::npos)
        result = int(pos);

    return result;
}

/**
 *  This pair of functions work to make sure that all ASCII characters in
 *  the string are restricted to the range of 0 to 127.  This is done by
 *  encoding characters of value 128 to 255 in the format "\xx" where "xx" is
 *  two hexadecimal digits. For simplicity the x's are always lower-case.
 *  They are also zero-padded to two bytes.
 *
 * \param s
 *      Provides the string to be "midi-ized".
 *
 * \param limit
 *      Provides the maximum number of characters allowed in the converted
 *      string.  Defaults to 0, which means "no limit", i.e. a very large
 *      value.
 */

std::string
string_to_midi_bytes (const std::string & s, size_t limit)
{
    int maximum = limit == 0 ? INT_MAX : int(limit) ;
    std::string result;
    for (const auto c : s)
    {
        unsigned char b = (unsigned char) (c);      /* if bigger than 127   */
        if (b > 127)
        {
            if (maximum >= 3)
            {
                char tmp[4];
                int count = snprintf(tmp, sizeof tmp, "\\%02x", b);
                maximum -= count;
                result += tmp;
            }
            else
                break;
        }
        else
        {
            result.push_back(c);
            if (--maximum == 0)
                break;
        }
    }
    return result;
}

std::string
midi_bytes_to_string (const std::string & s)
{
    auto bslashpos = s.find_first_of("\\");
    if (bslashpos != std::string::npos)
    {
        std::string result;
        bool slashed = false;
        int sum = 0;
        int hexcount = 0;
        for (const auto c : s)
        {
            if (slashed)
            {
                int value = hex_digit(c);
                if (value >= 0)
                {
                    ++hexcount;
                    if (hexcount == 1)
                        sum += value * 16;
                    else if (hexcount == 2)
                    {
                        result.push_back(c);
                        slashed = false;
                        hexcount = 0;
                    }
                }
                else
                {
                    result.push_back(c);
                    slashed = false;
                    hexcount = 0;
                }
            }
            else
            {
                if (c == '\\')
                    slashed = true;
                else
                    result.push_back(c);
            }
        }
        return result;
    }
    else
        return s;
}

/**
 *  Converts a string value to a boolean.  Note that an empty string returns
 *  the default value, which is false if the caller does not supply a default
 *  parameter.
 *
 * \param s
 *      The string representing the value. "1", "true", "on", and "yes" are
 *      true, all other values are false.  Capitalized versions checked.
 *
 * \param defalt
 *      The desired default for an empty string.  The default \a defalt value
 *      is false.
 */

bool
string_to_bool (const std::string & s, bool defalt)
{
    return s.empty() ? defalt :
    (
        s == "1" || s == "true" || s == "on" || s == "yes"
    );
}

bool
string_to_int_pair
(
    const std::string & s,
    int & v1, int & v2,
    const std::string & delimiter
)
{
    bool result = s.find_first_of(delimiter) != std::string::npos;
    if (result)
    {
        tokenization numbers = tokenize(s, delimiter);
        result = numbers.size() == 2;
        if (result)
        {
            result = std::isdigit(numbers[0][0]) && std::isdigit(numbers[1][0]);
            if (result)
            {
                v1 = string_to_int(numbers[0]);
                v2 = string_to_int(numbers[1]);
            }
        }
    }
    return result;
}

bool
string_to_time_signature (const std::string & s, int & beats, int & width)
{
    return string_to_int_pair(s, beats, width, "/");
}

/**
 *  Converts a string to a double value.  This function bypasses characters
 *  until it finds a digit (whether part of the number or a "0x" construct),
 *  and then converts it.  The strtol() function is used with a base of 0 so
 *  that decimal, hexadecimal, and octal values can all be parsed.
 *
 *  This function is the base implementation for string_to_int() as well.
 *
 * \param s
 *      Provides the string to convert to a double value. Integers, numbers
 *      with a decimal point, and simple, but strictly formatted, fractions
 *      (e.g. "1/4") are supported.
 *
 * \param defalt
 *      The desired default for an empty string.  The default \a defalt value
 *      is 0.0.
 *
 * \param rounding
 *      If not 0 (the default), then round the value to the given number of
 *      decimal places.  This prevents values with messy extra digits past the
 *      decimal.
 *
 * \return
 *      Returns the signed long integer value represented by the string.
 *      If the string is empty or has no digits, then 0.0 is returned.
 */

double
string_to_double (const std::string & s, double defalt, int rounding)
{
    double result = defalt;
    if (! s.empty())
    {
        try
        {
            int beats, width;
            bool is_time_sig = string_to_time_signature(s, beats, width);
            if (is_time_sig)
            {
                double numerator = double(beats);
                double denominator = double(width);
                result = numerator / denominator;
            }
            else
                result = std::stod(s, nullptr);

            if (rounding > 0)
            {
                double power = std::pow(10.0, rounding);
                result = std::floor(result * power) / power;
            }
        }
        catch (std::invalid_argument const &)
        {
            // no code
        }
    }
    return result;
}

std::string
double_to_string (double value, int precision)
{
    char temp[32];
    if (precision == 0)
        (void) snprintf(temp, sizeof temp, "%g", value);
    else
        (void) snprintf(temp, sizeof temp, "%*g", precision, value);

    return std::string(temp);
}

float
string_to_float (const std::string & s, float defalt, int rounding)
{
    return float(string_to_double(s, double(defalt), rounding));
}

/**
 *  Converts a string to a signed long value.  This function bypasses
 *  characters until it finds a digit (whether part of the number or a "0x"
 *  construct), and then converts it.  The strtol() function is used with a
 *  base of 0 so that decimal, hexadecimal, and octal values can all be
 *  parsed.
 *
 *  This function is the base implementation for string_to_int() as well.
 *
 * \param s
 *      Provides the string to convert to a signed long integer.
 *
 * \param defalt
 *      The desired default for an empty string. The default \a defalt value
 *      is 0.
 *
 * \return
 *      Returns the signed long integer value represented by the string.
 *      If the string is empty or has no digits, then the default value is
 *      returned.
 */

long
string_to_long (const std::string & s, long defalt)
{
    long result = defalt;
    try
    {
        result = std::stol(s, nullptr, 0);
    }
    catch (std::invalid_argument const &)
    {
        // no code
    }
    return result;
}

std::string
long_to_string (long value)
{
    char temp[32];
    (void) snprintf(temp, sizeof temp, "%ld", value);
    return std::string(temp);
}

/**
 *  Converts a string to an unsigned long integer.
 */

unsigned long
string_to_unsigned_long (const std::string & s, unsigned long defalt)
{
    double result = defalt;
    try
    {
        result = std::stoul(s, nullptr, 0);
    }
    catch (std::invalid_argument const &)
    {
        // no code
    }
    return result;
}

/**
 *  Converts a string to an unsigned integer.
 */

unsigned
string_to_unsigned (const std::string & s, unsigned defalt)
{
    return unsigned(string_to_unsigned_long(s, (unsigned long)(defalt)));
}

/**
 *  Converts a string to an integer.  Simply calls string_to_long().
 *
 * \param s
 *      Provides the string to convert to an integer.
 *
 * \return
 *      Returns the integer value represented by the string.
 */

int
string_to_int (const std::string & s, int defalt)
{
    return int(string_to_long(s, long(defalt)));
}

std::string
int_to_string (int value)
{
    char temp[32];
    (void) snprintf(temp, sizeof temp, "%d", value);
    return std::string(temp);
}

/**
 *  Tests that a string is not empty and has non-space characters.  Provides
 *  essentially the opposite test that string_is_void() provides.  The
 *  definition of white-space is provided by the std::isspace()
 *  function/macro.
 *
 * \param s
 *      The string pointer to check for emptiness.
 *
 * \return
 *      Returns true if the pointer is valid, the string has a non-zero
 *      length, and is not just white-space.
 */

bool
string_not_void (const std::string & s)
{
   bool result = false;
   if (! s.empty())
   {
      for (int i = 0; i < int(s.length()); ++i)
      {
         if (! std::isspace(s[i]))
         {
            result = true;
            break;
         }
      }
   }
   return result;
}

/**
 *  Tests that a string is empty or has only white-space characters.  Meant to
 *  have essentially the opposite result of string_not_void().  The meaning of
 *  empty is special here, as it refers to a string being useless as a token:
 *
 *      -  The string is of zero length.
 *      -  The string has only white-space characters in it, where the
 *         isspace() macro provides the definition of white-space.
 *
 * \param s
 *      The string pointer to check for emptiness.
 *
 * \return
 *      Returns true if the string has a zero length, or is only
 *      white-space.
 */

bool
string_is_void (const std::string & s)
{
   bool result = s.empty();
   if (! result)
      result = ! string_not_void(s);

   return result;
}

/**
 *  Compares two strings for a form of semantic equality, for the purposes of
 *  editable_event(), for example.  The strings_match() function returns true
 *  if the comparison items are identical, without case-sensitivity in
 *  character content up to the length of the secondary string.  This allows
 *  abbreviations to match. (And, in scanning routines, the first match is
 *  immediately accepted.)
 *
 * \param target
 *      The primary string in the comparison.  This is the target string, the
 *      one we hope to match.  It is <i> assumed </i> to be non-empty, and the
 *      result is false if it is empty.
 *
 * \param x
 *      The secondary string in the comparison.  It must be no longer than the
 *      target string, or the match is false.
 *
 * \return
 *      Returns true if both strings are are identical in characters, up to
 *      the length of the secondary string, with the case of the characters
 *      being insignificant.  Otherwise, false is returned.
 */

bool
strings_match (const std::string & target, const std::string & x)
{
    bool result = ! target.empty();
    if (result)
    {
        result = x.length() <= target.length();
        if (result)
        {
            for (int i = 0; i < int(x.length()); ++i)
            {
                if (std::tolower(x[i]) != std::tolower(target[i]))
                {
                    result = false;
                    break;
                }
            }
        }
    }
    return result;
}

/**
 *  Returns the source string with all characters converted to lowercase.
 */

std::string
tolower (const std::string & source)
{
    std::string result;
    for (auto c : source)
    {
        char c2 = std::tolower(c);
        result += c2;
    }
    return result;
}

/**
 *  Returns the source string with all characters converted to uppercase.
 */

std::string
toupper (const std::string & source)
{
    std::string result;
    for (auto c : source)
    {
        char c2 = std::toupper(c);
        result += c2;
    }
    return result;
}

/**
 *  Easy conversion from boolean to string, "true" or "false".
 */

std::string
bool_to_string (bool x)
{
    static const std::string s_true { "true" };
    static const std::string s_false { "false" };
    return x ? s_true : s_false ;
}

/**
 *  Easy conversion from boolean to character, "T" or "F".
 */

char
bool_to_char (bool x)
{
    static char s_true { 'T' };
    static char s_false { 'F' };
    return x ? s_true : s_false ;
}

std::string
pointer_to_string (void * ptr)
{
    /*
     * long long int value = reinterpret_cast<ptr>;
     */

    char temp[32];
    snprintf(temp, sizeof temp, "0x%p", ptr);
    return std::string(temp);
}

/**
 *  Tokenizes a substanza, defined as the text between square brackets,
 *  including the square brackets.
 *
 *  Other code uses sscanf() to extract data from within "[ ]".  See
 *  midicontrolfile::parse_midi_control_out() for an example.
 *
 * \param tokens
 *      Provides a vector into which to push the tokens.  The destination for
 *      all of the strings found in parsing.
 *
 * \param source
 *      The substanza to be parsed and tokenized.
 *
 * \param bleft
 *      The position in the source at which to start the parsing.  The default
 *      value is 0.
 *
 * \param brackets
 *      Provides the starting and ending characters for the token in a two
 *      character string.  The default is empty, in which case "[]" is
 *      implied.
 *
 * \return
 *      Returns the number of tokens pushed (i.e. the final size of the tokens
 *      vector).  If it returns 0, there are no tokens, and the destination
 *      vector has been cleared.  If it returns 2, then the two tokens are "["
 *      and "]"... the substanza is empty of data.
 */

int
tokenize_stanzas
(
    tokenization & tokens,
    const std::string & source,
    std::string::size_type bleft,
    const std::string & brackets
)
{
    static std::string s_delims = SEQ66_TRIM_CHARS;         /* isspace()?   */
    std::string BL = "[";
    std::string BR = "]";
    char CBR = ']';
    if (brackets.size() >= 2)
    {
        BL = brackets[0];
        BR = brackets[1];
        CBR = brackets[1];
    }
    tokens.clear();
    bleft = source.find_first_of(BL, bleft);
    if (bleft != std::string::npos)
    {
        auto bright = source.find_first_of(BR, bleft + 1);
        if (bright != std::string::npos && bright > bleft)
        {
            tokens.push_back(BL);
            ++bleft;
            if (std::isspace(source[bleft]))
                bleft = source.find_first_not_of(s_delims, bleft);

            if (source[bleft] != CBR)
            {
                for (;;)
                {
                    auto last = source.find_first_of(s_delims, bleft);
                    if (last == std::string::npos)
                    {
                        if (bright > bleft)
                        {
                            tokens.push_back
                            (
                                source.substr(bleft, bright - bleft)
                            );
                        }
                        break;
                    }
                    else
                    {
                        tokens.push_back(source.substr(bleft, last - bleft));
                        bleft = source.find_first_not_of(s_delims, last);
                    }
                }
            }
            tokens.push_back(BR);
        }
    }
    return int(tokens.size());
}

/**
 *  Tokenizes a string containing a pair of numbers separated by either spaces
 *  or an 'x'.  Useful in grabbing dimensions.  Handles integers or basic
 *  floats.  It assumes only a single delimiter between each token:
 *
 *      -   "1.0x2.0"
 *      -   "1.0 2.0"
 *
 *  No matter what the delimiter, spaces are trimmed from each token.
 *
 * \param source
 *      Provides the string to be parsed into tokens.
 *
 * \param delimiters
 *      The character(s) separating the tokens. Defaults to a Space character.
 *
 * \return
 *      Returns the number of tokens converted in a string vector.
 */

tokenization
tokenize
(
    const std::string & source,
    const std::string & delimiters
)
{
    tokenization result;
    std::size_t previous = source.find_first_not_of(delimiters);
    while (previous != std::string::npos)
    {
        std::size_t current = source.find_first_of(delimiters, previous);
        if (current == std::string::npos)
        {
            std::string temp = trim(source.substr(previous));
            result.push_back(temp);
            break;
        }
        else
        {
            std::string temp = trim(source.substr(previous, current-previous));
            result.push_back(temp);
            previous = source.find_first_not_of(delimiters, current);
        }
    }
    return result;
}

/**
 *  This function makes values in quotes into a single token, and it uses the
 *  space and tab to delimit tokens.  Otherwise it is like tokenize() above.
 *  It handles only double quotes, which should match.  We don't want to watch
 *  out for apostrophes.  The quotes are stripped.
 */

tokenization
tokenize_quoted (const std::string & source)
{
    tokenization result;
    tokenization temp = tokenize(source);
    if (! temp.empty())
    {
        bool quotes = false;
        std::string quoted;
        for (const auto & token : temp)
        {
            if (token.front() == '"')
            {
                if (token.back() == '"')            /* single-word quote    */
                {
                    if (token.size() > 1)
                    {
                        quoted = token.substr(1, token.length() - 2);
                        if (! quoted.empty())
                            result.push_back(quoted);
                    }
                    else                            /* isolated end quote   */
                        result.push_back(quoted);
                }
                else
                {
                    quotes = true;
                    quoted = token.substr(1);
                }
            }
            else
            {
                if (token.back() == '"')            /* end of quoted string */
                {
                    if (quotes)
                    {
                        quotes = false;
                        quoted += " ";
                        quoted += token.substr(0, token.length() - 1);
                        if (! quoted.empty())
                        {
                            result.push_back(quoted);
                            quoted.clear();         /* in case more quotes  */
                            quotes = false;
                        }
                    }
                }
                else
                {
                    if (quotes)                     /* append the token     */
                    {
                        quoted += " ";
                        quoted += token;
                    }
                    else
                        result.push_back(token);
                }
            }
        }
    }
    return result;
}

/**
 *  Simplifies a string by tokenizing it based on spaces, and dropping tokens
 *  that have some special characters and don't start with a letter, then
 *  reassembling the remaining tokens with spaces in between.
 */

std::string
simplify (const std::string & source)
{
    std::string result;
    tokenization tokens = tokenize(source);
    if (tokens.empty())
    {
        result = source;
    }
    else
    {
        static std::string s_special = "[:]()";
        bool first_one = false;
        for (const auto & t : tokens)
        {
            bool ok = std::isalpha(t[0]);
            if (! ok)
                ok = t.find_first_of(s_special) == std::string::npos;

            if (ok)
            {
                if (first_one)
                    result += " ";

                result += t;
                first_one = true;
            }
        }
    }
    return result;
}

/**
 *
 *  This is a simplistic string-conversion function; it requires that
 *  the source string be ASCII-encoded.
 *
 * Windows:
 *
 *  -#  Handle the trivial case of an empty string.
 *  -#  Determine the required length of the new string.
 *  -#  Construct a new string of required length.
 *  -#  Convert the old string to the new string.
 *
 * Linux:
 *
 *  Just do a simple assign.
 *
 */

std::wstring
widen_string (const std::string & source)
{
    if (source.empty())
        return std::wstring();          /* trivial case of empty string     */

#if defined SEQ66_PLATFORM_WINDOWS
    size_t required_length = ::MultiByteToWideChar
    (
        CP_UTF8, 0, source.c_str(), int(source.length()), 0, 0
    );
    std::wstring result(required_length, L'\0');
    ::MultiByteToWideChar
    (
        CP_UTF8, 0, source.c_str(), int(source.length()), &result[0], int(result.length())
    );
    return result;
#else
    std::wstring result;
    result.assign(source.begin(), source.end());
    return result;
#endif
}

/**
 *  Takes a string and adds line breaks to make it fit full words within
 *  a margin. Any existing line breaks are treated like spaces.
 *
 *  This function can optionally add a comment character at the beginning
 *  of each line, followed by a space.
 *
 *  As with most of our string functions, this one isn't built for speed or
 *  space-saving.
 *
 * Method:
 *
 *  -#  Tokenize the string using SEQ66_WHITE_CHARS = " \t\r\n\v\f" as
 *      the delimiters.
 *
 * \param source
 *      Provides the string to be wrapped.
 *
 * \param margin
 *      Provides the wrap margin, beyond which no character will appear.
 *      Words that hit that limit are belayed to the next line. The default
 *      margin is 80.
 *
 * \param commentchar
 *      If non-zero (which is the default), then this character is prepended
 *      to each line.  A common value is "#".
 *
 * \return
 *      Returns the word-wrapped string.
 */

std::string
word_wrap (const std::string & source, size_t margin, char commentchar)
{
    std::string result;
    if (! source.empty())
    {
        std::string commenting{"  "};
        size_t linelen = 0;
        tokenization words = tokenize(source, SEQ66_WHITE_CHARS);
        commenting[0] = commentchar;
        for (auto w : words)
        {
            bool room = (linelen + w.length()) < margin;
            if (linelen == 0 || ! room)
            {
                if (commentchar != 0)
                    w = commenting + w;

                if (! room)
                    result += "\n";

                result += w;
                linelen = w.length();
            }
            else
            {
                w = " " + w;
                result += w;
                linelen += w.length();
            }
        }
        if (linelen > 0)
            result += "\n";
    }
    return result;
}

/**
 *  This function is meant for --help. It allows the display of long
 *  help text for an option by splitting the description line and indenting
 *  any subsequent lines that are necessary.
 *
 *  As an example, consider this line from another application:
 *
 *  -U, --jack-session uuid  Set UUID for JACK session; turns on session
 *                           management. Use 'on' to enable it and let JACK
 *                           set the UUID.
 *
 * \param source
 *      Provides a string defining a command-line option (for example). In
 *      the example above, this is the text at the right.
 *
 * \param leftmargin
 *      Provides the margin for lines after the first (if necessary).
 *      This provides the hanging indent.
 *
 * \param rightmargin
 *      Provides the maximum length of the lines that are generated.
 *
 * \return
 *      Returns the reformatted string.
 */

std::string
hanging_word_wrap
(
    const std::string & source,
    size_t leftmargin,
    size_t rightmargin
)
{
    std::string result;
    if (! source.empty())
    {
        int line = 0;                       /* the first line   */
        size_t linelen = leftmargin;
        std::string padding(leftmargin, ' ');
        tokenization words = tokenize(source, SEQ66_WHITE_CHARS);
        for (auto w : words)
        {
            bool room = (linelen + w.length()) < rightmargin;
            if (! room)
            {
                result += "\n";
                result += padding;
                linelen = leftmargin;
                ++line;
            }
            w = " " + w;
            result += w;
            linelen += w.length();
        }
    }
    return result;
}

}           // namespace seq66

/*
 * strfunctions.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

