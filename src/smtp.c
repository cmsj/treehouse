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

   smtp.c

   Handles sending mails.
   $Header: /cvsroot/treehouse/treehouse/src/smtp.c,v 1.7 2002/05/21 07:12:43 chrisjones Exp $
*/

/* Include our header file */
#include "header.h"

/*
   send_mail - Sends email out using the given server/port.
  
  	INPUTS:
  			*mail	-	structure containing all the fields pertaining to the mail
  	RETURNS:
  			0	-	Success.
  			1	-	Failed, unable to resolve the mail server's hostname (from "pop_connect")
  			2	-	Failed, unable to connect to the mail server (from "pop_connect")
  			3	-	Failed, unable to locate local hostname.
  			4	-	Unknown server error.
  			6	-	Unable to create a socket.
  			7	-	Invalid from address.
  			8	-	Invalid to address(es).
  			9	-	Invalid cc address(es).
*/
gint send_mail (struct smtp_mail * mail)
{
  gint sockfd;
  gchar localhost[256];
  gchar *cmd = NULL;
  gchar tmp[1024];
  gint result = 0;
  gint i = 0, j = 0, l = 0;
  struct utsname *buname;

  buf_info *buffer;

  GIOChannel *io;

  result = gethostname (localhost, sizeof (localhost) * 1024);
  if (result != 0)
    return (3);

  if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    return (6);

  io = g_io_channel_unix_new (sockfd);

  result = tcp_connect (io, mail->server, mail->port);
  if (result > 0) {
    close (sockfd);
    return (result);
  }

  buffer = g_malloc (sizeof (*buffer));
  buffer->buf = g_malloc (buffer->size = MAX_SERVER_BUF);

  /* Read the server's welcome banner */
  tcp_read_write (io, buffer, "", TCP_READ);

  cmd = g_strconcat ("HELO ", localhost, "\r\n", NULL);
  tcp_read_write (io, buffer, cmd, TCP_BOTH);
  g_free (cmd);

  cmd = g_strconcat ("MAIL FROM:<", mail->from, ">\n", NULL);
  tcp_read_write (io, buffer, cmd, TCP_BOTH);
  g_free (cmd);

  if (!(buffer->buf[0] == '2' && buffer->buf[1] == '5' && buffer->buf[2] == '0')) {
    /* The server has responded other than "250 foo...." so something went wrong. */
printf ("Server replied: '%s'\n", buffer->buf);
    close (sockfd);
    g_free (buffer->buf);
    g_free (buffer);
    return (7);
  }

  /* Begin decoding 'mail->to', 'mail->cc' and 'mail->bcc' (comma delimited and terminated) */
  for (i = 0; i < strlen (mail->to); i++) {
    if (mail->to[i] == ',')
      j++;
  }

  if (j == 0) {
    display_error (_("Error: No detectable \"To:\" addresses"));
    close (sockfd);
    g_free (buffer->buf);
    g_free (buffer);
    return (8);
  }

  if (j == 1) {
    mail->to[strlen (mail->to) - 1] = '\0';
    cmd = g_strconcat ("RCPT TO:<", mail->to, ">\n", NULL);
    tcp_read_write (io, buffer, cmd, TCP_BOTH);
    g_free (cmd);

    /* 250 means it's ok, 251 means the server is going to forward the mail for us */
    if (!(buffer->buf[0] == '2' && buffer->buf[1] == '5' && (buffer->buf[2] == '0' || buffer->buf[2] == '1'))) {
      /* Server has rejected the address we specified */
printf ("Server replied: '%s'\n", buffer->buf);
      close (sockfd);
      g_free (buffer->buf);
      g_free (buffer);
      return (8);
    }
  } else {
    for (i = 0; i < strlen (mail->to); i++) {
      /* We have just reached the end of an address */
      if (mail->to[i] == ',') {
	/* Advance the "pointer" to the start of the next address */
	l = i + 1;

	cmd = g_strconcat ("RCPT TO:<", tmp, ">\n", NULL);
	tcp_read_write (io, buffer, cmd, TCP_BOTH);
	g_free (cmd);

	if (!(buffer->buf[0] == '2' && buffer->buf[1] == '5' && buffer->buf[2] == '0')) {
	  /* server rejected the address we specified */
printf ("Server replied: '%s'\n", buffer->buf);
	  display_error (g_strconcat ("Mail server rejected address: '", tmp, "' with error '", buffer->buf, "'", NULL));
	}

	j--;
	if (j == 0) {
	  break;
	} else
	  continue;
      }

      if (mail->to[i] == '\0') {
	break;
      }

      /* Copy the current character of the current address */
      tmp[i - l] = mail->to[i];

      /* Set the next character to a null termination, just in case it is the last character of the address. */
      tmp[i - l + 1] = '\0';
    }
  }

  cmd = g_strdup ("DATA\n");
  tcp_read_write (io, buffer, cmd, TCP_BOTH);
  g_free (cmd);

  buname = g_malloc (sizeof (struct utsname));
  uname (buname);

  /* Send the rest of the headers */
  cmd = g_strconcat ("X-Mailer: Treehouse ", VERSION, " (", buname->sysname, " ",
		 buname->machine, " ", buname->release, ")\nFrom: ",
		 mail->from_proper, "\nOrganisation: ", mail->org, "\nTo: ",
		 mail->to_proper, "\nSubject: ", mail->subject, "\n", NULL);

  g_free (buname);

  tcp_read_write (io, buffer, cmd, TCP_WRITE);

  g_free (cmd);

  cmd = g_strconcat (mail->message, "\r\n.\r\n", NULL);

  tcp_read_write (io, buffer, cmd, TCP_BOTH);
  g_free (cmd);

  cmd = g_strdup ("QUIT\n");
  tcp_read_write (io, buffer, cmd, TCP_BOTH);
  g_free (cmd);

printf ("Server replied: '%s'\n", buffer->buf);
  g_free (buffer->buf);
  g_free (buffer);
  close (sockfd);

  return (0);
}

