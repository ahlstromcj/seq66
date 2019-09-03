#if ! defined SEQ66_STRFUNCTIONS_HPP
#define SEQ66_STRFUNCTIONS_HPP

/**
 * \file          strfunctions.hpp
 *
 *    Provides the declarations for safe replacements for some C++
 *    file functions.
 *
 * \author        Chris Ahlstrom
 * \date          2018-11-23
 * \updates       2019-01-12
 * \version       $Revision$
 *
 *    Also see the strfunctions.cpp module.
 */

#include <string>
#include <vector>

#include "midi/midibytes.hpp"           /* seq66::midibool type             */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Lists of characters to trim from strings.
 */

const std::string SEQ66_TRIM_CHARS        = " \t\r\n\v\f";
const std::string SEQ66_TRIM_CHARS_QUOTES = " \t\r\n\v\f\"'";

/*
 * Global (free) string functions.
 */

extern std::string strip_comments (const std::string & item);
extern std::string strip_quotes (const std::string & item);
extern std::string add_quotes (const std::string & item);
extern bool strncompare (const std::string & a, const std::string & b, size_t n);
extern bool strcasecompare (const std::string & a, const std::string & b);
extern std::string & ltrim
(
    std::string & str, const std::string & chars = SEQ66_TRIM_CHARS
);
extern std::string & rtrim
(
    std::string & str, const std::string & chars = SEQ66_TRIM_CHARS
);
extern std::string & trim
(
    std::string & str, const std::string & chars = SEQ66_TRIM_CHARS
);
extern std::string string_replace
(
    const std::string & source,
    const std::string & target,
    const std::string & replacement
);
extern int tokenize_stanzas
(
    std::vector<std::string> & tokens,
    const std::string & source,
    std::string::size_type bleft = 0,
    const std::string & brackets = ""
);
extern std::string write_stanza_bits
(
    const midibooleans & bitbucket,
    bool newstyle = true
);
extern void push_8_bits (midibooleans & target, unsigned bits);
extern bool parse_stanza_bits
(
    midibooleans & target,
    const std::string & mutestanza
);

#endif      // SEQ66_STRFUNCTIONS_HPP

}           // namespace seq66

/*
 * strfunctions.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

