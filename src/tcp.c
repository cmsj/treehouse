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

   tcp.c

   TCP/IP functions
   $Header: /cvsroot/treehouse/treehouse/src/tcp.c,v 1.4 2002/05/16 00:30:11 chrisjones Exp $
*/

/* Includes */
#include "header.h"

/*
   tcp_connect - Establishes a TCP/IP socket connection with the server using GLib IO Channels

  	INPUTS:
  			struct GIOChannel *io	-	GLib IO Channel
  			gchar *host	 	- 	hostname of the remote machine to connect to
  			gint port		- 	The port to use on the remote machine
  	RETURNS:
  			0	-	Success.
  			1	-	Failed, unable to resolve the hostname.
  			2	-	Failed, unable to connect to remote machine.
*/
gint tcp_connect (GIOChannel *io, gchar *host, gint port)
{
  struct sockaddr_in addr;
  struct hostent *he = gethostbyname (host);

  if (!he) /* We were unable to resolve the hostname */
    return (1);

  memset ((char *) &addr, 0, sizeof (struct sockaddr_in));
  addr.sin_family = AF_INET;
  memcpy (&(addr.sin_addr.s_addr), he->h_addr, he->h_length);
  addr.sin_port = htons (port);

  if (connect (g_io_channel_unix_get_fd (io), (struct sockaddr *) &addr, sizeof (struct sockaddr)) < 0)
    return (2);

  return (0);
}

/*
   tcp_read_write - Read or Write data to a provided socket descriptor
  
  	INPUTS:
			*buffer		-	Details of the buffer we should fill with
							 data obtained from the remote server
			*cmd		-	Text to send to the remote server
			sockfd		-	Socket descriptor to use
			flags		-	Specifies read/write only or read & write
  	RETURNS:
  			0	-	Success
  			1	-	Writing failed
  			2	-	Reading failed
*/
gint tcp_read_write (GIOChannel *io, buf_info *buffer, gchar *cmd, gint flags)
{
  guint bytes = 0, tmp = 0;
  gchar *pos;

  if (flags == TCP_BOTH || flags == TCP_WRITE) {
    /* Write data to the socket */
    do {
      g_io_channel_write (io, cmd, strlen (cmd), &tmp);
      bytes += tmp;
      if (bytes < 0) {
	g_print ("Error: Writing to tcp socket failed with error: '%d'\n", errno);
	buffer->buf[0] = 0;
	return (1);
      }
    }
    while ((bytes == 0) || (bytes < 0));
  }

  bytes = 0;

  if (flags == TCP_BOTH || flags == TCP_READ) {
    /* Read data from the socket */
    memset (buffer->buf, 0, buffer->size);

    if (!(strncmp (cmd, "RETR", 4))) {
      /* This is a RETR */
      while ((pos = strstr (buffer->buf, "\r\n.\r\n")) == NULL) {
	if (bytes == buffer->size) {
	  display_error (_("Error: buffer exceeded"));
	  return (2);
	}
        g_io_channel_read (io, buffer->buf + bytes, buffer->size - bytes, &tmp);
        bytes += tmp;
      }
    } else {
        g_io_channel_read (io, buffer->buf, buffer->size, &tmp);
        bytes += tmp;
    }

    if (buffer->buf[bytes - 1] != '\n')
      g_print (_("Treehouse Error: Mail does not appear to be complete (expected: '%d' received: '%d')\n"), buffer->size, bytes);

    if (bytes == -1) {
      g_print ("Error: Reading from tcp socket failed with error: '%d'\n", errno);
      buffer->buf[0] = 0;
      return (1);
    }

    buffer->buf[bytes] = 0;

  }

  return (0);
}
