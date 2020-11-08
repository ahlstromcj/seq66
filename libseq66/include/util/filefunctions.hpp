#if ! defined SEQ66_FILEFUNCTIONS_HPP
#define SEQ66_FILEFUNCTIONS_HPP

/**
 * \file          filefunctions.hpp
 *
 *    Provides the declarations for safe replacements for some C++
 *    file functions.
 *
 * \author        Chris Ahlstrom
 * \date          2015-11-20
 * \updates       2020-11-06
 * \version       $Revision$
 *
 *    Also see the filefunctions.cpp module.  The functions here use
 *    old-school file-pointers and the new-fangled std::string.
 */

#include <string>

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 * Global function declarations.
 */

extern bool file_access (const std::string & targetfile, int mode);
extern bool file_exists (const std::string & targetfile);
extern bool file_readable (const std::string & targetfile);
extern bool file_writable (const std::string & targetfile);
extern bool file_accessible (const std::string & targetfile);
extern bool file_executable (const std::string & targetfile);
extern bool file_is_directory (const std::string & targetfile);
extern bool file_name_good (const std::string & filename);
extern bool file_mode_good (const std::string & mode);
extern FILE * file_open (const std::string & filename, const std::string & mode);
extern FILE * file_open_for_read (const std::string & filename);
extern FILE * file_create_for_write (const std::string & filename);
extern bool file_close (FILE * filehandle, const std::string & filename);
extern bool file_copy
(
    const std::string & file,
    const std::string & newfile
);
extern bool file_append_log
(
    const std::string & filename,
    const std::string & data
);
extern bool name_has_directory (const std::string & filename);
extern bool is_root_path (const std::string & path);
extern bool make_directory_path (const std::string & directory_name);
extern bool delete_directory (const std::string & filename);
extern bool set_current_directory (const std::string & path);
extern std::string get_current_directory ();
extern std::string get_full_path (const std::string & path);
extern char path_slash ();
extern std::string os_normalize_path
(
    const std::string & path,
    bool terminate = false
);
extern std::string normalize_path
(
    const std::string & path,
    bool tounix = true,
    bool terminate = false
);
extern std::string clean_file (const std::string & path, bool tounix = true);
extern std::string clean_path (const std::string & path, bool tounix = true);
extern std::string append_file
(
    const std::string & path,
    const std::string & filename,
    bool to_unix = true
);
extern std::string append_path
(
    const std::string & path,
    const std::string & pathname,
    bool to_unix = true
);
extern std::string filename_concatenate
(
    const std::string & path,
    const std::string & filebase
);
extern std::string pathname_concatenate
(
    const std::string & path0,
    const std::string & path1
);
extern bool filename_split
(
    const std::string & fullpath,
    std::string & path,
    std::string & filebase
);
extern std::string file_path_set
(
    const std::string & fullpath,
    const std::string & newpath
);
extern std::string filename_base
(
    const std::string & fullpath,
    bool noext = false
);
extern bool file_extension_match
(
    const std::string & path,
    const std::string & target
);
extern std::string file_extension (const std::string & path);
extern std::string file_extension_set
(
    const std::string & path,
    const std::string & ext
);
extern std::string executable_full_path ();
extern std::string user_home ();

#endif      // SEQ66_FILEFUNCTIONS_HPP

}           // namespace seq66

/*
 * filefunctions.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

