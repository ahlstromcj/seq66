/*
 *  This file is part of seq66.
 *
 *  seq66 is free software; you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option) any later
 *  version.
 *
 *  seq66 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with seq66; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          configfile.cpp
 *
 *  This module defines the base class for configuration and options
 *  files.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-23
 * \updates       2023-10-28
 * \license       GNU GPLv2 or above
 *
 *  std::streamoff is a signed integral type (usually long long) that can
 *  represent the maximum possible file size supported by the operating
 *  system.  It is used to represent offsets from std::streampos. -1 is used
 *  to represent error conditions by some I/O library functions.
 *
 *  Relationship with std::streampos (a specialization of std::fpos):
 *
 *  -   The difference between two std::streampos objects is a value of type
 *      std::streamoff.
 *  -   An std::streamoff value may be added or subtracted from std::fpos,
 *      yielding a different std::fpos.
 *  -   An std::streampos is implicitly convertible to std::streamoff (the
 *      result is the offset from the beginning of the file).
 *  -   An std::streampos is constructible from an std::streamoff.
 *  -   An std::streampos contains an std::streamoff.
 *
 *  istream::tellg() returns a streampos.
 */

#include <cctype>                       /* std::isspace(), std::isdigit()   */
#include <iomanip>                      /* std::hex, std::setw()            */

#include "cfg/configfile.hpp"           /* seq66:: configfile class         */
#include "cfg/rcsettings.hpp"           /* seq66::rcsettings class          */
#include "util/filefunctions.hpp"       /* seq66::filename_base() etc.      */
#include "util/strfunctions.hpp"        /* strncompare() for std::string    */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 *  Static members.  This error-messaging information is static so that the
 *  errors from all the configuration files can be displayed at once.
 */

std::string configfile::sm_error_message;
bool configfile::sm_is_error        = false;
int configfile::sm_int_missing      = -9998;
int configfile::sm_int_default      = -9999;
float configfile::sm_float_missing  = -9998.0f;
float configfile::sm_float_default  = -9999.0f;

tokenization configfile::sm_file_extensions
{
    ".ctrl",
    ".drums",
    ".mutes",
    ".palette",
    ".playlist",
    ".qss",
    ".rc",
    ".usr"
};

/**
 *  Provides the string plus rcsettings constructor for a configuration file.
 *
 * \param name
 *      The name of the configuration file.
 *
 * \param rcs
 *      A reference to the rcsettings object to hold the settings of the
 *      configuration file.  It applies to all configuration files, including
 *      usrfile.
 */

configfile::configfile
(
    const std::string & name,
    rcsettings & rcs,
    const std::string & fileext
) :
    m_rc                (rcs),
    m_file_extension    (fileext),
    m_name              (name),
    m_version           ("0"),
    m_file_version      ("0"),
    m_line              (),
    m_line_number       (0),
    m_line_pos          (0)
{
    // no code needed
}

/**
 *  Returns a pre-trimmed line from the configuration file.  As part of this
 *  trimming, double quotes (not single quotes) at the beginning and end are
 *  also removed.  The check is not robust at this time.
 *
 * \return
 *      Returns a copy of line(), but trimmed of white space and, if present,
 *      quotes surrounding the line after the space trimming.
 */

std::string
configfile::trimline () const
{
    std::string result = line();
    result = trim(result);

    auto bpos = result.find_first_of("\"");
    if (bpos != std::string::npos)
    {
        auto epos = result.find_last_of("\"");
        int len;
        if (epos != std::string::npos)
            len = int(epos - bpos - 1);
        else
            len = int(result.length() - 1 - bpos);

        result = result.substr(bpos + 1, len);
    }
    return result;
}

/**
 * [comments]
 *
 * Header commentary is skipped during parsing.  However, we provide an
 * optional comment block.  Trimming of spaces is disabled for this operation.
 */

std::string
configfile::parse_comments (std::ifstream & file)
{
    std::string result;
    if (line_after(file, "[comments]", 0, false))   /* 1st line w/out strip */
    {
        do
        {
            result += line();
            result += "\n";

        } while (next_data_line(file, false));      /* no strip here either */
    }
    return result;
}

std::string
configfile::parse_version (std::ifstream & file)
{
    std::string result = get_variable(file, "[Seq66]", "version");
    file_version(result);
    return result;
}

