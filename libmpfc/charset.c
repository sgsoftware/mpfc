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

/* Output string memory portion size */
#define CS_PORTION_SIZE 64

/* Convert string between charsets */
char *cs_convert( char *str, char *cs_in, char *cs_out, pmng_t *pmng )
{
	cs_info_t in_info, out_info;
	cs_output_t out_str;

	/* Check input */
	if (str == NULL || pmng == NULL || cs_in == NULL || cs_out == NULL)
		return NULL;

	/* Get information about specified charsets */
	cs_get_info(cs_in, &in_info, pmng);
	cs_get_info(cs_out, &out_info, pmng);

	/* Check that these charsets are correct */
	if (in_info.m_id == CS_UNKNOWN || out_info.m_id == CS_UNKNOWN)
		return NULL;

	/* Expand input charset if it is automatic */
	if (csp_get_flags(in_info.m_csp, in_info.m_index) & CSP_AUTO)
	{
		cs_in = csp_expand_auto(in_info.m_csp, str, in_info.m_index);
		if (cs_in == NULL)
			return NULL;
		cs_get_info(cs_in, &in_info, pmng);
		if (in_info.m_id == CS_UNKNOWN)
			return NULL;
	}

	/* Initialize output string */
	out_str.m_str = NULL;
	out_str.m_allocated = out_str.m_len = 0;

	/* Process input string */
	for ( ;; )
	{
		dword ch, unicode;
		
		/* Obtain next character */
		ch = cs_get_next_ch(&str, &in_info);

		/* Convert it between charsets */
		unicode = cs_to_unicode(ch, &in_info);
		cs_unicode2str(&out_str, unicode, &out_info);
		if (!ch)
			break;
	}
	out_str.m_str = (char *)realloc(out_str.m_str, out_str.m_len);
	return out_str.m_str;
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
	else if (info->m_id == CS_UTF8)
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
		ch = ((byte)first & (0xFF >> bits)) << shift;
		shift -= (8 - bits);
		(*str) ++;
		while (shift > 0)
		{
			ch |= (((byte)(**str) & 0x3F));
			shift -= 6;
			(*str) ++;
		}
		return ch;
	}
	return 0;
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

/* Add a unicode character to output string */
void cs_unicode2str( cs_output_t *str, dword unicode, cs_info_t *info )
{
	dword ch;
	
	if (str == NULL || info == NULL)
		return;

	/* Convert from unicode */
	ch = cs_from_unicode(unicode, info);

	/* Case of 1-byte charset */
	if (info->m_id == CS_1_BYTE)
	{
		cs_append2out(str, (char)ch);
	}
	/* UTF-8 */
	else if (info->m_id == CS_UTF8)
	{
		int bytes, bits = 32;
		dword code = unicode;

		/* Get number of significant bits */
		while (!(code & 0x80000000) && (bits > 0))
		{
			bits --;
			code <<= 1;
		}
		if (bits <= 7)
		{
			cs_append2out(str, (char)unicode);
		}
		else
		{
			byte utf_code[6];
			int first_byte = 5;
			
			/* Convert to UTF bytes */
			code = unicode;
			for ( ;; )
			{
				if (bits > 6)
				{
					utf_code[first_byte] = (0x80 | (code & 0x3F));
					bits -= 6;
					first_byte --;
					code >>= 6;
				}
				else
				{
					utf_code[first_byte] = ((0xFF << (bits + 1)) | code);
					break;
				}
			}

			/* Write to output */
			for ( ; first_byte < 6; first_byte ++ )
				cs_append2out(str, (char)(utf_code[first_byte]));
		}
	}
} /* End of 'cs_unicode2str' function */

/* Reallocate memory for output string */
void cs_reallocate( cs_output_t *str, int new_len )
{
	int new_allocated;
	
	if (str == NULL)
		return;

	new_allocated = new_len;
	while (new_allocated % CS_PORTION_SIZE)
		new_allocated ++;
	str->m_str = (char *)realloc(str->m_str, new_allocated);
} /* End of 'cs_reallocate' function */

/* Append character to output string */
void cs_append2out( cs_output_t *str, char ch )
{
	if (str == NULL)
		return;

	cs_reallocate(str, str->m_len + 1);
	str->m_str[str->m_len ++] = ch;
} /* End of 'cs_append2out' function */

/* Get number of bytes occupied by symbol in UTF-8 charset (looking
 * at its first byte) */
int cs_utf8_get_num_bytes( byte first )
{
	if (!(first & 0x80))
		return 1;
	else if (!(first & 0x40))
		return 0;
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
		return 0;
} /* End of 'cs_utf8_get_num_bytes' function */

/* End of 'charset.c' file */

