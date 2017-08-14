/*
    Treehouse v0.1.0
    Copyright (C) 1999 Chris Jones

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

   html.c

   HTML conversion functions
   $Header: /cvsroot/treehouse/treehouse/src/html.c,v 1.3 2001/06/26 00:39:47 chrisjones Exp $
*/

/* Includes */
#include "header.h"

/*
   text_to_html - Convert ASCII text to HTML "on-the-fly"
  
  	INPUTS:
			*html_text		-	Destination for HTML source (must be malloc()'d to correct length first)
			*text			-	ASCII text to be converted
  	RETURNS:
*/
void text_to_html (gchar * html_text, gchar * text)
{
  gchar *src = text;
  gchar *dst = html_text;
  *html_text = 0;

  while (*src) {
    switch (*src) {
    case '<':
      if (g_strncasecmp (src, "<a href", 7)) {
        if (g_strncasecmp (src, "</a", 3)) {
	        strcpy (dst, "&lt;");
	        dst += 4;
	        break;
	      } else {
	        *dst++ = *src;
	        break;
	      }
      } else {
	      *dst++ = *src;
	      break;
      }
    break;
    case '&':
      strcpy (dst, "&amp;");
      dst += 5;
    break;
    default:
      *dst++ = *src;
    break;
    }
    src++;
  }
  strcpy (dst, "\0");
  return;
}

/*
   html_len - Determine the length some text will be after it has been HTML-ised
  
  	INPUTS:
			*html		-	Text to examine
  	RETURNS:
  			len		-	length of the HTML-ised text
*/
gint html_len (gchar * html)
{

  gint len = 0;
  gchar *tmp = html;

  while (*tmp) {
    switch (*tmp) {
    case '<':
      len += 4;
      tmp++;
      break;

    case '&':
      len += 5;
      tmp++;
      break;

    default:
      len++;
      tmp++;
      break;
    }
  }

  return (len);

}
