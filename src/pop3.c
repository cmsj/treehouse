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

   pop3.c

   Handles all the communication with the POP3 server.
   $Header: /cvsroot/treehouse/treehouse/src/pop3.c,v 1.5 2002/05/21 06:16:56 chrisjones Exp $
*/

/* Include our header file */
#include "header.h"

/*
   getnword - Used to tokenise a string to extract specific words within that string
  
  	INPUTS:
  			gchar *s			-	The string to search for
  			gint n			-	The number of the word required
  			gchar *word		-	String to store the word in
  	RETURNS:
  			1	-	Success
  			0	-	Failed, word not found.
*/
gint getnword (gchar * s, gint n, gchar * word)
{
  gchar *token;
  token = strtok (s, " ");
  n--;
  while (n) {
    token = strtok (NULL, " ");
    n--;
  }
  if (token) {
    strcpy (word, token);
    return (1);
  } else {
    return (0);
  }
}

/*
   get_mail - Download any new mail waiting for us on the mail server given.
  
  	INPUTS:
  			gchar *mailserver	-	hostname of the mail server we are to use
  			gchar *username		-	username we should use to log onto the server with
  			gchar *password		-	password we should use with the username above
  			gint port		-	The port to use on the mail server (usually 110)
  			gint delete		-	Do we need to delete mail from the server
			gchar *account		-	Name of the account we are checking
  	RETURNS:
  			0	-	Success.
  			1	-	Failed, unable to resolve the mail server's hostname (from "pop_connect")
  			2	-	Failed, unable to connect to the mail server (from "pop_connect")
  			3	-	Failed, invalid username or password (could mean mailbox was locked)
  			4	-	Unknown server error.
  			5	-	Unable to decode mail headers.
  			6	-	Unable to create a socket.
  			7	-	Success (but no mails downloaded)
			8	-	General error
*/
gint get_mail (gchar * mailserver, gchar * username, gchar * password, gint port, gint delete, gchar * account)
{
  gint result = 0, i = 0, merrno = 0, sockfd, nummsg = 0, mbsize = 0;
  gchar cmd2[1024];
  gchar *cmd;
  gchar buf2[1024];
  gchar token[1024];
  gchar *token2;
  gchar *mail = NULL;
  gchar *header = NULL;
  gchar *body = NULL;
  gchar *folder = "Inbox";
  gchar *txt;
  gchar *tmp;
  float i_f, nummsg_f;

  GIOChannel *io;

  struct add_mail *amail;
  buf_info *buffer;
  buf_info *mail_buf;

  MYSQL mysql;

  if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
    if (sockfd)
      close (sockfd);
    return (6);
  }

  io = g_io_channel_unix_new (sockfd);

  result = tcp_connect (io, mailserver, port);
  if (result > 0) {
    if (sockfd)
      close (sockfd);
    return (result);
  }

  mysql = sql_connect ("treehouse");

  if (mysql_errno (&mysql)) {
    display_error (_("Unable to establish database connection"));
    close (sockfd);
    return (8);
  }

  while (gtk_events_pending ())
    gtk_main_iteration ();

  buffer = g_malloc (sizeof (buf_info));
  buffer->buf = g_malloc (buffer->size = MAX_SERVER_BUF);

  tcp_read_write (io, buffer, "", TCP_READ);

  cmd = g_strconcat ("USER ", username, "\r\n", NULL);
  tcp_read_write (io, buffer, cmd, TCP_BOTH);
  g_free (cmd);

  if (buffer->buf[0] == '-') {
    display_error (_("POP3 server rejected your username"));
    g_free (buffer->buf);
    g_free (buffer);
    close (sockfd);
    return (3);
  }

  cmd = g_strconcat ("PASS ", password, "\r\n", NULL);
  tcp_read_write (io, buffer, cmd, TCP_BOTH);
  g_free (cmd);

  if (buffer->buf[0] == '-') {
    display_error (_("POP3 server rejected your password"));
    g_free (buffer->buf);
    g_free (buffer);
    close (sockfd);
    return (3);
  }

  cmd = g_strconcat ("STAT\r\n", NULL);
  tcp_read_write (io, buffer, cmd, TCP_BOTH);
  g_free (cmd);

  if (buffer->buf[0] == '-') {
    display_error (_("POP3 server rejected the 'STAT' command\nThis is highly unusual"));
    g_free (buffer->buf);
    g_free (buffer);
    close (sockfd);
    return (4);
  }

  /* Take a copy of the server's reply so we can use strtok() on the original reply without breaking it */
  strcpy (buf2, buffer->buf);

  getnword (buffer->buf, 3, token);
  mbsize = atoi (token);

  getnword (buf2, 2, token);
  nummsg = atoi (token);

  if (nummsg == 0) {
    /* There are no new messages, so we jump straight to dpop3 */
    goto dpop3;
  }

  /* Loop through the messages and insert them into the mail database */
  for (i = 0; i < nummsg; i++) {
    i_f = i + 1;
    nummsg_f = nummsg;

    gnome_appbar_set_progress (GNOME_APPBAR (w->StatusBar), (i_f / nummsg_f));
    txt = g_strdup_printf (" Status: Checking for new mail on account: '%s'... checking message %d of %d", account, i + 1, nummsg);
    gnome_appbar_push (GNOME_APPBAR (w->StatusBar), txt);
    g_free (txt);
    while (gtk_events_pending ())
      gtk_main_iteration ();

    /* Get the server to tell us the size of the current message */
    g_snprintf (cmd2, sizeof (cmd2), "LIST %d\r\n", i + 1);
    tcp_read_write (io, buffer, cmd2, TCP_BOTH);

    token2 = g_malloc (strlen (buffer->buf));

    getnword (buffer->buf, 3, token2);

    amail = g_malloc (sizeof (struct add_mail) + 1);

    /* Store the uidl of the mail */
    g_snprintf (cmd2, sizeof (cmd2), "UIDL %d\r\n", i + 1);
    tcp_read_write (io, buffer, cmd2, TCP_BOTH);
    getnword (buffer->buf, 2, token);

printf ("Preparing UIDL: '%s'\n", token);
    amail->uidl = g_malloc (strlen (token));
    strncpy (amail->uidl, token, strlen (token));
    amail->uidl[strlen (token) - 2] = '\0';
printf ("Stored UIDL: '%s'\n", amail->uidl);
    /* Check to see if we already have this mail */
    if (!delete) {
      /* FIXME: This should pay attention to the account in use too, but that will require more function parameters */
      cmd = g_strconcat ("INSERT INTO mail (mId) VALUES ('", amail->uidl, "')", NULL);
      merrno = 0;
      mysql_query (&mysql, cmd);
      g_free (cmd);
      merrno = mysql_errno (&mysql);
      if (merrno == ER_DUP_ENTRY) {
	gnome_appbar_pop (GNOME_APPBAR (w->StatusBar));
	while (gtk_events_pending ())
	  gtk_main_iteration ();
	continue;
      } else {
	/* Fake a ROLLBACK since MySQL doesn't support it */
	cmd = g_strconcat ("DELETE FROM mail WHERE mId = '", amail->uidl, "'", NULL);
	mysql_query (&mysql, cmd);
	g_free (cmd);
      }
    }

    mail_buf = g_malloc (sizeof (*mail_buf));
    mail_buf->buf = g_malloc (atoi (token2) + 100);
    mail_buf->size = atoi (token2) + 99;

    g_snprintf (cmd2, sizeof (cmd2), "RETR %d\r\n", i + 1);
    tcp_read_write (io, mail_buf, cmd2, TCP_BOTH);
    mail_buf->buf[strlen (mail_buf->buf) - 4] = '\0';

    g_free (token2);

    /* Test the server's response */
    /* If it is a '-' something has failed (this shouldnt happen, but we handle it just in case) */
    if (mail_buf->buf[0] == '-') {
      display_error (_("POP3 Server rejected the 'RETR' command\nPlease consult your system administrator"));
      g_free (amail->uidl);
      g_free (amail);
      g_free (buffer->buf);
      g_free (buffer);
      g_free (mail_buf->buf);
      g_free (mail_buf);
      close (sockfd);
      mysql_close (&mysql);
      return (4);
    }

    /*
       Find the first point in the string of the word "Received" (which is assumed to be the start of the first header),
       store the start of the headers, body and then seperate the two with a '\0'
       Some mail servers seem to put '\r\n\r\n' elsewhere in the headers and this confuses a lot of mail clients
       I consider this behaviour to be inconsistant with the RFCs related to the format of emails.
    */
    /* First advance past the server's reply message */
    mail = strstr (mail_buf->buf, "\r\n") + 2;
    header = mail;
    body = strstr (mail, "\r\n\r\n");
    if (body == NULL) {
      display_error (_("Error: Unable to locate end of headers"));
      g_free (amail->uidl);
      g_free (amail);
      g_free (buffer->buf);
      g_free (buffer);
      g_free (mail_buf->buf);
      g_free (mail_buf);
      close (sockfd);
      mysql_close (&mysql);
      return (5);
    }
    /* Advance past the "\r\n\r\n" */
    body += 4;
    header[strlen (mail) - strlen (body) - 1] = '\0';

    /* Pick out important data from the Header. We need From:, To: Subject: and Date: */
    result = 0;

    result = extract_header (&amail->from, header, "\r\nFrom:");
    if (result)
      amail->from = g_strdup (" ");

    result = extract_header (&amail->to, header, "\r\nTo:");
    if (result)
      amail->to = g_strdup (" ");

    result = extract_header (&amail->subject, header, "\r\nSubject:");
    if (result)
      amail->subject = g_strdup (" ");

    result = extract_header (&amail->date, header, "\r\nDate:");
    if (result)
      amail->date = g_strdup (" ");

    /* FIXME: These should be filled out directly by the above code */
    /*  otherwise we are wasting time and ram */

    amail->header = g_strdup (header);
    amail->body = g_strdup (body);
    amail->mailserver = g_strdup (mailserver);
    amail->username = g_strdup (username);
    amail->folder = g_strdup (folder);

    /* Insert the mail into the database */
    /* FIXME: If this fails, it shouldn't just dump out of the function, it should attempt to get the rest of the mails */
    /*         (unless the add_mail() error was a fatal database error) */
    result = add_mail (amail);

    g_free (amail->from);
    g_free (amail->to);
    g_free (amail->subject);
    g_free (amail->date);
    g_free (amail->header);
    g_free (amail->body);
    g_free (amail->mailserver);
    g_free (amail->username);
    g_free (amail->folder);
    g_free (amail->uidl);
    g_free (amail);

    if (result > 0) {
      g_free (buffer->buf);
      g_free (buffer);
      g_free (mail_buf->buf);
      g_free (mail_buf);
      close (sockfd);
      mysql_close (&mysql);
      return (result);
    }

    if (delete) {
      g_snprintf (cmd2, sizeof (cmd2), "DELE %d\r\n", i + 1);
      tcp_read_write (io, buffer, cmd2, TCP_BOTH);
      /* Process the server's reply */
      /* If this starts with a '-' character it has failed */
      if (buffer->buf[0] == '-')
	display_error (_("Error: Delete failed"));
    }

    /* Finished parsing this mail. Go and get the next one. */
    gnome_appbar_pop (GNOME_APPBAR (w->StatusBar));
    while (gtk_events_pending ())
      gtk_main_iteration ();
    g_free (mail_buf->buf);
    g_free (mail_buf);
    mail = NULL;
  }

