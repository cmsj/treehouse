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

    This program uses some code from pop3check by steven radack.
    (permission was obtained for this as it is not GPL'd)
    Some code taken from or inspired by Spruce (which is GPL'd)

    fileio.c

    Functions for accessing files
    $Header: /cvsroot/treehouse/treehouse/src/fileio.c,v 1.5 2002/05/16 00:30:11 chrisjones Exp $
*/

/* Includes */
#include "header.h"

/* open_real - The real file opening function

        INPUTS:
  			*name	-	name of the file (usually an mId)
  			type	-	type of file ('b' for body, 'h' for header)
                        mode    -       enum of OPEN_FILE_READ or OPEN_FILE_WRITE
  	RETURNS:
  			-1	-	Unable to get env var $HOME. Probably a broken install, fuck 'em ;)
  			-2	-	Unable to open file
				-	Any other return value is a file descriptor
*/ 
gint open_real (gchar *name, gchar *type, int mode)
{
  gint fp, flags;
  gchar *cmd = NULL;

  switch (mode) {
    case OPEN_FILE_READ:
      flags = O_RDONLY;
    break;
    case OPEN_FILE_WRITE:
      flags = O_CREAT|O_WRONLY;
    break;
  }
  if (getenv ("HOME")) {
    cmd = g_strconcat (getenv ("HOME"), "/.treehouse/mail/", name, type, NULL);
printf ("open_real'ing file: '%s'\n", cmd);
    fp = open (cmd, flags, S_IRWXU);
    if (fp == -1) {
      perror ("open_real");
      g_free (cmd);
      return (-2);
    }
  } else {
    return (-1);
  }

  return (fp);
}

/*
   open_file - Opens a mail header/body file for reading and returns a file descriptor to it
  
  	INPUTS:
  			*name	-	name of the file (usually an mId)
  			type	-	type of file ('b' for body, 'h' for header)
  	RETURNS:
  			-1	-	Unable to get env var $HOME. Probably a broken install, fuck 'em ;)
  			-2	-	Unable to open file
				-	Any other return value is a file descriptor
*/
gint open_file (gchar *name, gchar *type)
{
  return (open_real (name, type, OPEN_FILE_READ));
}

/*
   open_filew - Opens a mail header/body file for writing and returns a file descriptor to it
  
  	INPUTS:
  			*name	-	name of the file (usually an mId)
  			type	-	type of file ('b' for body, 'h' for header)
  	RETURNS:
  			-1	-	Unable to get env var $HOME. Probably a broken install, fuck 'em ;)
  			-2	-	Unable to open file
				-	Any other return value is a file descriptor
*/
gint open_filew (gchar *name, gchar *type)
{
  return (open_real (name, type, OPEN_FILE_WRITE));
}


/*
   read_file - Read a complete file into a buffer and return it
  
  	INPUTS:
  			fp	-	file descriptor of the file to read
  	RETURNS:
  			text of the file
*/
gchar *read_file (gint fp)
{
  struct stat *file_inf;
  gint bytes = 0;
  gchar *txt;

  file_inf = g_malloc (sizeof (*file_inf) + 1);
  fstat (fp, file_inf);
  txt = g_malloc ((int)file_inf->st_size + 1);

  while (bytes != -1) {
    bytes += read (fp, txt + bytes, file_inf->st_size - bytes);
    if (bytes == file_inf->st_size)
      break;
  }
  if (bytes != -1) {
    txt[file_inf->st_size] = '\0';
  }
  g_free (file_inf);

  return (txt);
}

/*
   write_file - Write a complete file from a buffer
  
  	INPUTS:
  			fp	-	file descriptor of the file to read
			text-	text buffer to write
  	RETURNS:
  			0	-	Success
			1	-	Failure
*/
gint write_file (gint fp, char *text)
{
  gint bytes = 0;

  while (bytes != -1) {
    bytes += write (fp, text + bytes, strlen (text + bytes));
    if (bytes == strlen (text))
      break;
  }

  return (0);
}