bool
configfile::file_version_old (std::ifstream & file)
{
    std::string file_version_string = parse_version(file);
    int file_version = string_to_int(file_version_string);
    int code_version = version_number();
    return file_version < code_version;
}

/**
 *  Helper function for error-handling.  It assembles a message and then
 *  passes it to error_message().
 *
 * \param sectionname
 *      Provides the name of the section for reporting the error.
 *
 * \param additional
 *      Additional context information to help in finding the error.
 *
 * \return
 *      Always returns false.
 */

bool
configfile::make_error_message
(
    const std::string & sectionname,
    const std::string & additional
)
{
    std::string msg = sectionname;
    msg += ": ";
    if (! additional.empty())
        msg += additional;

    error_message(msg);                 /* log to the console       */
    append_error_message(msg);          /* append to message string */
    return false;
}

bool
configfile::version_error_message (const std::string & configtype, int vnumber)
{
    std::string msg = "'";
    msg += configtype;
    msg += "' file version ";
    msg += std::to_string(vnumber);
    msg += " is too old. Please upgrade.\n";
    return make_error_message("Version error", msg);
}

/**
 *  A useful intermediate function to save a call and allow for debugging.  In
 *  addition, it also trims basic white-space from the beginning and end of
 *  the line, to make parsing a little more robust.
 *
 *  Also replaces file.get_line(m_line, sizeof m_line) with an std::string
 *  get_line().
 *
 * \param file
 *      The reference to the opened input file-stream.
 *
 * \param strip
 *      If true, then strip out any following comment in the line, as denoted
 *      by a hash-tag character not enclosed in single- or double-quotes.
 *      The default value is false.
 *
 * \return
 *      Returns the value of file.good().  If the trimmed line is empty,
 *      returns true, too; the caller can ignore the line.
 */

bool
configfile::get_line (std::ifstream & file, bool strip)
{
    m_line_pos = file.tellg();
    (void) std::getline(file, m_line);
    if (strip)
    {
        m_line = trim(m_line);
        m_line = strip_comments(m_line);
    }

    bool result = file.good();
    if (result)
        ++m_line_number;

    return result;
}

/**
 *  Gets the next line of data from an input stream.  If the line starts with
 *  a number-sign, or a null, it is skipped, to try the next line.  This
 *  occurs until a section marker ("[") or an EOF is encountered.  Member
 *  m_line is a "return" value (side-effect).
 *
 * \param file
 *      Points to an input stream.  We converted this item to a reference;
 *      pointers can be subject to problems.  For example, what if someone
 *      passes a null pointer?  Nonetheless, this is a small library.  Since
 *      this function has this parameter, the caller can deal with multiple
 *      files at the same time.
 *
 * \param strip
 *      If true (the default), trims white space and strips out hash-tag
 *      comments.  Some sections, such as '[comments]', need this to be set to
 *      false.
 *
 * \return
 *      Returns true if a presumed data line was found.  False is returned if
 *      not found before an EOF or a section marker ("[") is found.  This
 *      feature assists in adding new data to the file without crapping out on
 *      old-style configuration files.
 */

bool
configfile::next_data_line (std::ifstream & file, bool strip)
{
    bool result = get_line(file, strip);        /* optional white zappage   */
    if (result)
    {
        char ch = m_line[0];
        while ((ch == '#' || ch == '[' || ch == 0) && ! file.eof())
        {
            if (m_line[0] == '[')               /* we hit the next section  */
            {
                result = false;
                break;
            }
            if (get_line(file, strip))          /* may trim white space     */
            {
                ch = m_line[0];
            }
            else
            {
                result = false;
                break;
            }
        }
        if (file.eof())
            result = false;
    }
    return result;
}