dpop3:
  printf ("Hit the dpop3 goto!!!\n");

  cmd = g_strconcat ("DSYNC ", dpop3_last_dsync (), "\r\n", NULL);
  tcp_read_write (io, buffer, cmd, TCP_BOTH);
  g_free (cmd);
  if (buffer->buf[0] == '-') {
    display_error (_("Warning: The mailserver failed on the DSYNC command\nAre you sure it supports DPOP3?"));
    exit (0);
  }
printf ("Got back a buffer: '%s'\n", buffer->buf);
  /* Loop through all the commands */
  cmd = buffer->buf;

  for (;;) {
    tmp = strstr (cmd, "\r\n");
    if (!tmp) break;
    *tmp = '\0';
    cmd = strchr (cmd, ' ');
    if (!cmd) break;
    cmd++;
    printf ("Processing command: '%s'\n", cmd);
    if (!(strncmp (cmd, "DMRETR", 6))) {
      dpop3_dmretr (io, cmd + 7);
    } else if (!(strncmp (cmd, "DMMOVE", 6))) {
      dpop3_dmmove (cmd + 7);
    } else if (!(strncmp (cmd, "DMDELE", 6))) {
      dpop3_dmdele (cmd + 7);
    } else if (!(strncmp (cmd, "DFNEW", 5))) {
      dpop3_dfnew (cmd + 6);
    } else if (!(strncmp (cmd, "DFMOVE", 6))) {
      dpop3_dfmove (cmd + 7);
    } else if (!(strncmp (cmd, "DFDELE", 6))) {
      dpop3_dfdele (cmd + 7);
    } else if (!(strncmp (cmd, "DFRENE", 6))) {
      dpop3_dfrene (cmd + 7);
    }
    cmd = tmp + 2;
  }

  /* Send our sync log */
  dpop3_dsync (io);

  /* Terminate the connection */
  cmd = g_strconcat ("QUIT\r\n", NULL);
  tcp_read_write (io, buffer, cmd, TCP_BOTH);
  g_free (cmd);
  if (buffer->buf[0] == '-') {
    display_error (_("Warning: The mailserver failed on the QUIT command\nThis often means that a DELE (delete) command has failed\nYou should consult your system administrator and possibly use a debug build of Treehouse to obtain more information"));
  }

  /* All mails done, return success */
  close (sockfd);
  g_free (buffer->buf);
  g_free (buffer);
  mysql_close (&mysql);

  return (0);
}

