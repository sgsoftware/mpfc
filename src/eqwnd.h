/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : eqwnd.h
 * PURPOSE     : SG MPFC. Interface for equalizer window functions.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 5.08.2004
 * NOTE        : Module prefix 'eqwnd'.
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

#ifndef __SG_MPFC_EQUALIZER_H__
#define __SG_MPFC_EQUALIZER_H__

#include "types.h"
#include "wnd.h"

/* Number of bands (including preamp) */
#define EQWND_NUM_BANDS 11

/* Equalizer window type */
typedef struct 
{
	/* Window part */
	wnd_t m_wnd;

	/* Cursor position */
	int m_pos;
} eq_wnd_t;

/* Create a new equalizer window */
eq_wnd_t *eqwnd_new( wnd_t *parent );

/* Initialize equalizer window */
bool_t eqwnd_construct( eq_wnd_t *eq, wnd_t *parent );

/* Handle display message */
wnd_msg_retcode_t eqwnd_on_display( wnd_t *wnd );

/* Handle key message */
wnd_msg_retcode_t eqwnd_on_keydown( wnd_t *wnd, wnd_key_t key );

/* Handle mouse left button click */
wnd_msg_retcode_t eqwnd_on_mouse_ldown( wnd_t *wnd, int x, int y,
		wnd_mouse_button_t btn, wnd_mouse_event_t type );

/* Display slider */
int eqwnd_display_slider( wnd_t *wnd, int x, int start_y, int end_y,
							bool_t hl, float val, char *str );

/* Get equalizer variable name */
void eqwnd_get_var_name( int pos, char *name );

/* Set equalizer variable value */
void eqwnd_set_var( int pos, float val, bool_t rel );

/* Save equalizer parameters */
void eqwnd_save_params( void );

/* Launch load preset from EQF file dialog */
void eqwnd_load_eqf_dlg( void );

/* Handle 'ok_clicked' for EQF loading dialog */
wnd_msg_retcode_t eqwnd_on_load( wnd_t *wnd );

/* Load a Winamp EQF file */
void eqwnd_load_eqf( char *filename );

/* Get equalizer band slider position */
void eqwnd_get_slider_pos( int band, int *x, int *y, int *w, int *h );

/* Convert band value to position */
int eqwnd_val2pos( float value, int height );

/* Convert position to band value */
float eqwnd_pos2val( int pos, int height );

/* Show help screen */
void eqwnd_help( eq_wnd_t *eq );

#endif

/* End of 'eqwnd.h' file */

