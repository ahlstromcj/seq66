/*
 *  This file is part of seq66.
 *
 *  seq66 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq66 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq66; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          util_unit_test.cpp
 * \library       Seqtool (from the Seq66 project)
 * \author        Chris Ahlstrom
 * \date          2018-11-11
 * \updates       2018-12-25
 * \version       $Revision$
 * \license       $XPC_SUITE_GPL_LICENSE$
 *
 *    This application provides unit tests for the midicontrol module of the
 *    libseq66 library.
 *
 * Unit Test Groups:
 *
 *       1.  seq66::strfunctions
 *           1.  tokenize_stanzas()
 *           2.  write_stanza_bits()
 *           3.  parse_stanza_bits()
 */

#include <iostream>
#include <iomanip>

#include "seq66-config.h"               /* SEQ66_SEQTOOL_TESTING_SUPPORT    */
#include "midi/midibytes.hpp"           /* seq66::midibool alias/typedef    */
#include "util/strfunctions.hpp"        /* seq66::strfunctions module       */

#if defined SEQ66_SEQTOOL_TESTING_SUPPORT

#include "util_unit_test.hpp"

/**
 *  A helper function to display a vector of strings.
 */

static void
show_string_vector (const std::vector<std::string> & tokens)
{
    int count = int(tokens.size());
    std::cout << std::setw(2) << count << " tokens: ";
    if (count > 0)
    {
        for (const auto & s : tokens)
            std::cout << s << " ";
    }
    else
        std::cout << "None";

    std::cout << std::endl;
}

/**
 *
 * \group 1. seq66::strfunction
 *
 * \case
 *    1. tokenize_stanzas()
 *
 * \note
 *    It all of these unit-tests, it is important to understand that a
 *    status coming up invalid [i.e. status.valid() == false] is \e not a
 *    test failure -- it only indicates that the status object is invalid \e
 *    or that the test is not allowed to run.
 *
 * \tests
 *    -  seq66::tokenize_stanzas() function
 *
 * \param options
 *    Provides the command-line options for the unit-test application.
 *
 * \return
 *    Returns the unit-test status object needed by the protocol.
 */

xpc::cut_status
util_unit_test_01_01 (const xpc::cut_options & options)
{
    xpc::cut_status status
    (
        options, 1, 1, "seq66::strfunctions", T_("tokenize_stanzas()")
    );
    bool ok = status.valid();                   /* invalidity not an error  */
    if (ok)
    {
        if (! status.can_proceed())             /* is test allowed to run?  */
        {
            status.pass();                      /* no, force it to pass     */
        }
        else
        {
            if (status.next_subtest("tokenize_stanzas(): Smoke Test"))
            {
                std::vector<std::string> tokens;
                std::string substanza("[ 1 0 1 0 1 0 1 0 ]");
                int count = seq66::tokenize_stanzas(tokens, substanza);
                if (options.is_verbose())
                    show_string_vector(tokens);

                ok = count == 10;
                if (ok && status.next_subtest("Binary tight"))
                {
                    substanza = "[1 0 1 0 1 0 1 0]";
                    count = seq66::tokenize_stanzas(tokens, substanza);
                    ok = count == 10;
                    if (options.is_verbose())
                        show_string_vector(tokens);
                }
                if (ok && status.next_subtest("Hex loose"))
                {
                    substanza = "[ 0x01 0xab 0xbc 0xcd ]";
                    count = seq66::tokenize_stanzas(tokens, substanza);
                    ok = count == 6;
                    if (options.is_verbose())
                        show_string_vector(tokens);
                }
                if (ok && status.next_subtest("Hex tight"))
                {
                    substanza = "[0x01 0xab 0xbc 0xcd]";
                    count = seq66::tokenize_stanzas(tokens, substanza);
                    ok = count == 6;
                    if (options.is_verbose())
                        show_string_vector(tokens);
                }
                if (ok && status.next_subtest("One value"))
                {
                    substanza = "[0x01]";
                    count = seq66::tokenize_stanzas(tokens, substanza);
                    ok = count == 3;
                    if (options.is_verbose())
                        show_string_vector(tokens);
                }
                if (ok && status.next_subtest("Empty substanza"))
                {
                    substanza = "[]";
                    count = seq66::tokenize_stanzas(tokens, substanza);
                    ok = count == 2;
                    if (options.is_verbose())
                        show_string_vector(tokens);
                }
                if (ok && status.next_subtest("Illegal substanza"))
                {
                    substanza = "] 1 0 1 0 1 0 1 0 [";
                    count = seq66::tokenize_stanzas(tokens, substanza);
                    ok = count == 0;
                    if (options.is_verbose())
                        show_string_vector(tokens);
                }
                if (ok && status.next_subtest("Alternate brackets"))
                {
                    substanza = "{ 1 0 1 0 1 0 1 0 }";
                    count = seq66::tokenize_stanzas(tokens, substanza, 0, "{}");
                    ok = count == 10;
                    if (options.is_verbose())
                        show_string_vector(tokens);
                }
                if (ok && status.next_subtest("Quotes"))
                {
                    substanza = "\" 1 0 1 0 1 0 1 0 \"";
                    count = seq66::tokenize_stanzas
                    (
                        tokens, substanza, 0, "\"\""
                    );
                    ok = count == 10;
                    if (options.is_verbose())
                        show_string_vector(tokens);
                }
                if (! ok)
                    errprint("You dumbass!");

                status.pass(ok);
            }
        }
    }
    return status;
}