/*
	gint extract_header( gchar *header, gchar *source, gchar *name );
	
	Extracts the first occurance of the header labeled 'name' in 'source' and puts it
	 in 'header'.
	NOTE: You need to g_free() header yourself.
	
	RETURNS:
		0	-	Success
		1	-	'name' not found
		2	-	Header error

*/
gint extract_header (gchar ** header, gchar * source, gchar * name)
{
  gchar *start = NULL;
  guint i = 0;

  /* Find the first location of 'name' */
  start = strstrcase (source, name);
  if (!start) {
    return (1);
  }

  /* Advance 'start' to point at the actual text itself instead of 'name' */
  start += strlen (name) + 1;

  /* Allocate some mem for header */
  *header = g_malloc (strlen (source) + 1);

  while (*start) {
    if (*start == '\r') {
      (*header)[i] = ' ';
      i++;
      start++;
      continue;
    }
    if ((*start == '\n') && (*++start != ' ')) {
      (*header)[i] = '\0';
      break;
    }
    (*header)[i] = *start;
    i++;
    start++;
  }

  *header = g_strstrip (*header);

  if (i == (strlen (start) - 1)) {
    /* We reached the end of the header string */
    /* This should only occur if we are viewing a message in the Outbox/Sent Items */
    /*  because they don't have complete headers */
    return (2);
  }

  return (0);
}

