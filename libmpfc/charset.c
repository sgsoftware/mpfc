/******************************************************************
 * Copyright (C) 2004 by SG Software.
 ******************************************************************/

/* FILE NAME   : charset.c
 * PURPOSE     : SG MPFC. libcharset functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 31.01.2004
 * NOTE        : Module prefix 'cs'.
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

#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "charset.h"
#include "pmng.h"

/* Convert string between charsets */
char *cs_convert( char *str, char *cs_in, char *cs_out, pmng_t *pmng )
{
	cs_info_t in_info, out_info;
	int len, i;
	char *out_str;
	char in_name[80], out_name[80];

	/* Check input */
	if (str == NULL || pmng == NULL)
		return NULL;

	util_log("string is %s\n", str);

	/* Get charset names if they are not specified */
	if (cs_in == NULL)
		cs_in = cfg_get_var(pmng->m_cfg_list, "charset-in");
	if (cs_out == NULL)
		cs_out = cfg_get_var(pmng->m_cfg_list, "charset-out");
	util_log("input cs is %s, output is %s\n", cs_in, cs_out);

	/* Get information about specified charsets */
	cs_get_info(cs_in, &in_info, pmng);
	cs_get_info(cs_out, &out_info, pmng);

	/* Check that these charsets are correct */
	if (in_info.m_id == CS_UNKNOWN || out_info.m_id != CS_1_BYTE)
		return NULL;

	/* Get number of symbols in our string */
	len = cs_get_len(str, &in_info);
	util_log("string length is %d\n", len);

	/* Allocate memory for output string */
	out_str = (char *)malloc(len + 1);
	if (out_str == NULL)
		return NULL;

	/* Process input string */
	for ( i = 0; i < len; i ++ )
	{
		dword ch, unicode;
		
		/* Obtain next character */
		util_log("getting next symbol\n");
		ch = cs_get_next_ch(&str, &in_info);
		util_log("it's %d\n", ch);

		/* Convert it between charsets */
		unicode = cs_to_unicode(ch, &in_info);
		util_log("unicode is %d\n", unicode);
		out_str[i] = cs_from_unicode(unicode, &out_info);
		util_log("converted\n");
	}
	out_str[i] = 0;
	return out_str;
} /* End of 'cs_convert' function */

/* Get charset info */
void cs_get_info( char *name, cs_info_t *info, pmng_t *pmng )
{
	/* Fill info with start values */
	info->m_id = CS_UNKNOWN;
	info->m_csp = NULL;
	info->m_index = -1;
	
	/* Find charset among internal */
	if (!strcasecmp(name, "UTF-8"))
	{
		info->m_id = CS_UTF8;
		return;
	}
	/* Find charset in plugins */
	else
	{
		info->m_csp = pmng_find_charset(pmng, name, &info->m_index);
		if (info->m_csp != NULL)
			info->m_id = CS_1_BYTE;
	}
} /* End of 'cs_get_info' function */

/* Get string length (in symbols) */
int cs_get_len( char *str, cs_info_t *info )
{
	int len;

	/* For 1-byte charset string length in symbols is equal to the one
	 * in bytes */
	if (info->m_id == CS_1_BYTE)
		return strlen(str);
	
	/* Parse string */
	for ( len = 0;; len ++ )
	{
		int bytes;
		
		switch (info->m_id)
		{
		case CS_UTF8:
			bytes = cs_utf8_get_num_bytes(*str);
			if (bytes < 0)
				return 0;
			str += bytes;
			break;			
		}
	}
	return len;
} /* End of 'cs_get_len' function */

/* Get next character from string */
dword cs_get_next_ch( char **str, cs_info_t *info )
{
	/* In one-byte charset move 1 byte further */
	if (info->m_id == CS_1_BYTE)
	{
		char ch = **str;
		(*str) ++;
		return (dword)ch;
	}

	/* UTF-8 */
	if (info->m_id == CS_UTF8)
	{
		byte first = (byte)(**str);
		int bytes = cs_utf8_get_num_bytes(first), bits;
		dword ch;
		int shift = 6 * (bytes - 1);
		 
		/* Check errors */
		if (bytes < 0)
			return 0;

		/* Get lower bits of first bytes (containing data) */
		bits = (bytes == 1) ? bytes : bytes + 1;
		ch = (first & (0xff >> bits)) << shift;
		shift -= (8 - bits);
		(*str) ++;
		while (shift > 0)
		{
			ch |= (((byte)(**str) & 0x40) << shift);
			shift -= 6;
		}
		return ch;
	}
} /* End of 'cs_get_next_ch' function */

/* Convert character code to unicode */
dword cs_to_unicode( dword ch, cs_info_t *info )
{
	if (info->m_id == CS_UNKNOWN)
		return 0;
	else if (info->m_id != CS_1_BYTE)
		return ch;
	else
	{
		/* Get from plugin */
		return csp_get_code(info->m_csp, ch, info->m_index);
	}
	return 0;
} /* End of 'cs_to_unicode' function */

/* Convert unicode to character code */
dword cs_from_unicode( dword unicode, cs_info_t *info )
{
	if (unicode < 128)
		return unicode;
	 
	if (info->m_id == CS_UNKNOWN)
		return 0;
	else if (info->m_id != CS_1_BYTE)
		return unicode;
	else
	{
		int ch;

		/* Find character with specified code */
		for ( ch = 128; ch < 256; ch ++ )
		{
			if (csp_get_code(info->m_csp, ch, info->m_index) == unicode)
				return ch;
		}
	}
	return ' ';
} /* End of 'cs_from_unicode' function */

/* Get number of bytes occupied by symbol in UTF-8 charset (looking
 * at its first byte) */
int cs_utf8_get_num_bytes( byte first )
{
	if (!(first & 0x80))
		return 1;
	else if (!(first & 0x40))
		return -1;
	else if (!(first & 0x20))
		return 2;
	else if (!(first & 0x10))
		return 3;
	else if (!(first & 0x08))
		return 4;
	else if (!(first & 0x04))
		return 5;
	else if (!(first & 0x02))
		return 6;
	else
		return -1;
} /* End of 'cs_utf8_get_num_bytes' function */

/* End of 'charset.c' file */