/**
 *  Acts like a combination of line_after() and next_data_line(), but requires
 *  a specific variable-name to be found.  For example, given the following
 *  lines (blank lines are simply ignored):
 *
\verbatim
        [loop-control]
        liveplay = true
        bpm = 120
        name = "Funky"
\endverbatim
 *
 *  <code>std::string s = next_variable(f, "[loop-control]", "name");</code>
 *  will return <code>s = "Funky"</code>.
 *
 *  See the extract_value() function for information on the parsing of the
 *  line.
 *
 * \warning
 *      This function assumes that the next_data_line() function
 *
 * \param file
 *      Provides the file-stream that is being read.
 *
 * \param tag
 *      Provides the section tag (e.g. "[midi-control]") to be found before
 *      the variable name parameter can be processed.
 *
 * \param variablename
 *      Provides the variablename to be found in the specified tag section.
 *
 * \param position
 *      Indicates the position to seek to, which defaults to 0
 *      (std::iso::beg).  A non-default value is useful to speed up parsing in
 *      cases where sections are always ordered.
 *
 * \return
 *      If the "variablename = value" clause is found, then the the value that
 *      is read is returned.  Otherwise, an empty string is returned, which
 *      might be an error, or signify a default value.  If the name is
 *      surrounded by single or double quotes, these are trimmed.
 */

std::string
configfile::get_variable
(
    std::ifstream & file,
    const std::string & tag,
    const std::string & variablename,
    int position
)
{
    std::string result;
    for
    (
        bool done = ! line_after(file, tag, position);
        ! done; done = ! next_data_line(file)
    )
    {
        if (! line().empty())                   /* any value in section?    */
        {
            std::string value = extract_variable(line(), variablename);
            if (! is_questionable_string(value))
            {
                result = value;
                break;
            }
        }
    }
    return result;
}

/**
 *  Parses a line of the form "name = value".  Now exposed for use outside of
 *  the get_variable() function.  This function assumes that the line has been
 *  found.
 *
 *  The spaces around the '=' are optional.  This function is meant to better
 *  support an "INI" style of value specification.  The variable name should
 *  be obvious, whereas in the Seq24 implementation we had to append a comment
 *  to make it easier for the user to comprehend the configuration file.
 *
 *  Check-point 1:
 *
 *      See if there is any space after the variable name, but before the "="
 *      sign.  If so, the first space, not the "=" sign, terminates the
 *      variable name.
 *
 *  Check-point 2:
 *
 *      Now get the first non-space after the "=" sign.  If there is a
 *      double-quote character ('"'), then see if there a match quote, and get
 *      everything inside the quotes.  Otherwise, grab the first token on the
 *      value (right) side.  Note that single-quotes are not treated as quote
 *      characters.
 *
 * \param line
 *      A line of the form "name = value".
 *
 * \param variablename
 *      The "name" of the variable whose value is to be extracted.
 *
 * \return
 *      If the extracted name matches the \a name parameter, then the value is
 *      returned; it might be empty.  Otherwise, a question mark is returned.
 */

std::string
configfile::extract_variable
(
    const std::string & line,
    const std::string & variablename
)
{
    std::string result = questionable_string();     /* a fancy name for "?" */
    auto epos = line.find_first_of("=");
    if (epos != std::string::npos)
    {
        auto spos = line.find_first_of(" ");        /* Check-point 1        */
        if (spos > epos)
            spos = epos;

        std::string vname = line.substr(0, spos);
        if (vname == variablename)
        {
            bool havequotes = false;                /* Check-point 2        */
            char quotechar[2] = { 'x', 0 };
            auto qpos = line.find_first_of("\"", epos + 1);
            auto qpos2 = std::string::npos;
            if (qpos != std::string::npos)
            {
                quotechar[0] = line[qpos];
                qpos2 = line.find_first_of(quotechar, qpos + 1);
                if (qpos2 != std::string::npos)
                    havequotes = true;
            }
            if (havequotes)
            {
                result = line.substr(qpos + 1, qpos2 - qpos - 1);
            }
            else
            {
                spos = line.find_first_not_of(" ", epos + 1);
                if (spos != std::string::npos)
                {
                    epos = line.find_first_of(" ", spos);
                    result = line.substr(spos, epos-spos);
                }
            }
        }
    }
    return result;
}

bool
configfile::get_boolean
(
    std::ifstream & file,
    const std::string & tag,
    const std::string & variablename,
    int position,
    bool defalt
)
{
    std::string value = get_variable(file, tag, variablename, position);
    return string_to_bool(value, defalt);
}

void
configfile::write_seq66_header
(
    std::ofstream & file,
    const std::string & configtype,
    const std::string & ver
)
{
    file <<
        "\n[Seq66]\n\nconfig-type = \"" << configtype << "\"\n"
        "version = " << ver << "\n"
        ;
}

