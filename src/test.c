/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : test.c
 * PURPOSE     : SG MPFC. Testing facilities implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 6.09.2004
 * NOTE        : Module prefix 'test'.
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

#include <pthread.h>
#include "types.h"
#include "player.h"
#include "test.h"
#include "wnd_root.h"

/* Test thread data */
pthread_t test_pid;
bool_t test_stop_job = FALSE;

/* The job currently being run */
int test_job = TEST_NO_JOB;

/* Start test */
bool_t test_start( int id )
{
	/* Start test thread */
	test_stop_job = FALSE;
	test_job = id;
	logger_debug(player_log, "Creating test thread");
	if (pthread_create(&test_pid, NULL, test_thread, NULL))
		return FALSE;
	return TRUE;
} /* End of 'test_start' function */

/* Stop currently running job */
void test_stop( void )
{
	if (test_job != TEST_NO_JOB)
	{
		test_stop_job = TRUE;
		pthread_join(test_pid, NULL);
		logger_debug(player_log, "Test thread terminated");
		test_job = TEST_NO_JOB;
	}
} /* End of 'test_stop' function */

/* Start testing thread */
void *test_thread( void *arg )
{
	/* Execute the test */
	switch (test_job)
	{
	case TEST_WNDLIB_PERFOMANCE:
		test_wndlib_perfomance();
		break;
	}
	test_job = TEST_NO_JOB;
	return NULL;
} /* End of 'test_thread' function */

/* Test the window library perfomance */
void test_wndlib_perfomance( void )
{
	int i, j;

	/* First execute info dialogs for each of the play list songs */
	for ( i = 0; i < player_plist->m_len && (!test_stop_job); i ++ )
	{
		wnd_msg_send(player_wnd, "user", 
				wnd_msg_user_new(PLAYER_MSG_INFO, (void *)i));
	}

	/* Now execute some cycles of focus changing */
	for ( i = 0; i < 10; i ++ )
	{
		for ( j = 0; j < (player_plist->m_len + 3) && (!test_stop_job); j ++ )
			wnd_msg_send(player_wnd, "user", 
					wnd_msg_user_new(PLAYER_MSG_NEXT_FOCUS, (void *)i));
	}
} /* End of 'test_wndlib_perfomance' function */

/* End of 'test.c' file */