/*
	gchar *strstrcase( gchar *haystack, gchar *needle );
	
	Case insensetive version of strstr()

	INPUTS:
		gchar *haystack		-	String to search
		gchar *needle		-	String to find
	RETURNS:
		gchar *			-	Pointer to found string or NULL

	This function is based on one found in Spruce
*/
gchar *strstrcase(gchar *haystack, gchar *needle)
{
  gchar *ptr;
  guint len;

  if (!needle || !haystack)
    return (gchar*)NULL;

  len = strlen(needle);
  if (len > strlen(haystack))
    return (gchar*)NULL;

  for (ptr = haystack; *(ptr + len - 1) != '\0'; ptr++) {
    if (!g_strncasecmp(ptr, needle, len)) {
       return ptr;
    }
  }

  return (gchar*)NULL;
}

/*
	void cb_receive( void );
	
	Check for new mail on each of the configured accounts

	INPUTS:
	RETURNS:

*/
void cb_receive (void)
{
  gint res = 0;
  MYSQL mysql;
  MYSQL_RES *ress;
  MYSQL_ROW msg_list_row;

  gint i = 0;
  gint num_rows = 0;

  mysql = sql_connect ("treehouse");

  if (mysql_errno (&mysql)) {
    display_error (_("Unable to establish database connection"));
    return;
  }

  mysql_query (&mysql, "SELECT label,pop3,pop3port,pop3user,pop3passwd,delmail,checkdef FROM accounts WHERE checkdef = 1 ORDER BY label");

  ress = mysql_store_result (&mysql);

  num_rows = (int) mysql_num_rows (ress);

  for (i = 0; i < num_rows; i++) {
    msg_list_row = mysql_fetch_row (ress);

    while (gtk_events_pending ())
      gtk_main_iteration ();

    if (msg_list_row[6]) {
      gnome_appbar_push (GNOME_APPBAR (w->StatusBar), g_strconcat (_ (" Status: Checking for new mail on account: '"), msg_list_row[0], "'...", NULL));
      res = get_mail (msg_list_row[1], msg_list_row[3], msg_list_row[4], atoi (msg_list_row[2]), atoi (msg_list_row[5]), msg_list_row[0]);
    }

    if (res == 2)
      display_error (g_strconcat (_("Error: Unable to connect to server '"), msg_list_row[1], "'", NULL));

    while (gtk_events_pending ())
      gtk_main_iteration ();


    gnome_appbar_pop (GNOME_APPBAR (w->StatusBar));
    gnome_appbar_set_progress (GNOME_APPBAR (w->StatusBar), 0);
    while (gtk_events_pending ())
      gtk_main_iteration ();
  }

  fill_message_list (w, w->last_selected_folder);

  mysql_free_result (ress);
  mysql_close (&mysql);

  return;
}