void
configfile::write_seq66_footer (std::ofstream & file)
{
    file
        << "\n# End of " << name() <<
        "\n#\n# vim: sw=4 ts=4 wm=4 et ft=dosini\n"
        ;
}

void
configfile::write_boolean
(
    std::ofstream & file,
    const std::string & name,
    bool status
)
{
    file << name << " = " << bool_to_string(status) << "\n";
}

int
configfile::get_integer
(
    std::ifstream & file,
    const std::string & tag,
    const std::string & variablename,
    int position
)
{
    std::string value = get_variable(file, tag, variablename, position);
    int result = sm_int_missing;
    if (! value.empty())
        result = value == "default" ? sm_int_default : string_to_int(value) ;

    return result;
}

void
configfile::write_integer
(
    std::ofstream & file,
    const std::string & name,
    int value,
    bool usehex
)
{
    if (usehex)
        file << name << " = 0x" << std::hex << std::setw(2) << value << "\n";
    else
        file << name << " = " << value << "\n";
}

float
configfile::get_float
(
    std::ifstream & file,
    const std::string & tag,
    const std::string & variablename,
    int position
)
{
    std::string value = get_variable(file, tag, variablename, position);
    float result = sm_float_missing;
    if (! value.empty())
        result = value == "default" ?
            sm_float_default : float(string_to_double(value)) ;

    return result;
}

void
configfile::write_float
(
    std::ofstream & file,
    const std::string & name,
    float value
)
{
    file << name << " = " << value << "\n";
}

/**
 *  Handles a number of write-string cases.  We make copies of the string
 *  value for internal use by this function.  Can optionally make sure the
 *  value is quoted.  Empty strings are always quoted.
 */

void
configfile::write_string
(
    std::ofstream & file,
    const std::string & name,
    std::string value,
    bool quote_it
)
{
    bool add_equals = true;
    if (is_empty_string(name))              /* standalone, no-name string   */
        add_equals = false;

    if (is_missing_string(value))
        quote_it = true;                    /* always quote empty strings   */

    if (quote_it)
        value = add_quotes(value);

    if (add_equals)
        file << name << " = " << value << "\n";
    else
        file << value << "\n";              /* a no-name value              */
}

/**
 *  Gets the active flag and the name of the file from the given tag section.
 *  Very useful for all "included" configuration files.
 *
 *  We now enforce that all configuration files are restricted to the HOME
 *  directory, so we also strip the path from the file-name.
 */

bool
configfile::get_file_status
(
    std::ifstream & file,
    const std::string & tag,
    std::string & filename,     /* side-effect */
    int position
)
{
    bool result = get_boolean(file, tag, "active", position);
    filename = strip_quotes(get_variable(file, tag, "name", position));
    if (filename.empty())
    {
        result = false;
    }
    else
    {
        if (name_has_path(filename))
            filename = filename_base(filename);
    }
    return result;
}

void
configfile::write_file_status
(
    std::ofstream & file,
    const std::string & tag,
    const std::string & filename,
    bool status
)
{
    std::string quoted = add_quotes(filename);
    file
        << "\n" << tag << "\n\n"
        << "active = " << bool_to_string(status) << "\n"
        << "name = " << quoted << "\n"
        ;
}

void
configfile::write_comment
(
    std::ofstream & file,
    const std::string & commenttext
)
{
    file << "\n"
"# [comments] holds user documentation for this file. The first empty, hash-\n"
"# commented, or tag line ends the comment.\n"
"\n[comments]\n\n" << commenttext
        ;
}

/**
 *  Looks for the next named section.  Unlike line_after(), it does not
 *  restart from the beginning of the file.  Like next_data_line(), it starts
 *  at the current line in the file.  This makes it useful in parsing files,
 *  such as a playlist, that have multiple sections with the same name.
 *
 *  Note one other quirk.  If we are on a line matching the tag, then we do
 *  not search, but instead use that line.  The reason is that the
 *  next_data_line() function for the previous section will often end up
 *  at the beginning of the next section.  Especially important with
 *  play-lists.
 *
 * \param file
 *      Points to the input file stream.  Since this function has this
 *      parameter, the caller can deal with multiple files at the same time.
 *      However, the caller must either close the file explicitly or open it
 *      on the stack.
 *
 * \param tag
 *      Provides a tag to be found.  Lines are read until a match occurs
 *      with this tag.  Normally, the tag is a section marker, such as
 *      "[user-interface]".  Best to assume an exact match is needed.
 *
 * \return
 *      Returns true if the tag was found.  Otherwise, false is returned.
 */

