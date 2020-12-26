/**
 * \file          strfunctions.cpp
 *
 *    Provides the implementations for safe replacements for the various C
 *    file functions.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-24
 * \updates       2020-12-22
 * \version       $Revision$
 *
 *    We basically include only the functions we need for Seq66, not
 *    much more than that.  These functions are adapted from our xpc_basic
 *    project.
 */

#include <cctype>                       /* std::toupper() function          */

#include "util/strfunctions.hpp"        /* free functions in seq66 n'space  */

/**
 *  A more readable way to depict the strings "" and "?".
 */

#define SEQ66_DOUBLE_QUOTES     "\"\""
#define SEQ66_QUESTION_MARK     "?"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provides a way to work with a visible empty string (e.g. in a
 *  configuration file).  This function returns true if the string really is
 *  empty, or just contains two double quotes ("").
 *
 *  See the add_quotes() and double_quotes() functions and the
 *  SEQ66_DOUBLE_QUOTES macro.
 */

bool
is_empty_string (const std::string & item)
{
    return item.empty() || item == SEQ66_DOUBLE_QUOTES;
}

/**
 *  Returns "" as a string for external callers.
 */

std::string
empty_string ()
{
    return std::string(SEQ66_DOUBLE_QUOTES);
}

std::string
questionable_string ()
{
    return std::string(SEQ66_QUESTION_MARK);
}

bool
is_questionable_string (const std::string & item)
{
    return item == SEQ66_QUESTION_MARK;
}


/**
 *  Indicates if one string can be found within another.  Doesn't force the
 *  called to use size_type.
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
    auto lpos = source.find_first_of(SEQ66_DOUBLE_QUOTES, pos);
    if (lpos != std::string::npos)
    {
        auto rpos = source.find_first_of(SEQ66_DOUBLE_QUOTES, lpos + 1);
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
    std::string result;
    if (item.empty())
    {
        result = SEQ66_DOUBLE_QUOTES;
    }
    else
    {
        result = item;
        auto pos0 = result.find_first_of("\"");
        auto pos1 = result.find_last_of("\"");
        bool change = pos0 != 0 && pos1 < result.length() - 1;
        if (change)
        {
            result = "\"";
            result += item;
            result += "\"";
        }
    }
    return result;
}

/**
 *  Returns double quotes as a string.
 */