/**
 *
 * \group 1. seq66::strfunction
 *
 * \case
 *    2. write_stanza_bits()
 *
 * \tests
 *    -  seq66::write_stanza_bits() function
 *
 * \param options
 *    Provides the command-line options for the unit-test application.
 *
 * \return
 *    Returns the unit-test status object needed by the protocol.
 */

xpc::cut_status
util_unit_test_01_02 (const xpc::cut_options & options)
{
    xpc::cut_status status
    (
        options, 1, 2, "seq66::strfunctions", T_("write_stanza_bits()")
    );
    bool ok = status.valid();                   /* invalidity not an error  */
    if (ok)
    {
        if (! status.can_proceed())             /* is test allowed to run?  */
        {
            status.pass();                      /* no, force it to pass     */
        }
        else
        {
            if (status.next_subtest("write_stanza_bits(): Smoke Test"))
            {
                std::vector<seq66::midibool> bits;
                bits.push_back(seq66::midibool(true));  // [ 1 0 1 0 1 0 1 0 ]
                bits.push_back(seq66::midibool(false));
                bits.push_back(seq66::midibool(true));
                bits.push_back(seq66::midibool(false));
                bits.push_back(seq66::midibool(true));
                bits.push_back(seq66::midibool(false));
                bits.push_back(seq66::midibool(true));
                bits.push_back(seq66::midibool(false));
                std::string b = seq66::write_stanza_bits(bits, false);
                if (options.is_verbose())
                    std::cout << " 8 Bin bits = " << b << std::endl;

                ok = b == "[ 1 0 1 0 1 0 1 0 ]";
                if (ok && status.next_subtest("hex smoke"))
                {
                    b = seq66::write_stanza_bits(bits, true);
                    if (options.is_verbose())
                        std::cout << " 8 Hex bits = " << b << std::endl;

                    ok = b == "[ 0xaa ]";
                }
                if (ok && status.next_subtest("16 bits"))
                {
                    bits.push_back(seq66::midibool(true));  // 1 0 1 0 1 0 1 0
                    bits.push_back(seq66::midibool(false));
                    bits.push_back(seq66::midibool(true));
                    bits.push_back(seq66::midibool(false));
                    bits.push_back(seq66::midibool(true));
                    bits.push_back(seq66::midibool(false));
                    bits.push_back(seq66::midibool(true));
                    bits.push_back(seq66::midibool(false));
                    b = seq66::write_stanza_bits(bits, false);
                    if (options.is_verbose())
                        std::cout << "16 Bin bits = " << b << std::endl;

                    ok = b == "[ 1 0 1 0 1 0 1 0 ] [ 1 0 1 0 1 0 1 0 ]";
                }
                if (ok && status.next_subtest("16 hex bits"))
                {
                    b = seq66::write_stanza_bits(bits, true);
                    if (options.is_verbose())
                        std::cout << "16 Hex bits = " << b << std::endl;

                    ok = b == "[ 0xaa 0xaa ]";
                }
                if (ok && status.next_subtest("32 bits"))
                {
                    bits.push_back(seq66::midibool(false));
                    bits.push_back(seq66::midibool(false));
                    bits.push_back(seq66::midibool(false));
                    bits.push_back(seq66::midibool(false));
                    bits.push_back(seq66::midibool(false));
                    bits.push_back(seq66::midibool(false));
                    bits.push_back(seq66::midibool(false));
                    bits.push_back(seq66::midibool(false));
                    bits.push_back(seq66::midibool(false));
                    bits.push_back(seq66::midibool(false));
                    bits.push_back(seq66::midibool(false));
                    bits.push_back(seq66::midibool(false));
                    bits.push_back(seq66::midibool(false));
                    bits.push_back(seq66::midibool(false));
                    bits.push_back(seq66::midibool(false));
                    bits.push_back(seq66::midibool(false));
                    b = seq66::write_stanza_bits(bits, false);
                    if (options.is_verbose())
                        std::cout << "32 Bin bits = " << b << std::endl;

                    ok = b == "[ 1 0 1 0 1 0 1 0 ] [ 1 0 1 0 1 0 1 0 ] "
                                "[ 0 0 0 0 0 0 0 0 ] [ 0 0 0 0 0 0 0 0 ]";
                }
                if (ok && status.next_subtest("32 hex bits"))
                {
                    b = seq66::write_stanza_bits(bits, true);
                    if (options.is_verbose())
                        std::cout << "32 Hex bits = " << b << std::endl;

                    ok = b == "[ 0xaa 0xaa 0x00 0x00 ]";
                }
            }
            if (ok && status.next_subtest("4 bits"))
            {
                std::vector<seq66::midibool> bits;
                bits.push_back(seq66::midibool(true));  // [ 1 0 1 0 ]
                bits.push_back(seq66::midibool(false));
                bits.push_back(seq66::midibool(true));
                bits.push_back(seq66::midibool(false));
                std::string b = seq66::write_stanza_bits(bits, false);
                if (options.is_verbose())
                    std::cout << " 4 Bin bits = " << b << std::endl;

                ok = b == "[ 1 0 1 0 ]";
                if (ok && status.next_subtest("4 hex bits"))
                {
                    b = seq66::write_stanza_bits(bits, true);
                    if (options.is_verbose())
                        std::cout << " 4 Hex bits = " << b << std::endl;

                    ok = b == "[ 0x0a ]";
                }
            }
            if (! ok)
                errprint("You dumbass!");

            status.pass(ok);
        }
    }
    return status;
}