bool
configfile::next_section (std::ifstream & file, const std::string & tag)
{
    bool result = false;
    file.clear();
    if (tag == m_line)
    {
        result = true;
    }
    else
    {
        bool ok = get_line(file);       /* fills in m_line as a side-effect */
        while (ok)                      /* includes the EOF check           */
        {
            result = strncompare(m_line, tag);
            if (result)
            {
                break;
            }
            else
            {
                if (file.bad())
                    error_message("bad file stream reading config file");
                else
                    ok = get_line(file);        /* trims the white space   */
            }
        }
    }
    if (result)
        result = next_data_line(file);

    return result;
}

/**
 *  This function gets a specific line of text, specified as a tag.
 *  Then it gets the next non-blank line (i.e. data line) after that.
 *  This function is normally used to find major sections enclosed in brackets,
 *  such as "[midi-control]".
 *
 *  This function always starts from the beginning of the file.  Therefore,
 *  it can handle reading Seq66 configuration files that have had
 *  their tagged sections arranged in a different order.  This feature makes
 *  the configuration file a little more robust against errors.
 *
 * \param file
 *      Points to the input file stream.  Since this function has this
 *      parameter, the caller can deal with multiple files at the same time.
 *      However, the caller must either close the file explicitly or open it
 *      on the stack.
 *
 * \param tag
 *      Provides a tag to be found.  Lines are read until a match occurs
 *      with this tag.  Normally, the tag is a section marker, such as
 *      "[user-interface]".  Best to assume an exact match is needed.
 *
 * \param position
 *      Indicates the position to seek to, which defaults to 0
 *      (std::iso::beg).  A non-default value is useful to speed up parsing in
 *      cases where sections are always ordered.
 *
 * \param strip
 *      If true (the default), trims white space and strips out hash-tag
 *      comments, but only in lines after the tag is found.
 *
 * \return
 *      Returns true if the tag was found.  Otherwise, false is returned.
 */

bool
configfile::line_after
(
    std::ifstream & file,
    const std::string & tag,
    int position,
    bool strip
)
{
    bool result = false;
    file.clear();                               /* clear the file flags     */
    file.seekg(std::streampos(position), std::ios::beg); /* seek to spot    */
    m_line_number = 0;                          /* back to beginning        */

    bool ok = get_line(file, true);             /* trims spaces/comments    */
    while (ok)                                  /* includes the EOF check   */
    {
        result = strncompare(m_line, tag);
        if (result)
        {
            break;
        }
        else
        {
            if (file.bad())
                error_message("bad file stream reading config file");
            else
                ok = get_line(file);            /* trims the white space    */
        }
    }
    if (result)
        result = next_data_line(file, strip);   /* might preserve space etc */

    return result;
}

/**
 *  Like line_after, finds a tag, but merely marks the position preceding the
 *  tag.  The idea is to find a number of tags that might be ordered by number.
 *  Also useful when changes are made to tag names, to detect legacy names for
 *  section tags.
 *
 * \param file
 *      Points to the input file stream.
 *
 * \param tag
 *      Provides a tag to be found, which, for this function, is usually a
 *      partial tag, such as "[Drum".  Spaces are signficant!
 *
 * \return
 *      Returns the position of the line before the tag, converted to an
 *      integer.  If not found, -1 is returned.
 */

int
configfile::find_tag (std::ifstream & file, const std::string & tag)
{
    int result = (-1);
    file.clear();                               /* clear the file flags     */
    file.seekg(0, std::ios::beg);               /* seek to the beginning    */
    m_line_number = 0;                          /* back to beginning        */

    bool ok = get_line(file, true);             /* trims spaces/comments    */
    while (ok)                                  /* includes the EOF check   */
    {
        bool match = strncompare(m_line, tag);
        if (match)
        {
            result = line_position();           /* int(m_line_pos)          */
            break;
        }
        else
        {
            if (file.bad())
                error_message("bad file stream reading config file");
            else
                ok = get_line(file);            /* trims the white space    */
        }
    }
    return result;
}