const std::string &
double_quotes ()
{
    static std::string s_double_quotes = std::string(SEQ66_DOUBLE_QUOTES);
    return s_double_quotes;
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
 *  alternative, and analogous to strncmp().
 *
 * \param a
 *      Provides the "compared" string.
 *
 * \param b
 *      Provides the "comparing" string.
 *
 * \param n
 *      Provides the number of characters in the "compared" string that must
 *      match.  If equal to 0, then a regular operator ==() is used.
 *
 * \return
 *      Returns true if the strings compare identically for the first \a n
 *      characters.
 */

bool
strncompare (const std::string & a, const std::string & b, size_t n)
{
    bool result = ! a.empty() && ! b.empty();
    if (result)
        result = (n > 0) ? a.compare(0, n, b) == 0 : a == b ;

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

static bool
has_digit (const std::string & s)
{
    bool result = false;
    if (! s.empty())
    {
        for (const auto c : s)
        {
            if (std::isdigit(c))
            {
                result = true;
                break;
            }
        }
    }
    return result;
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
    return s.empty() ?
        defalt :
        (
            s == "1"   || s == "true" || s == "TRUE" ||
            s == "yes" || s == "YES" ||
            s == "on"  || s == "ON"
        );
}

/**
 *  Converts a string to a double value.  This function bypasses
 *  characters until it finds a digit (whether part of the number or a "0x"
 *  construct), and then converts it.  The strtol() function is used with a
 *  base of 0 so that decimal, hexadecimal, and octal values can all be
 *  parsed.
 *
 *  This function is the base implementation for string_to_midibyte() and
 *  string_to_int() as well.
 *
 * \param s
 *      Provides the string to convert to a signed long integer.
 *
 * \param defalt
 *      The desired default for an empty string.  The default \a defalt value is
 *      0.0.
 *
 * \return
 *      Returns the signed long integer value represented by the string.
 *      If the string is empty or has no digits, then 0.0 is returned.
 */

double
string_to_double (const std::string & s, double defalt)
{
    return has_digit(s) ? std::stod(s, nullptr) : defalt ;
}

/**
 *  Converts a string to a signed long value.  This function bypasses
 *  characters until it finds a digit (whether part of the number or a "0x"
 *  construct), and then converts it.  The strtol() function is used with a
 *  base of 0 so that decimal, hexadecimal, and octal values can all be
 *  parsed.
 *
 *  This function is the base implementation for string_to_midibyte() and
 *  string_to_int() as well.
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
    return has_digit(s) ? std::stol(s, nullptr, 0) : defalt ;
}

/**
 *  Converts a string to an unsigned long integer.
 */

unsigned long
string_to_unsigned_long (const std::string & s, unsigned long defalt)
{
    return has_digit(s) ? std::stoul(s, nullptr, 0) : defalt ;
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

/**
 *  Converts a string to a MIDI byte.  Simply calls string_to_long().
 *
 * \param s
 *      Provides the string to convert to a MIDI byte.
 *
 * \return
 *      Returns the MIDI byte value represented by the string.
 */

midibyte
string_to_midibyte (const std::string & s, midibyte defalt)
{
    return midibyte(string_to_long(s, long(defalt)));   /* tricky */
}

/**
 *  Shortens a file-specification to make sure it is no longer than the
 *  provided length value.  This is done by removing character in the middle,
 *  if necessary, and replacing them with an ellipse.
 *
 *  This function operates by first trying to find the <code> /home </code>
 *  directory.  If found, it strips off <code> /home/username </code> and
 *  replace it with the Linux <code> ~ </code> replacement for the <code>
 *  $HOME </code> environment variable.  This function assumes that the
 *  "username" portion <i> must </i> exist, and that there's no goofy stuff
 *  like double-slashes in the path.
 *
 * \param fpath
 *      The file specification, including the full path to the file, and the
 *      name of the file.
 *
 * \param leng
 *      Provides the length to which to limit the string.
 *
 * \return
 *      Returns the fpath parameter, possibly shortened to fit within the
 *      desired length.
 */

std::string
shorten_file_spec (const std::string & fpath, int leng)
{
    std::size_t fpathsize = fpath.size();
    if (fpathsize <= std::size_t(leng))
    {
        return fpath;
    }
    else
    {
        std::string ellipse("...");
        std::size_t halflength = (std::size_t(leng) - ellipse.size()) / 2 - 1;
        std::string result = fpath.substr(0, halflength);
        std::string lastpart = fpath.substr(fpathsize-halflength-1);
        result = result + ellipse + lastpart;
        return result;
    }
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
 *      result is false if it is empty..
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
    std::vector<std::string> & tokens,
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
    tokens.clear();                                         /* set size = 0 */
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
 *  floats.
 *
 * \return
 *      Returns the number of tokens converted: 0, 1, or 2.
 */

std::vector<std::string>
tokenize
(
    const std::string & source,
    const std::string delimiter     // = " "
)
{
    std::vector<std::string> result;
    std::size_t previous = 0;
    std::size_t current = source.find(delimiter);
    while (current != std::string::npos)
    {
        result.push_back(source.substr(previous, current - previous));
        previous = current + 1;
        current = source.find(delimiter, previous);
    }
    result.push_back(source.substr(previous, current - previous));
    return result;
}


/**
 * \param bitbucket
 *      The vector of bit values to be written.  Currently, this function
 *      assumes that the number of bit values is perfectly divisible by 8.
 *      If the user makes a mistake, tough shitsky.
 *
 * \param newstyle
 *      If true (the default), then hexadecimal values are written, in groups
 *      of 8 bits.  Hexadecimal values are better when set-size is greater than
 *      the legacy value, 32.
 *
 * \return
 *      Returns the assembled string, of the form "[ bits ]".
 */

std::string
write_stanza_bits
(
    const midibooleans & bitbucket,
    bool newstyle
)
{
    std::string result("[ ");
    int bitcount = int(bitbucket.size());
    if (bitcount > 0)
    {
        if (newstyle)
        {
            int bitcount = 8;                       /* group by 8 bits      */
            unsigned hexvalue = 0x00;
            for (auto b : bitbucket)
            {
                unsigned bitvalue = b != 0 ? 1 : 0 ;
                hexvalue |= bitvalue;
                --bitcount;
                if (bitcount == 0)
                {
                    char temp[16];
                    (void) snprintf(temp, sizeof temp, "0x%02x ", hexvalue);
                    result += temp;
                    bitcount = 8;
                    hexvalue = 0x00;
                }
                else
                    hexvalue <<= 1;
            }

            /*
             * Less than 8 bits encountered, emit the number anyway, after
             * undoing the last left-shift.
             */

            if (bitcount > 0 && bitcount < 8)
            {
                char temp[16];
                (void) snprintf(temp, sizeof temp, "0x%02x ", hexvalue >> 1);
                result += temp;
            }
        }
        else
        {
            int counter = 0;
            for (auto b : bitbucket)
            {
                result += (b != 0) ? "1" : "0" ;
                result += " ";
                if (++counter % 8 == 0 && counter < int(bitbucket.size()))
                    result += "] [ ";
            }
        }
    }
    result += "]";
    return result;
}

/**
 *  Adds the 8 bits of an unsigned value to a vector of midibools.
 *
 */

void
push_8_bits (midibooleans & target, unsigned bits)
{
    unsigned bitmask = 0x80;            /* start with the highest (MSB) bit */
    for (int i = 0; i < 8; ++i)
    {
        midibool mb = (bits & bitmask) != 0 ? midibool(1) : midibool(0) ;
        target.push_back(mb);
        bitmask >>= 1;
    }
}

/**
 *  We want to support both the legacy mute-group settings, with 4x8
 *  groups of "bits", and a newer setting, using an unsigned char (8 bits)
 *  to hold the bits.  The number of bits is based on the row and column
 *  settings for [mute-group].
 *
\verbatim
  --- loop 0                                                   loop 31 ---
 |                                                                        |
 v                                                                        v
[0 0 0 0 0 0 0 0 ] [ 1 1 1 1 1 1 1 1 ] [ 0 1 0 1 0 1 0 1 ] [1 0 1 0 1 0 1 0 ]
[      0x00      ] [       0xFF      ] [       0x55      ] [      0xAA      ]
[ 0x00 0xFF 0x55 0xAA ]
\endverbatim
 *
 *  NB: we need another mute-group flag to indicate how the groups will be
 *  written, in case some people don't want to deal with bit-masks.
 *
 *  The styles cannot be mixed; a single 'x' character on the line indicates
 *  the new format, and is scanned for before processing the line..
 *
 *  As with the legacy, the new style will support at least 8 bits per
 *  grouping. The groupings are purely organizational.  The bits are set in
 *  order from loop 0 on up, with no gaps or 2-D organization.
 *
 */

bool
parse_stanza_bits
(
    midibooleans & target,
    const std::string & mutestanza
)
{
    bool result = ! mutestanza.empty();
    if (result)
    {
        midibooleans bitbucket;
        auto p = mutestanza.find_first_of("xX");
        auto bleft = mutestanza.find_first_of("[");
        bool newstyle = p != std::string::npos;
        std::vector<std::string> tokens;
        int tokencount = tokenize_stanzas(tokens, mutestanza, bleft);
        result = tokencount > 0;
        if (result)
        {
            for (int tk = 0; tk < tokencount; ++tk)
            {
                std::string temp = tokens[tk];
                if (temp == "[" || temp == "]")
                {
                    /* nothing to do */
                }
                else
                {
                    unsigned v = unsigned(string_to_int(temp));
                    if (newstyle)
                    {
                        if (v < 256)
                            push_8_bits(bitbucket, v);
                        else
                            push_8_bits(bitbucket, 0);  /* error */
                    }
                    else
                    {
                        if (v != 0)
                            v = 1;

                        bitbucket.push_back(midibool(v));
                    }
                }
            }
            bleft = mutestanza.find_first_of("[", bleft + 1);
            result = bitbucket.size() > 0;
            if (result)
                target = bitbucket;
            else
                target.clear();
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

