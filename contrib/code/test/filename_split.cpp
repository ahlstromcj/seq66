
#if defined SEQ66_PLATFORM_DEBUG

#include "util/filefunctions.hpp"

static const char * fmt = "%28s: %28s; %28s; %28s\n";

static void
split_test (const std::string & s, bool useext)
{
    std::string path, filebase, filebare, extension;
    std::string t = "'" + s + "'";
    bool haspath = useext ?
        seq66::filename_split_ext(s, path, filebare, extension) :
        seq66::filename_split(s, path, filebase) ;

    if (! haspath)
        path = "<no path>";
    else
        path = "'" + path + "'";

    if (filebase.empty())
        filebase = "<no base>";
    else
        filebase = "'" + filebase + "'";

    if (filebare.empty())
        filebare = "<no bare>";
    else
        filebare = "'" + filebare + "'";

    if (extension.empty())
        extension = "<no .ext>";
    else
        extension = "'" + extension + "'";

    if (useext)
    {
        printf
        (
            fmt, t.c_str(), path.c_str(), filebare.c_str(), extension.c_str()
        );
    }
    else
    {
        printf
        (
            fmt, t.c_str(), path.c_str(), filebase.c_str(), extension.c_str()
        );
    }
}

static void
filename_split_tests ()
{
    static std::vector<std::string> s_tests =
    {
        "",
        "aptitude",
        "aptitude.",
        "aptitude.exe",
        ".",
        ".filename",
        ".filename.",
        ".filename.extra",
        "relative/path/file",
        "relative/path/file.",
        "relative/path/file.extra",
        "relative/path/file/",
        "/absolute/path/file",
        "/absolute/path/file.",
        "/absolute/path/file.extra",
        "/absolute/path/file/",
        ".config/path/file",
        ".config/path/file.",
        ".config/path/file.extra",
        ".config/path/file/",
    };
    printf(fmt, "Full Path", "Path", "Base name", "Extension");
    for (const auto & s : s_tests)
        split_test(s, false);               /* no use of extension  */

    printf("\n");
    printf(fmt, "Full Path", "Path", "Bare name", "Extension");
    for (const auto & s : s_tests)
        split_test(s, true);                /* split with extension */
}

#endif  // defined SEQ66_PLATFORM_DEBUG