/**
 *  Extracts an integer value from a tag like the following.  For this entry,
 *  the tag to use is "[Drum".
 *
 * \verbatim
 *      [Drum 33]
 * \endverbatim
 */

int
configfile::get_tag_value (const std::string & tag)
{
    int result = (-1);
    auto pos = tag.find_first_of("0123456789");
    if (pos != std::string::npos)
    {
        std::string buff = tag.substr(pos);     /* "35]" */
        result = string_to_int(buff);
    }
    else
    {
        std::string msg = tag;
        msg += " tag has no integer value";
        error_message(tag);
    }
    return result;
}

void
configfile::write_date (std::ofstream & file, const std::string & tag)
{
    file
        << "# Seq66 " << seq_version() << " " << tag << " configuration file\n"
        << "#\n# " << name() << "\n"
        << "# Written " << get_current_date_time() << "\n"
        << "#\n"
        ;
}

/**
 *  Sets the error message, which can later be displayed to the user.
 *  Actually, it now appends the error message, so all can be displayed in the
 *  user-interface.  We also avoid annoying duplicates.
 *
 * \param msg
 *      Provides the error message to be set.
 */

void
configfile::append_error_message (const std::string & msg)
{
    if (msg.empty())
    {
        sm_error_message.clear();
        sm_is_error = false;
    }
    else
    {
        sm_is_error = true;
        if (msg != sm_error_message)        /* avoid duplicate if possible  */
        {
            if (! sm_error_message.empty())
                sm_error_message += "\n";   /* converts to <br> in msg box  */

            sm_error_message += msg;
        }
    }
}

bool
configfile::set_up_ifstream (std::ifstream & instream)
{
    bool result = instream.is_open();
    if (result)
    {
        instream.seekg(0, std::ios::beg);                   /* seek to start */

        std::string s = get_variable(instream, "[Seq66]", "version");
        if (s.empty())
        {
            char temp[128];
            snprintf
            (
                temp, sizeof temp, "Version not found: %s\n", name().c_str()
            );
            result = make_error_message(file_extension(), temp);
        }
        else
        {
            /*
             * This test is kind of iffy, so disabled for now.
             *
             *      int version = string_to_int(s);
             *      if (version != rc_ref().ordinal_version())
             *      {
             *          warnprint("'rc' file version changed!");
             *      }
             */
        }
    }
    else
    {
        char temp[128];
        snprintf(temp, sizeof temp, "Read open fail: %s\n", name().c_str());
        result = make_error_message(file_extension(), temp);
    }
    return result;
}

/*
 *  Free functions.
 */

bool
delete_configuration (const std::string & path, const std::string & basename)
{
    bool result = ! path.empty() && ! basename.empty();
    if (result)
    {
        std::string base = filename_base(basename, true);   /* strip .ext */
        std::string msg = "Deleting " + base + " from";
        file_message(msg, path);
        for (const auto & ext : configfile::sm_file_extensions)
        {
            std::string fname = filename_concatenate(path, base);
            fname = file_extension_set(fname, ext);
            if (file_exists(fname))
                (void) file_delete(fname);
        }
    }
    return result;
}

bool
copy_configuration
(
    const std::string & source,         /* path */
    const std::string & basename,
    const std::string & destination     /* path */
)
{
    bool result = ! source.empty() &&
        ! basename.empty() && ! destination.empty();

    if (result)
    {
        std::string base = filename_base(basename, true);   /* strip .ext */
        std::string sourcename = filename_concatenate(source, base);
        std::string destinationname = filename_concatenate(destination, base);
        std::string msg = "Copying " + source + base + " to";
        file_message(msg, destination);
        for (const auto & ext : configfile::sm_file_extensions)
        {
            std::string srcname = file_extension_set(sourcename, ext);
            if (file_exists(srcname))
            {
                std::string destname = file_extension_set(destinationname, ext);
                bool ok = file_copy(srcname, destname);
                if (! ok)
                {
                    result = false;
                    break;
                }
            }
        }
    }
    return result;
}

std::string
get_current_date_time ()
{
    return current_date_time();
}

}           // namespace seq66

/*
 * configfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

