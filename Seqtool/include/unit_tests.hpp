#if ! defined UNIT_TESTS_HPP
#define UNIT_TESTS_HPP

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
 * \file          unit_tests.hpp
 * \library       Seqtool (from the Seq66 project)
 * \author        Chris Ahlstrom
 * \date          2018-11-11
 * \updates       2018-11-14
 * \version       $Revision$
 * \license       $XPC_SUITE_GPL_LICENSE$
 *
 *    This application provides a few unit tests of the libseq66
 *    library module.
 */

#include "seq66-config.h"

#if defined SEQ66_SEQTOOL_TESTING_SUPPORT

#include <xpc/cut.hpp>                  /* xpc::cut unit-test class         */

/*
 *  Free functions for testing.
 */

extern int unit_tests (int argc, char * argv []);

#endif          // SEQ66_SEQTOOL_TESTING_SUPPORT

#endif          // UNIT_TESTS_HPP

/*
 * unit_tests.hpp
 *
 * vim: ts=4 sw=4 et ft=cpp
 */