/*
	gint parse_address( gchar **name, gchar **addr, gchar *source );
	
	Parse a From: header and return it's componants (name and email address)

	INPUTS:
		**name		-	Pointer to a place we can store the name
		**addr		-	Pointer to a place we can store the address
		*source		-	Unparsed text
	RETURNS:
		0	-	Success
		1	-	Failed
	NOTES:
		This function currently understands From: headers in the following formats:
			foo@bar.etc		    -	Case 1: Address only
			Foo <bar@etc.far>	-	Case 2: Name (free text) followed by '<'/'>' delimited address
			"Foo" <bar@etc.far>	-	Case 3: Name (free text, delimited by '"'/'"') followed by '<'/'>' delimited address
			Foo (bar@etc.far)	-	Case 4: Name (free text) followed by '('/')' delimited address
			foo@bar.etc (Far)	-	Case 5: Address followed by free text delimited by '('/')'
		These are the only From: header formats I am currently aware of (there are probably more).
		If no recognised format is encountered it will search for a '@' and work outwards from there until it finds whitespace,
		 a '\0' or a character not permitted in addresses (Case 6) and assume that whatever it finds is the email address.
		If that fails it will return as having failed (rval 1) and leave **name and **addr as NULL (Case 7).
		name and addr probably don't want to be NULL - if they are they will be left empty
*/
gint parse_address (gchar ** name, gchar ** addr, gchar * source)
{
  gchar *tmp1, *tmp2;
  gint use_fallback = 0;

#ifdef DEBUG
  g_print ("Entering parse_address\n");
  g_print ("Parsing: '%s'\n", source);
#endif

  tmp1 = strchr (source, '<');
  if (tmp1) {
#ifdef DEBUG
    g_print ("Found '<' - case 2, 3 or junk\n");
#endif
    tmp2 = strchr (tmp1 + 1, '>');
    if (tmp2) {
      /* We have found: ?<?> - either case 2 or 3 */
#ifdef DEBUG
      g_print ("Found '>' after '<' - assuming case 2 or 3\n");
#endif
      /* Separate the two fields with '\0's */
      *tmp1 = '\0';
      tmp1++;
      *tmp2 = '\0';

      /* Remove any leading/trailing white space */
      source = g_strstrip (source);
      tmp1 = g_strstrip (tmp1);

      /* Remove any leading/trailing '"' characters from the name part */
      if (*source == '"') {
	source++;
      }
      if (source[strlen (source) - 1] == '"') {
	source[strlen (source) - 1] = '\0';
      }
#ifdef DEBUG
      g_print ("Name: '%s' Addr: '%s'\n", source, tmp1);
#endif
      if (name) *name = g_strdup (source);
      if (addr) *addr = g_strdup (tmp1);
    } else {
      /* We have found something with a spurious '<' in it */
      /* Fallback on working out the email from the '@'    */
#ifdef DEBUG
      g_print ("Suprious '<' found, using fallback\n");
#endif
      use_fallback = 1;
    }
  } else {
    /* Case 1, 4 or 5 (no '<' found) */
    tmp1 = strchr (source, '(');
    if (tmp1) {
      /* Case 4, 5 or junk */
#ifdef DEBUG
      g_print ("Found '(' - case 4, 5 or junk\n");
#endif
      tmp2 = strchr (tmp1, '@');
      if (tmp2) {
	/* Case 4 or junk */
#ifdef DEBUG
	g_print ("?(@? found - case 4 or junk\n");
#endif
	tmp2 = strchr (tmp1 + 1, ')');
	if (tmp2) {
	  /* Found ?(?@?) - Case 4 */
#ifdef DEBUG
	  g_print ("Found ')' after '@' after '(' - assuming case 4\n");
#endif

	  /* Separate the two fields with '\0's */
	  *tmp1 = '\0';
	  tmp1++;
	  *tmp2 = '\0';

	  /* Remove any leading/trailing white space */
	  source = g_strstrip (source);
	  tmp1 = g_strstrip (tmp1);

	  /* Remove any leading/trailing '"' characters from the name part */
	  if (*source == '"') {
	    source++;
	  }
	  if (source[strlen (source) - 1] == '"') {
	    source[strlen (source) - 1] = '\0';
	  }
#ifdef DEBUG
	  g_print ("Name: '%s' Addr: '%s'\n", source, tmp1);
#endif
	  if (name) *name = g_strdup (source);
	  if (addr) *addr = g_strdup (tmp1);
	} else {
	  /* Found ?(?@? - assuming junk and falling back */
#ifdef DEBUG
	  g_print ("Spurious '(' encountered before '@', using fallback\n");
#endif
	  use_fallback = 1;
	}
      } else {
	/* Case 5 or junk */
	tmp1 = strchr (source, '@');
	if (tmp1) {
	  tmp2 = strchr (tmp1, '(');
	  if (tmp2) {
	    tmp1 = strchr (tmp2, ')');
	    if (tmp1) {
#ifdef DEBUG
	      g_print ("Case 5 found\n");
#endif

	      *tmp1 = '\0';
	      *tmp2 = '\0';
	      tmp2++;

	      source = g_strstrip (source);
	      tmp2 = g_strstrip (tmp2);

#ifdef DEBUG
	      g_print ("Name: '%s' Addr: '%s'\n", tmp2, source);
#endif
	      if (name) *name = g_strdup (tmp2);
	      if (addr) *addr = g_strdup (source);
	    } else {
	      use_fallback = 1;
	    }
	  } else {
	    use_fallback = 1;
	  }
	} else {
	  use_fallback = 1;
	}
      }
    } else {
      /* case 1 or junk - fallback suggested */
      source = g_strstrip (source);
#ifdef DEBUG
      g_print ("Name: '%s' Addr: '%s'\n", "", source);
#endif
      if (name) *name = g_strdup ("");
      if (addr) *addr = g_strdup (source);
    }
  }

  if (use_fallback) {
    g_print ("Sorry, not implemented yet!\n");
  }
#ifdef DEBUG
  g_print ("Leaving parse_address\n");
#endif

  return (0);
}