/**
 *
 * \group 1. seq66::strfunction
 *
 * \case
 *    3. parse_stanza_bits()
 *
 * \tests
 *    -  seq66::parse_stanza_bits() function
 *
 * \param options
 *    Provides the command-line options for the unit-test application.
 *
 * \return
 *    Returns the unit-test status object needed by the protocol.
 */

xpc::cut_status
util_unit_test_01_03 (const xpc::cut_options & options)
{
    xpc::cut_status status
    (
        options, 1, 3, "seq66::strfunctions", T_("parse_stanza_bits()")
    );
    bool ok = status.valid();                   /* invalidity not an error  */
    if (ok)
    {
        if (! status.can_proceed())             /* is test allowed to run?  */
        {
            status.pass();                      /* no, force it to pass     */
        }
        else
        {
            if (status.next_subtest("parse_stanza_bits(): Smoke Test"))
            {
                std::vector<std::string> tokens;
                std::vector<seq66::midibool> bits;
                std::string substanza("[ 1 0 1 0 1 0 1 0 ]");
                std::string b;
                int count = seq66::tokenize_stanzas(tokens, substanza);
                ok = count == 10;
                if (ok)
                {
                    ok = seq66::parse_stanza_bits(bits, substanza);
                    if (ok)
                    {
                        b = seq66::write_stanza_bits(bits, false);
                        if (options.is_verbose())
                            std::cout << " 8 Bin bits = " << b << std::endl;

                        ok = b == "[ 1 0 1 0 1 0 1 0 ]";
                    }
                }
                if (ok && status.next_subtest("Binary tight"))
                {
                    substanza = "[1 0 1 0 1 0 1 0]";
                    count = seq66::tokenize_stanzas(tokens, substanza);
                    ok = count == 10;
                    if (ok)
                        ok = seq66::parse_stanza_bits(bits, substanza);

                    if (ok)
                    {
                        b = seq66::write_stanza_bits(bits, false);
                        if (options.is_verbose())
                            std::cout << " 8 Bin bits = " << b << std::endl;

                        ok = b == "[ 1 0 1 0 1 0 1 0 ]";
                    }
                }
                if (ok && status.next_subtest("Hex loose"))
                {
                    substanza = "[ 0x80 0xff 0x80 0xff ]";
                    count = seq66::tokenize_stanzas(tokens, substanza);
                    ok = count == 6;
                    if (ok)
                        ok = seq66::parse_stanza_bits(bits, substanza);

                    if (ok)
                    {
                        b = seq66::write_stanza_bits(bits, false);
                        if (options.is_verbose())
                            std::cout << " 8 Hex bits = " << b << std::endl;

                        ok = b == "[ 1 0 0 0 0 0 0 0 ] "
                                  "[ 1 1 1 1 1 1 1 1 ] "
                                  "[ 1 0 0 0 0 0 0 0 ] "
                                  "[ 1 1 1 1 1 1 1 1 ]";

                        if (ok)
                        {
                            b = seq66::write_stanza_bits(bits, true);
                            if (options.is_verbose())
                                std::cout << " 8 Hex bits = " << b << std::endl;

                            ok = b == "[ 0x80 0xff 0x80 0xff ]";
                        }
                    }
                }
                if (ok && status.next_subtest("Hex tight"))
                {
                    substanza = "[0x80 0xff 0x80 0xff]";
                    count = seq66::tokenize_stanzas(tokens, substanza);
                    ok = count == 6;
                    if (ok)
                        ok = seq66::parse_stanza_bits(bits, substanza);

                    if (ok)
                    {
                        b = seq66::write_stanza_bits(bits, false);
                        if (options.is_verbose())
                            std::cout << " 8 Hex bits = " << b << std::endl;

                        ok = b == "[ 1 0 0 0 0 0 0 0 ] "
                                  "[ 1 1 1 1 1 1 1 1 ] "
                                  "[ 1 0 0 0 0 0 0 0 ] "
                                  "[ 1 1 1 1 1 1 1 1 ]";
                    }
                }
                if (ok && status.next_subtest("One value"))
                {
                    substanza = "[0x01]";
                    ok = seq66::parse_stanza_bits(bits, substanza);
                    if (ok)
                    {
                        b = seq66::write_stanza_bits(bits, false);
                        if (options.is_verbose())
                            std::cout << " 8 Hex bits = " << b << std::endl;

                        ok = b == "[ 0 0 0 0 0 0 0 1 ]";
                        if (ok)
                        {
                            b = seq66::write_stanza_bits(bits, true);
                            ok = b == "[ 0x01 ]";
                        }
                    }
                }
                if (ok && status.next_subtest("Empty substanza loose"))
                {
                    substanza = "[ ]";
                    ok = ! seq66::parse_stanza_bits(bits, substanza);
                    if (ok)
                        ok = bits.size() == 0;
                }
                if (ok && status.next_subtest("Empty substanza tight"))
                {
                    substanza = "[]";
                    ok = ! seq66::parse_stanza_bits(bits, substanza);
                    if (ok)
                        ok = bits.size() == 0;
                }
                if (ok && status.next_subtest("Illegal substanza"))
                {
                    substanza = "] 1 0 1 0 1 0 1 0 [";
                    ok = ! seq66::parse_stanza_bits(bits, substanza);
                    if (ok)
                        ok = bits.size() == 0;
                }
                if (ok && status.next_subtest("16 bits in 2 substanzas"))
                {
                    substanza = "[ 1 0 1 0 1 0 1 0 ] [ 1 0 1 0 1 0 1 0 ]";

                    /*
                     * Unnecessary redundant testing.
                     *
                     * count = seq66::tokenize_stanzas(tokens, substanza);
                     * ok = count == 20;
                     * if (ok)
                     */

                    ok = seq66::parse_stanza_bits(bits, substanza);
                    if (ok)
                    {
                        b = seq66::write_stanza_bits(bits, false);
                        if (options.is_verbose())
                            std::cout << " 8 x 2 Bin bits = " << b << std::endl;

                        ok = b == "[ 1 0 1 0 1 0 1 0 ] [ 1 0 1 0 1 0 1 0 ]";
                    }
                }
            }
            if (! ok)
                errprint("You dumbass!");

            status.pass(ok);
        }
    }
    return status;
}

#endif      // SEQ66_SEQTOOL_TESTING_SUPPORT

/*
 * util_unit_test.cpp
 *
 * vim: ts=4 sw=4 et ft=cpp
 */