/*
	void cb_send( void );
	
	Deliver any waiting mails

	INPUTS:
	RETURNS:

*/
void cb_send (void)
{
  MYSQL mysql, mysql2;
  MYSQL_RES *res, *res2;
  MYSQL_ROW msg_list_row, msg_list_row2;
  gchar *query;
  gint num_rows = 0, num_rows2 = 0, i = 0, result = 0;
  struct smtp_mail *mail;
  gchar *txt;
  float i_f, nummsg_f;
  gint bfile, hfile;
  gchar *hfile_txt;

  txt = g_strdup_printf (" Status: Sending mails...");
  gnome_appbar_push (GNOME_APPBAR (w->StatusBar), txt);
  g_free (txt);
  while (gtk_events_pending ())
    gtk_main_iteration ();

  mysql = sql_connect ("treehouse");

  if (mysql_errno (&mysql)) {
    display_error (_("Unable to establish database connection"));
    return;
  }

  mysql_query (&mysql, "SELECT * FROM mail WHERE mfolder = 'Outbox' ORDER BY mreceived");

  if (mysql_errno (&mysql)) {
    display_error ((char *) mysql_error (&mysql));
    mysql_close (&mysql);
    return;
  }

  res = mysql_store_result (&mysql);
  num_rows = (int) mysql_num_rows (res);

  while (gtk_events_pending ())
    gtk_main_iteration ();

  for (i = 0; i < num_rows; i++) {
    txt = g_strdup_printf (" Status: Sending mails... %d of %d", i + 1, num_rows);
    gnome_appbar_push (GNOME_APPBAR (w->StatusBar), txt);
    g_free (txt);
    i_f = i + 1;
    nummsg_f = num_rows;
    gnome_appbar_set_progress (GNOME_APPBAR (w->StatusBar), (i_f / nummsg_f));
    while (gtk_events_pending ())
      gtk_main_iteration ();

    mail = g_malloc (sizeof (*mail));

    msg_list_row = mysql_fetch_row (res);

    mail->to_proper = g_strdup (msg_list_row[2]);
    mail->from_proper = g_strdup (msg_list_row[1]);
    /*mail->message = g_strdup (msg_list_row[7]);*/
    mail->subject = g_strdup (msg_list_row[3]);

    bfile = open_file (msg_list_row[0], "b");
    mail->message = read_file (bfile);
    close (bfile);

    hfile = open_file (msg_list_row[0], "h");
    hfile_txt = read_file (hfile);
    close (hfile);

    if (extract_header (&mail->cc_proper, hfile_txt, "\nCc: "))
      mail->cc_proper = g_strdup (" ");

    g_free (hfile_txt);

    query = g_strconcat ("SELECT * FROM accounts WHERE label = '", msg_list_row[11], "'", NULL);
    mysql2 = sql_connect ("treehouse");
    if (mysql_errno (&mysql2)) {
      display_error (_("Unable to establish database connection"));
      g_free (mail->cc_proper);
      g_free (mail->subject);
      g_free (mail->message);
      g_free (mail->from_proper);
      g_free (mail->to_proper);
      g_free (mail);
      mysql_free_result (res);
      mysql_close (&mysql);
      break;
    }

    mysql_query (&mysql2, query);
    g_free (query);

    if (mysql_errno (&mysql2)) {
      display_error ((char *) mysql_error (&mysql2));
      mysql_close (&mysql2);
      g_free (mail->cc_proper);
      g_free (mail->subject);
      g_free (mail->message);
      g_free (mail->from_proper);
      g_free (mail->to_proper);
      g_free (mail);
      mysql_free_result (res);
      mysql_close (&mysql);
      return;
    }

    res2 = mysql_store_result (&mysql2);
    num_rows2 = (int) mysql_num_rows (res2);

    if (num_rows2 != 1) {
      display_error (_("Accounts error"));
      g_free (mail->cc_proper);
      g_free (mail->subject);
      g_free (mail->message);
      g_free (mail->from_proper);
      g_free (mail->to_proper);
      g_free (mail);
      mysql_free_result (res);
      mysql_close (&mysql);
      mysql_free_result (res2);
      mysql_close (&mysql2);
      return;
    }

    msg_list_row2 = mysql_fetch_row (res2);

    mail->from = g_strdup (msg_list_row2[3]);
    mail->server = g_strdup (msg_list_row2[8]);
    mail->port = atoi (msg_list_row2[9]);
    mail->org = g_strdup (msg_list_row2[4]);
    mail->replyto = g_strdup (msg_list_row2[5]);

    mysql_free_result (res2);

    /* FIXME: Parse mail->to and mail->cc */

    mail->to = g_strconcat (mail->to_proper, ",", NULL);
    mail->cc = g_strdup ("");

    if (!(result = send_mail (mail))) {
      query = g_strconcat ("UPDATE mail SET mfolder='SentItems',mread=1 WHERE mId = '", msg_list_row[0], "'", NULL);
      dpop3_log_add ("DMMOVE", g_strconcat (msg_list_row[0], " ", dpop3_get_foldid ("SentItems"), NULL));
      mysql_query (&mysql2, query);
      if (mysql_errno (&mysql2)) {
	display_error ((char *) mysql_error (&mysql2));
      }
      g_free (query);
    } else {
      display_error (_("Sending failed"));
    }

    g_free (mail->org);
    g_free (mail->replyto);
    g_free (mail->cc_proper);
    g_free (mail->subject);
    g_free (mail->message);
    g_free (mail->from_proper);
    g_free (mail->to_proper);
    g_free (mail->from);
    g_free (mail->server);
    g_free (mail->to);
    g_free (mail->cc);
    g_free (mail);
    mysql_close (&mysql2);
    gnome_appbar_pop (GNOME_APPBAR (w->StatusBar));
    while (gtk_events_pending ())
      gtk_main_iteration ();
  }

  mysql_free_result (res);
  mysql_close (&mysql);

  /* Refresh the Outbox if it is selected */
  if (!(g_strcasecmp (w->last_selected_folder, "Outbox")))
    fill_message_list (w, w->last_selected_folder);

  gnome_appbar_pop (GNOME_APPBAR (w->StatusBar));
  gnome_appbar_set_progress (GNOME_APPBAR (w->StatusBar), 0.0);
  while (gtk_events_pending ())
    gtk_main_iteration ();

  return;
}
