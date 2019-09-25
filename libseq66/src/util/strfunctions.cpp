/**
 * \file          strfunctions.cpp
 *
 *    Provides the implementations for safe replacements for the various C
 *    file functions.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-24
 * \updates       2019-01-12
 * \version       $Revision$
 *
 *    We basically include only the functions we need for Seq66, not
 *    much more than that.  These functions are adapted from our xpc_basic
 *    project.
 */

#include <cctype>                       /* std::toupper() function          */

#include "util/calculations.hpp"        /* seq66::string_to_int()           */
#include "util/strfunctions.hpp"        /* free functions in seq66 n'space  */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

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
    std::string::size_type hashpos = result.find_first_of("#");
    std::string::size_type qpos = result.find_first_of("\"'");
    if (qpos != std::string::npos)
    {
        std::string::size_type qpos2;
        char quotechar[2] = { 'x', 0 };
        quotechar[0] = result[qpos];
        qpos2 = result.find_first_of(quotechar, qpos + 1);
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
        std::string::size_type fpos = result.find_first_of("\"");
        if (fpos == 0)
        {
            std::string::size_type lpos = result.find_last_of("\"");
            std::string::size_type end_index = result.length() - 1;
            if (lpos != std::string::npos && lpos == end_index)
                result = result.substr(1, end_index - 1);
        }
        else
        {
            fpos = result.find_first_of("'");
            if (fpos == 0)
            {
                std::string::size_type lpos = result.find_last_of("'");
                std::string::size_type end_index = result.length() - 1;
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
        result = "\"\"";
    }
    else
    {
        result = item;
        std::string::size_type pos0 = result.find_first_of("\"");
        std::string::size_type pos1 = result.find_last_of("\"");
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
 *      Provides the number of characters that must match.  If equal to 0,
 *      then a regular operator ==() is used.
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
 *      The prospective string to be trimmed.
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
 *      The prospective string to be trimmed.
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
 *      The prospective string to be trimmed.
 *
 * \param chars
 *      The set of characters to be trimmed.
 *
 * \return
 *      Returns a reference to the string that was left-right-trimmed.
 */

std::string &
trim (std::string & str, const std::string & chars)
{
    return ltrim(rtrim(str, chars), chars);
}

/**
 *  Replaces the first occurrence of a sub-string with the specified string.
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
    const std::string & replacement
)
{
    std::string result = source;
    auto targetsize = target.size();            // std::string::size_type
    auto targetloc = result.find(target);       // std::string::size_type
    if (targetloc != std::string::npos)
        (void) result.replace(targetloc, targetsize, replacement);

    return result;
}

/**
 *  Tokenizes a substanza, defined as the text between square brackets,
 *  including the square brackets.
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
        std::string::size_type bright = source.find_first_of(BR, bleft + 1);
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
                    std::string::size_type last =
                        source.find_first_of(s_delims, bleft);

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
        std::string::size_type p = mutestanza.find_first_of("xX");
        std::string::size_type bleft = mutestanza.find_first_of("[");
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

