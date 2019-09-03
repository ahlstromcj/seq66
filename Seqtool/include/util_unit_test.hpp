#if ! defined XPCCUTPP_UTIL_UNIT_TEST_HPP
#define XPCCUTPP_UTIL_UNIT_TEST_HPP

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
 * \file          util_unit_test.hpp
 * \library       Seqtool (from the Seq66 project)
 * \author        Chris Ahlstrom
 * \date          2018-12-22
 * \updates       2018-12-25
 * \version       $Revision$
 * \license       $XPC_SUITE_GPL_LICENSE$
 *
 *    This application provides unit tests for the strfunctions module of the
 *    seqtool application.
 */

#include "seq66-config.h"

#if defined SEQ66_SEQTOOL_TESTING_SUPPORT

#include <xpc/cut.hpp>                 /* xpc::cut unit-test class            */

extern xpc::cut_status util_unit_test_01_01 (const xpc::cut_options &);
extern xpc::cut_status util_unit_test_01_02 (const xpc::cut_options &);
extern xpc::cut_status util_unit_test_01_03 (const xpc::cut_options &);

#endif          // SEQ66_SEQTOOL_TESTING_SUPPORT

#endif          // XPCCUTPP_UTIL_UNIT_TEST_HPP

/*
 * util_unit_test.hpp
 *
 * vim: ts=4 sw=4 et ft=cpp
 */
