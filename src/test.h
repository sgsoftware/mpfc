/******************************************************************
 * Copyright (C) 2003 - 2005 by SG Software.
 *
 * SG MPFC. Interface for testing facilities.
 * $Id$
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either version 2 
 * of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public 
 * License along with this program; if not, write to the Free 
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
 * MA 02111-1307, USA.
 */

#ifndef __SG_MPFC_TEST_H__
#define __SG_MPFC_TEST_H__

#include "types.h"

/* Tests */
enum
{
	TEST_NO_JOB = -1,
	TEST_WNDLIB_PERFOMANCE,
	TEST_NUMBER
};

/* Start test */
bool_t test_start( int id );

/* Stop currently running job */
void test_stop( void );

/* Start testing thread */
void *test_thread( void *arg );

/* Test the window library perfomance */
void test_wndlib_perfomance( void );

#endif

/* End of 'test.h' file */

