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
    (permission was obtained for this)
    Some code taken from or inspired by Spruce (which is GPL'd)

    message.c

    Message related functions
    $Header: /cvsroot/treehouse/treehouse/src/message.c,v 1.8 2002/05/21 08:42:57 chrisjones Exp $
*/

/* Includes */
#include "header.h"
#include "time.h"

/*
   fill_message_list - Insert mail listing into CList gadget
  
  	INPUTS:
			*w			-	structure used to store main window widgets
			*name		-	Name of the folder we must list
  	RETURNS:
*/
void fill_message_list (struct MainWindow *w, gchar * name)
{
  MYSQL mysql;
  MYSQL_RES *res;
  MYSQL_ROW msg_list_row;
  gchar *cmd, *status;
  gchar *empty[] = { "", "", N_("No items found"), "", "<NONE>" };
  gint i = 0, num_rows = 0;
  gint row_num;
  GtkStyle *style1, *style2, *style3;
  gchar *title;
  gint loc_unread = 0;
  unread = 0;

  if (!(g_strcasecmp (name, ""))) {
    gtk_clist_clear (GTK_CLIST (w->MessageList));
    gtk_clist_append (GTK_CLIST (w->MessageList), empty);
    gtk_clist_set_selectable (GTK_CLIST (w->MessageList), 0, FALSE);
    return;
  }

  title = g_strconcat ("Treehouse - ", name, NULL);
  gtk_window_set_title (GTK_WINDOW (w->MainWin), title);
  g_free (title);

  mysql = sql_connect ("treehouse");
  if (mysql_errno (&mysql)) {
    display_error (_("Unable to establish database connection"));
    return;
  }

  cmd = g_strconcat ("SELECT mreplied,mfrom,msubject,mreceived,mId,mread FROM mail WHERE mfolder = '", name, "' OR mfolder = '<UNDEF>' ORDER BY mreceived", NULL);
  mysql_query (&mysql, cmd);
  g_free (cmd);

  while (gtk_events_pending ())
    gtk_main_iteration ();

  res = mysql_store_result (&mysql);
  num_rows = (int) mysql_num_rows (res);
  while (gtk_events_pending ())
    gtk_main_iteration ();

  gtk_clist_freeze (GTK_CLIST (w->MessageList));
  gtk_clist_clear (GTK_CLIST (w->MessageList));

  style1 = gtk_widget_get_style (w->MessageList);
  style2 = gtk_style_copy (style1);
  style2->font = gdk_font_load (bm_cfg->bold_font);
  style3 = gtk_style_copy (style1);
  style3->font = gdk_font_load (bm_cfg->normal_font);

  if (!style2->font) {
    display_error ("Warning: Unable to load bold font");
    style2 = gtk_style_copy (style1);
  }

  if (!style3->font) {
    display_error ("Warning: Unable to load normal font");
    style3 = gtk_style_copy (style1);
  }

  if (num_rows == 0) {
    /* Nothing found */
    gtk_clist_append (GTK_CLIST (w->MessageList), empty);
    gtk_clist_set_selectable (GTK_CLIST (w->MessageList), 0, FALSE);
  } else {
    for (i = 0; i < num_rows; i++) {
      while (gtk_events_pending ())
	gtk_main_iteration ();

      msg_list_row = mysql_fetch_row (res);

      if (!msg_list_row) {
	display_error (_("Message listing failed"));
	mysql_free_result (res);
	mysql_close (&mysql);
	return;
      }

      row_num = gtk_clist_prepend (GTK_CLIST (w->MessageList), msg_list_row);

      /* Take care of unread/read styles */
      if (!atoi (msg_list_row[5])) {
	gtk_clist_set_row_style (GTK_CLIST (w->MessageList), row_num, style2);
	gtk_clist_set_pixmap (GTK_CLIST (w->MessageList), row_num, 0, closed_xpm, NULL);
	loc_unread++;
      } else {
	gtk_clist_set_row_style (GTK_CLIST (w->MessageList), row_num, style3);
	gtk_clist_set_pixmap (GTK_CLIST (w->MessageList), row_num, 0, open_xpm, NULL);
      }
    }
  }

  gtk_clist_thaw (GTK_CLIST (w->MessageList));
/*  gtk_style_unref (style2); */
/*  gtk_style_unref (style3); */

  unread = loc_unread;
  status = g_strdup_printf (_(" Status: Ready ( %d item%s, %d unread )"), num_rows, (num_rows == 1) ? "" : "s", loc_unread);
  gnome_appbar_clear_stack (GNOME_APPBAR (w->StatusBar));
  gnome_appbar_push (GNOME_APPBAR (w->StatusBar), status);
  g_free (status);

  while (gtk_events_pending ())
    gtk_main_iteration ();

  mysql_free_result (res);
  mysql_close (&mysql);

  return;
}

/*
   cb_show_message - Callback to display an individual message
  
  	INPUTS:
			*widget		-	The CList widget
			row			-	The row that was selected
			column		-	The column that was selected
			*event		-	Unused
			data		-	Unused
  	RETURNS:
*/
void cb_show_message (GtkWidget * widget, gint row, gint column, GdkEventButton * event, gpointer data)
{
  gchar *mId;
  gchar *html = NULL;
  MYSQL mysql;
  MYSQL_RES *res;
  MYSQL_ROW msg_row;
  gchar *cmd;
  gint len = 0;
  gint num_rows = 0;
  gint i = 0;
  GList *selection;
  gchar *cc, *html_cc = NULL;
  gchar *body = NULL;
  gchar *whole = NULL;
  gchar *text = NULL;
  gint bfile;
  gchar *bfile_txt = NULL;
  gchar *hfile_txt = NULL;
  gint is_mime = 0;
  gchar *mime_txt, *mime_tmp, *mime_tmp2;
  gint mime_len;
  struct mime_part *part;
  GList *mime = NULL, *tmp = NULL;

  struct add_mail *mail;

  selection = GTK_CLIST (w->MessageList)->selection;

  /* Check if there is a multiple selection - if so, we don't want to do anything */
  if (g_list_length (selection) > 1)
    return;

  gtk_clist_get_text (GTK_CLIST (widget), row, 4, &mId);

  if (mId == NULL || mId == '\0') {
    display_error (_("Error: Unable to locate UIDL in entry"));
    return;
  }

  /* Check to see if we are being asked to display the "No messages" placeholder */
  if (g_strcasecmp ((char *) mId, "<NONE>") == 0) {
    msg_pane_set_html (g_strconcat ("<html><body bgcolor=\"#", bm_cfg->html_back, "\">&nbsp;</body></html>", NULL));
    gnome_less_clear (GNOME_LESS (w->SourcePane));
    return;
  }

  /* Check to see if we are being asked to display the same mail again */
  /*  if so, we just return. This saves on mem and time */
  /* Changes elsewhere have probably made this redundant */
  if (g_strcasecmp (w->last_displayed_mail, (char *) mId) == 0)
    return;

  cmd = g_strconcat ("SELECT mfrom,mto,msubject,mreceived,mread FROM mail WHERE mId = '", (char *) mId, "'", NULL);

  mysql = sql_connect ("treehouse");

  if (mysql_errno (&mysql)) {
    display_error ("Unable to establish database connection");
    g_free (cmd);
    return;
  }

  mysql_query (&mysql, cmd);

  res = mysql_store_result (&mysql);
  num_rows = (gint) mysql_num_rows (res);

  if (num_rows == 0) {
    display_error (_("Error: No matches found"));
    g_free (cmd);
    mysql_free_result (res);
    mysql_close (&mysql);
    return;
  }

  while (gtk_events_pending ())
    gtk_main_iteration ();

  msg_row = mysql_fetch_row (res);

  /* Read the body file into memory */
  bfile = open_file (mId, "b");
  bfile_txt = read_file (bfile);
  close (bfile);

  /* Read the header file into memory */
  bfile = open_file (mId, "h");
  hfile_txt = read_file (bfile);
  close (bfile);

  while (gtk_events_pending ())
    gtk_main_iteration ();

  /* Prepare the mail as HTML text if necessary */
  mail = g_malloc0 (sizeof (*mail));

  if ((strstr (msg_row[0], "<") != NULL) || (strstr (msg_row[0], "&") != NULL)) {
    mail->from = g_malloc (html_len (msg_row[0]) + 1);
    text_to_html (mail->from, msg_row[0]);
  } else {
    mail->from = g_strdup (msg_row[0]);
  }

  if ((strstr (msg_row[1], "<") != NULL) || (strstr (msg_row[1], "&") != NULL)) {
    mail->to = g_malloc (html_len (msg_row[1]) + 1);
    text_to_html (mail->to, msg_row[1]);
  } else {
    mail->to = g_strdup (msg_row[1]);
  }

  if ((strstr (msg_row[2], "<") != NULL) || (strstr (msg_row[2], "&") != NULL)) {
    mail->subject = g_malloc (html_len (msg_row[2]) + 1);
    text_to_html (mail->subject, msg_row[2]);
  } else {
    mail->subject = g_strdup (msg_row[2]);
  }

  if (extract_header (&cc, hfile_txt, "\nCc:") == 1) {
    cc = g_strdup (" ");
  }

  if ((strstr (cc, "<") != NULL) || (strstr (cc, "&") != NULL)) {
    html_cc = g_malloc (html_len (cc) + 1);
    text_to_html (html_cc, cc);
  } else {
    html_cc = g_strdup (cc);
  }

  /* Detect MIME */
  if (!(extract_header (&body, hfile_txt, "\r\nMIME-Version:"))) {
    g_print ("MIME header: '%s'\n", body);
    if (body[0] == '1' && body[2] <= '1') {
      /* This is a MIME message we can understand */
      is_mime = 1;
    }
    g_free (body);
  }

  g_print ("MIME status of message: '%d'\n", is_mime);

  if (is_mime) {
    whole = g_strconcat (hfile_txt, "\r\n\r\n", bfile_txt, NULL);
    if (!(mime = mime_parse (hfile_txt, bfile_txt))) {
      g_free (whole);
    } else {
      g_print ("MIME parts: '%d'\n", g_list_length (mime));
    }
  }

  /* Column wrap the incoming mail */
  /* FIXME: This should be configurable */
  body = g_strdup (bfile_txt);
  col_wrap (&body, 85);

  i = html_len (body);
  mail->body = g_malloc (i + 1);
  while (gtk_events_pending ())
    gtk_main_iteration ();
  text_to_html (mail->body, body);

  while (gtk_events_pending ())
    gtk_main_iteration ();

  if (is_mime) {
    tmp = g_list_first (mime);
    printf ("Checking '%d' MIME parts for display\n", g_list_length (mime));
    num_rows = 0;
    g_free (mail->body);
    mail->body = g_strdup (" ");
    len = g_list_length (mime);
    for (num_rows = 0; num_rows < len; num_rows++) {
      mime_txt = g_strdup ("");
g_print ("Processing part: '%d'\n", num_rows);
      part = tmp->data;
g_print ("  '%s/%s'\n", part->type, part->subtype);
      if (!g_strcasecmp (part->type, "text")) {
        if (num_rows != mime_get_first_text (mime)) {
          mime_txt = g_strconcat (mime_txt, "<hr width=\"80%\">", NULL);
          if (strcasestr (part->disposition, "filename=\"")) {
            html = mime_get_parameter_value (part->disposition, "filename");
            mime_txt = g_strconcat (mime_txt, "<table width=\"100%\" bgcolor=\"", TAB_BACK, "\" spacing=\"0\" padding=\"5\"><tr><td><b>Attachment:</b> ", html, "</td><td><b>Type:</b> ", part->type, "/", part->subtype, "<br></td></tr><tr><td>&nbsp;</td><td><b>Encoding:</b> ", part->encoding, "</td></tr></table>", NULL);
            g_free (html);
          }
        }
        mime_tmp = mime_get_part (body, mime, num_rows, &mime_len);
        *(mime_tmp + strlen (mime_tmp) - 3) = '\0';
        if (g_strcasecmp (part->subtype, "html")) {
          printf ("Not an HTML part, converting ASCII to HTML\n");
          mime_tmp2 = g_malloc0 (html_len (mime_tmp) + 1);
          text_to_html (mime_tmp2, mime_tmp);
          g_free (mime_tmp);
          mime_tmp = g_strdup (mime_tmp2);
          g_free (mime_tmp2);
        }
        mime_txt = g_strconcat (mime_txt, mime_tmp, NULL);
        g_free (mime_tmp);
      }
      if (!g_strcasecmp (part->type, "image")) {
        mime_tmp = g_malloc (256);
        sprintf (mime_tmp, "%d", num_rows + 1);
        mime_txt = g_strconcat (mime_txt, "<hr width=\"80%\">", NULL);
        if (strcasestr (part->disposition, "filename=\"")) {
          html = mime_get_parameter_value (part->disposition, "filename");
          mime_txt = g_strconcat (mime_txt, "<table width=\"100%\" bgcolor=\"", TAB_BACK, "\" spacing=\"0\" padding=\"5\"><tr><td><b>Attachment:</b> ", html, "</td><td><b>Type:</b> ", part->type, "/", part->subtype, "<br></td></tr><tr><td>&nbsp;</td><td><b>Encoding:</b> ", part->encoding, "</td></tr></table>", NULL);
          g_free (html);
        }
        mime_txt = g_strconcat (mime_txt, "<center><img src=\"mime://", mId, "?part=", mime_tmp, "\"></center>", NULL);
        g_free (mime_tmp);
      }
      if (!g_strcasecmp (part->type, "application")) {
        mime_tmp = g_malloc (256);
        sprintf (mime_tmp, "%d", num_rows + 1);
        mime_txt = g_strconcat (mime_txt, "<hr width=\"80%\">", NULL);
        if (strcasestr (part->disposition, "filename=\"")) {
          html = mime_get_parameter_value (part->disposition, "filename");
          mime_txt = g_strconcat (mime_txt, "<table width=\"100%\" bgcolor=\"", TAB_BACK, "\" spacing=\"0\" padding=\"5\"><tr><td><b>Attachment:</b> ", html, "</td><td><b>Type:</b> ", part->type, "/", part->subtype, "<br></td></tr><tr><td>&nbsp;</td><td><b>Encoding:</b> ", part->encoding, "</td></tr></table>", NULL);
          mime_txt = g_strconcat (mime_txt, "<center><a href=\"mime://", mId, "?part=", mime_tmp, "\">", html, "</a></center>", NULL);
          g_free (html);
        }
        g_free (mime_tmp);
      }
      if (strlen (mime_txt) > 1) {
        mail->body = g_strconcat (mail->body, mime_txt, NULL);
        g_free (mime_txt);
      }
      tmp = tmp->next;
g_print ("Done processing part '%d'\n", num_rows);
    }
  }

  html = g_strdup_printf ("<html><head><title>Mail</title></head><body bgcolor=\"#%s\" text=\"#%s\" topmargin=\"1\" leftmargin=\"1\"><table width=\"100%%\" bgcolor=\"#%s\" spacing=\"0\" padding=\"5\" border=\"0\"><tr><td width=\"50%%\" nowrap><font size=\"4\">&nbsp;<b>From:</b>&nbsp;%s&nbsp;</font></td><td width=\"50%%\"><font size=\"4\">&nbsp;<b>To:</b>&nbsp;%s&nbsp;</font></td></tr><tr><td><font size=\"4\">&nbsp;<b>Cc:</b>&nbsp;%s&nbsp;</font></td><td nowrap><font size=\"4\">&nbsp;<b>Received:</b>&nbsp;%s&nbsp;</font></td></tr><tr><td colspan=\"2\"><font size=\"4\">&nbsp;<b>Subject:</b>&nbsp;%s&nbsp;</font></td></tr></table><pre>%s</pre></body></html>",
	bm_cfg->html_back, bm_cfg->html_fore, TAB_BACK, mail->from, mail->to,
	html_cc, msg_row[3], mail->subject, mail->body);

  if (is_mime)
    g_free (whole);

  while (gtk_events_pending ())
    gtk_main_iteration ();
    
  msg_pane_set_html (html); 
  strcpy (w->last_displayed_mail, (char *) mId);

  gnome_less_clear (GNOME_LESS (w->SourcePane));
  gnome_less_show_string (GNOME_LESS (w->SourcePane), g_strconcat (hfile_txt, "\n", bfile_txt, NULL));

  while (gtk_events_pending ())
    gtk_main_iteration ();

  if (!atoi (msg_row[4]))
    cb_mark_read (w, row, mId);

  while (gtk_events_pending ())
    gtk_main_iteration ();

  if (cmd) g_free (cmd);
  if (mail->from) g_free (mail->from);
  if (mail->to) g_free (mail->to);
  if (mail->subject) g_free (mail->subject);
  if (mail->body) g_free (mail->body);
  if (cc) g_free (cc);
  if (html_cc) g_free (html_cc);
  if (mail) g_free (mail);
  if (text) g_free (text);
  if (html) g_free (html);
  if (body) g_free (body);
  if (bfile_txt) g_free (bfile_txt);
  if (hfile_txt) g_free (hfile_txt);

  mysql_free_result (res);
  mysql_close (&mysql);

  printf ("Done displaying message\n");
  return;

}

/*
    cb_delete_mail - Callback to delete mails (by moving them to the "Deleted Items" folder)

	INPUTS:
			*widget			-	unused
			*data			-	unused
	RETURNS:
*/
void cb_delete_mail (GtkWidget * widget, gpointer * data)
{
  GList *foo;
  gint row = 0;
  gchar *mId;
  MYSQL mysql;
  gchar *cmd = NULL;
  guint num_sel = 0;
  gint i = 0;
  float i_f, num_sel_f;
  gint clist_rows = 0;

  foo = GTK_CLIST (w->MessageList)->selection;

  num_sel = g_list_length (foo);

  if (!foo)
    return;

  mysql = sql_connect ("treehouse");

  if (mysql_errno (&mysql)) {
    display_error (_("Unable to establish database connection"));
    g_free (cmd);
    return;
  }

  gnome_appbar_push (GNOME_APPBAR (w->StatusBar), _(" Status: Deleting mails..."));

  for (i = 0; i < num_sel; i++) {
    i_f = i + 1;
    num_sel_f = num_sel;

    gnome_appbar_set_progress (GNOME_APPBAR (w->StatusBar), (i_f / num_sel_f));
    while (gtk_events_pending ())
      gtk_main_iteration ();

    row = GPOINTER_TO_INT (foo->data);

    gtk_clist_get_text (GTK_CLIST (w->MessageList), row, 4, &mId);
    if (!g_strcasecmp ((char *) mId, "<NONE>"))
      continue;

    if (!g_strncasecmp (w->last_selected_folder, "Deleted Items", 13)) {
      cmd = g_strconcat ("DELETE FROM mail WHERE mId = '", (char *) mId, "'", NULL);
      dpop3_log_add ("DMDELE", mId);
    } else {
      cmd = g_strconcat ("UPDATE mail SET mfolder = 'Deleted Items' WHERE mId = '", (char *) mId, "'", NULL);
      dpop3_log_add ("DMMOVE", g_strconcat (mId, " ", dpop3_get_foldid ("Deleted Items"), NULL));
    }

    mysql_real_query (&mysql, cmd, strlen (cmd));
    if (mysql_errno (&mysql))
      display_error ((char *) mysql_error (&mysql));

    g_free (cmd);

    foo = foo->next;
  }

  gnome_appbar_set_progress (GNOME_APPBAR (w->StatusBar), 0.0);
  gnome_appbar_pop (GNOME_APPBAR (w->StatusBar));
  while (gtk_events_pending ())
    gtk_main_iteration ();

  mysql_close (&mysql);

  gtk_clist_freeze (GTK_CLIST (w->MessageList));

  fill_message_list (w, w->last_selected_folder);

  if (row > 0)
    row--;

  clist_rows = GTK_CLIST (w->MessageList)->rows;

  if (GTK_CLIST (w->MessageList)->rows <= row) {
    row = clist_rows - 1;
  }

  gtk_clist_select_row (GTK_CLIST (w->MessageList), row, 0);

  if (gtk_clist_row_is_visible (GTK_CLIST (w->MessageList), row) != GTK_VISIBILITY_FULL) {
    gtk_clist_moveto (GTK_CLIST (w->MessageList), row, 0, 1.0, 0.0);
    if (row != 0) {
      cb_show_message (w->MessageList, row, 0, NULL, NULL);
    } else if (row == 0) {
      cb_show_message (w->MessageList, 0, 0, NULL, NULL);
    } else {
      display_error ("Why the hell is this here?");
      msg_pane_set_html
	("<html><body bgcolor=\"#FFFFFF\">&nbsp;</body></html>");
      gnome_less_clear (GNOME_LESS (w->SourcePane));
    }
  }

  gtk_clist_thaw (GTK_CLIST (w->MessageList));

  return;

}

/*
    cb_mark_read - Callback to mark mails as read

	INPUTS:
			*w			-	MainWindow structure
			row			-	Row clicked on
			*mId			-	Message id
	RETURNS:
*/
void cb_mark_read (struct MainWindow *w, gint row, gchar * mId)
{
  MYSQL mysql;
  gchar *cmd = NULL;
  GtkStyle *style1, *style2;
  gchar *status;
  gint num_rows = 0;

  cmd = g_strconcat ("UPDATE mail SET mread = 1 WHERE mId = '", (char *) mId, "'", NULL);

  mysql = sql_connect ("treehouse");

  if (mysql_errno (&mysql)) {
    display_error (_("Unable to establish database connection"));
    g_free (cmd);
    return;
  }

  while (gtk_events_pending ())
    gtk_main_iteration ();
  mysql_query (&mysql, cmd);
  while (gtk_events_pending ())
    gtk_main_iteration ();

  mysql_close (&mysql);
  g_free (cmd);

  style1 = gtk_widget_get_style (w->MessageList);
  style2 = gtk_style_copy (style1);
  style2->font = gdk_font_load (bm_cfg->normal_font);
  if (style2->font)
    gtk_clist_set_row_style (GTK_CLIST (w->MessageList), row, style2);
  else
    display_error ("Font loading failed!");
  gtk_style_unref (style2);
  gtk_clist_set_pixmap (GTK_CLIST (w->MessageList), row, 0, open_xpm, NULL);

  unread--;

  num_rows = GTK_CLIST (w->MessageList)->rows;
  status = g_strdup_printf (_(" Status: Ready ( %d item%s, %d unread )"), num_rows, (num_rows == 1) ? "" : "s", unread);
  gnome_appbar_pop (GNOME_APPBAR (w->StatusBar));
  gnome_appbar_push (GNOME_APPBAR (w->StatusBar), status);
  g_free (status);

  return;

}

/*
    cb_mark_read_context - Mark mails as read from the right click menu

	INPUTS:
			*widget			-	unused
			*data			-	unused
	RETURNS:
*/
void cb_mark_read_context (GtkWidget * widget, gpointer * data)
{
  GList *foo;
  gint row;
  gchar *mId;
  gint i = 0;
  gint num_sel;

  foo = GTK_CLIST (w->MessageList)->selection;
  if (!foo)
    return;

  num_sel = g_list_length (foo);

  gtk_clist_freeze (GTK_CLIST (w->MessageList));

  for (i = 0; i < num_sel; i++) {
    row = GPOINTER_TO_INT (foo->data);

    gtk_clist_get_text (GTK_CLIST (w->MessageList), row, 4, &mId);
    if (!g_strcasecmp ((char *) mId, "<NONE>"))
      continue;

    cb_mark_read (w, row, (char *) mId);
    while (gtk_events_pending ())
      gtk_main_iteration ();

    foo = foo->next;
  }

  gtk_clist_thaw (GTK_CLIST (w->MessageList));

  return;
}

/*
    cb_mark_unread - Callback to mark mails as read

	INPUTS:
			*w			-	MainWindow structure
			row			-	Row clicked on
			*mId			-	Message id
	RETURNS:
*/
void cb_mark_unread (struct MainWindow *w, gint row, gchar * mId)
{
  MYSQL mysql;
  gchar *cmd;
  GtkStyle *style1, *style2;
  gchar *status;
  gint num_rows = 0;

  cmd = g_strconcat ("UPDATE mail SET mread = 0 WHERE mId = '", (char *) mId, "'", NULL);

  mysql = sql_connect ("treehouse");

  if (mysql_errno (&mysql)) {
    display_error ("Unable to establish database connection");
    g_free (cmd);
    return;
  }

  while (gtk_events_pending ())
    gtk_main_iteration ();
  mysql_query (&mysql, cmd);
  while (gtk_events_pending ())
    gtk_main_iteration ();

  mysql_close (&mysql);
  g_free (cmd);

  style1 = gtk_widget_get_style (w->MessageList);
  style2 = gtk_style_copy (style1);
  style2->font = gdk_font_load (bm_cfg->bold_font);
  if (style2->font)
    gtk_clist_set_row_style (GTK_CLIST (w->MessageList), row, style2);
  gtk_style_unref (style2);
  gtk_clist_set_pixmap (GTK_CLIST (w->MessageList), row, 0, closed_xpm, NULL);

  unread++;

  num_rows = GTK_CLIST (w->MessageList)->rows;
  status = g_strdup_printf (_(" Status: Ready ( %d item%s, %d unread )"), num_rows, (num_rows == 1) ? "" : "s", unread);
  gnome_appbar_pop (GNOME_APPBAR (w->StatusBar));
  gnome_appbar_push (GNOME_APPBAR (w->StatusBar), status);
  g_free (status);

  return;
}

/*
    cb_mark_unread_context - Mark mails as read from the right click menu

	INPUTS:
			*widget			-	unused
			*data			-	unused
	RETURNS:
*/
void cb_mark_unread_context (GtkWidget * widget, gpointer * data)
{
  GList *foo;
  gint row;
  gchar *mId;
  guint num_sel;
  gint i = 0;

  foo = GTK_CLIST (w->MessageList)->selection;
  if (!foo)
    return;

  num_sel = g_list_length (foo);

  gtk_clist_freeze (GTK_CLIST (w->MessageList));

  for (i = 0; i < num_sel; i++) {
    row = GPOINTER_TO_INT (foo->data);

    gtk_clist_get_text (GTK_CLIST (w->MessageList), row, 4, &mId);
    if (!g_strcasecmp ((char *) mId, "<NONE>"))
      continue;

    cb_mark_unread (w, row, (char *) mId);
    while (gtk_events_pending ())
      gtk_main_iteration ();

    foo = foo->next;
  }

  gtk_clist_thaw (GTK_CLIST (w->MessageList));

  return;
}

/*
    cb_mark_all_read - Mark all mails as read

	INPUTS:
			*widget			-	unused
			*data			-	unused
	RETURNS:
*/
void cb_mark_all_read (GtkWidget * widget, gpointer * data)
{
  gchar *folder, *tmp;
  MYSQL mysql;
  gchar *cmd = NULL;

  folder = g_strdup (w->last_selected_folder);
  tmp = strstr (folder, " (");
  if (tmp)
    *tmp = '\0';

  cmd = g_strconcat ("UPDATE mail SET mread = 1 WHERE mfolder = '", folder, "'", NULL);

  mysql = sql_connect ("treehouse");

  if (mysql_errno (&mysql)) {
    display_error (_("Unable to establish database connection"));
    g_free (cmd);
    g_free (folder);
    return;
  }

  gnome_appbar_push (GNOME_APPBAR (w->StatusBar), _(" Status: Marking all mails as read..."));
  while (gtk_events_pending ())
    gtk_main_iteration ();
  mysql_query (&mysql, cmd);
  while (gtk_events_pending ())
    gtk_main_iteration ();

  mysql_close (&mysql);
  g_free (cmd);
  g_free (folder);

  unread = 0;

  fill_message_list (w, w->last_selected_folder);

  gnome_appbar_pop (GNOME_APPBAR (w->StatusBar));

  return;
}

/*
    cb_mark_all_unread - Mark all mails as unread

	INPUTS:
			*widget			-	unused
			*data			-	unused
	RETURNS:
*/
void cb_mark_all_unread (GtkWidget * widget, gpointer * data)
{
  gchar *folder, *tmp;
  MYSQL mysql;
  gchar *cmd;

  folder = g_strdup (w->last_selected_folder);
  tmp = strstr (folder, " (");
  if (tmp)
    *tmp = '\0';

  cmd = g_strconcat ("UPDATE mail SET mread = 0 WHERE mfolder = '", folder, "'", NULL);

  mysql = sql_connect ("treehouse");

  if (mysql_errno (&mysql)) {
    display_error (_("Unable to establish database connection"));
    g_free (cmd);
    g_free (folder);
    return;
  }

  gnome_appbar_push (GNOME_APPBAR (w->StatusBar), _(" Status: Marking all mails as unread..."));
  while (gtk_events_pending ())
    gtk_main_iteration ();
  mysql_query (&mysql, cmd);
  while (gtk_events_pending ())
    gtk_main_iteration ();

  mysql_close (&mysql);
  g_free (cmd);
  g_free (folder);

  fill_message_list (w, w->last_selected_folder);

  gnome_appbar_pop (GNOME_APPBAR (w->StatusBar));

  return;
}

void cb_move_to_folder (GtkWidget * widget, gpointer * data)
{
  struct MainWindow *move;
  GtkWidget *dlg;
  GtkWidget *Root;
  gint i = 0, num_sel = 0, j = 0, row = 0, clist_rows = 0;
  GList *selection, *mails;
  GtkLabel *label;
  GtkWidget *item;
  gchar *folder = NULL;
  MYSQL mysql;
  gchar *cmd = NULL, *mId = NULL, *tmp = NULL;
  float j_f, num_sel_f;

  mails = GTK_CLIST (w->MessageList)->selection;
  if (!mails)
    return;

  move = g_malloc (sizeof (*move));

  move->FolderTree = gtk_tree_new ();
  dlg = gnome_dialog_new (_("Move to Folder..."), GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_CANCEL, NULL);
  Root = gtk_frame_new (NULL);

  gtk_container_add (GTK_CONTAINER (Root), GTK_WIDGET (move->FolderTree));
  gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dlg)->vbox), GTK_WIDGET (Root), TRUE, TRUE, 0);
  gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);

  fill_folder_tree (move);

  gtk_widget_show_all (dlg);

  i = gnome_dialog_run (GNOME_DIALOG (dlg));

  if (i == 0) {			/* User pressed "Ok" */
    gnome_appbar_push (GNOME_APPBAR (w->StatusBar), _(" Status: Moving mails..."));
    selection = GTK_TREE (move->FolderTree)->selection;
    if (!selection) {
      gnome_dialog_close (GNOME_DIALOG (dlg));
      g_free (move);
      return;
    }

    item = GTK_WIDGET (selection->data);
    label = GTK_LABEL (GTK_BIN (item)->child);
    gtk_label_get (label, &folder);

    tmp = strstr (folder, " (");
    if (tmp)
      *tmp = '\0';

    mysql = sql_connect ("treehouse");

    num_sel = g_list_length (mails);
    num_sel_f = num_sel;

    for (j = 0; j < num_sel; j++) {
      j_f = j + 1;
      gnome_appbar_set_progress (GNOME_APPBAR (w->StatusBar), (j_f / num_sel_f));
      while (gtk_events_pending ())
	gtk_main_iteration ();

      if (!mails)
	break;

      row = GPOINTER_TO_INT (mails->data);
      gtk_clist_get_text (GTK_CLIST (w->MessageList), row, 4, &mId);

      cmd = g_strconcat ("UPDATE mail SET mfolder = '", folder, "' WHERE mId = '", mId, "'", NULL);
      printf ("Query: '%s'\n", cmd);

      dpop3_log_add ("DMMOVE", g_strconcat (mId, " ", dpop3_get_foldid (folder), NULL));
      mysql_query (&mysql, cmd);

      if (mysql_errno (&mysql)) {
	display_error (_("SQL query failed"));
	break;
      }

      g_free (cmd);

      mails = mails->next;
    }

    mails = g_list_first (mails);

    mysql_close (&mysql);
    gnome_appbar_pop (GNOME_APPBAR (w->StatusBar));
    gnome_appbar_set_progress (GNOME_APPBAR (w->StatusBar), 0);

    fill_folder_tree (w);
    fill_message_list (w, w->last_selected_folder);

    if (row > 0)
      row--;

    clist_rows = GTK_CLIST (w->MessageList)->rows;

    if (GTK_CLIST (w->MessageList)->rows <= row) {
      row = clist_rows - 1;
    }

    gtk_clist_select_row (GTK_CLIST (w->MessageList), row, 0);

    if (gtk_clist_row_is_visible (GTK_CLIST (w->MessageList), row) != GTK_VISIBILITY_FULL) {
      gtk_clist_moveto (GTK_CLIST (w->MessageList), row, 0, 1.0, 0.0);
      if (row >= 0) {
	cb_show_message (w->MessageList, row, 0, NULL, NULL);
      } else {
	msg_pane_set_html ("<html><body bgcolor=\"#FFFFFF\">&nbsp;</body></html>");
	gnome_less_clear (GNOME_LESS (w->SourcePane));
      }
    }
  }

  gnome_dialog_close (GNOME_DIALOG (dlg));
  g_free (move);

  return;
}

void dpop3_dquery (char *query)
{
  MYSQL mysql;

  mysql = sql_connect ("treehouse");
  if (mysql_errno (&mysql)) {
    display_error (mysql_error (&mysql));
    return;
  }

  printf ("Query: '%s'\n", query);
  mysql_query (&mysql, query);

  mysql_close (&mysql);
}

void dpop3_dmmove (char *arg)
{
  char *tmp;
  char *query;

  tmp = strchr (arg, ' ');
  *tmp = '\0';
  tmp++;

  query = g_strdup_printf ("UPDATE mail SET mfolder = '%s' WHERE mId = '%s'", dpop3_get_foldname (arg), tmp);
  
  dpop3_dquery (query);

  free (query);

  return;
}

void dpop3_dmdele (char *arg)
{
  char *query;

  query = g_strdup_printf ("DELETE FROM mail WHERE mId = '%s'", arg);

  dpop3_dquery (query);

  free (query);

  return;
}

void dpop3_dfnew (char *arg)
{
  char *tmp;
  char *tmp2;
  char *query;

  tmp = strchr (arg, ' ');
  *tmp = '\0';
  tmp++;

  tmp2 = strchr (tmp, ' ');
  *tmp2 = '\0';
  tmp2++;

  query = g_strdup_printf ("INSERT INTO folders (mId, mname, mparent) VALUES ('%s'. '%s', '%s')", arg, tmp, dpop3_get_foldid (tmp2));

  dpop3_dquery (query);

  free (query);

  return;
}

void dpop3_dfmove (char *arg)
{
  char *query;
  char *tmp;

  tmp = strchr (arg, ' ');
  *tmp = '\0';
  tmp++;

  query = g_strdup_printf ("UPDATE folders SET mparent = '%s' WHERE mId = '%s'", dpop3_get_foldname (tmp), arg);

  dpop3_dquery (query);

  free (query);

  return;
}

void dpop3_dfdele (char *arg)
{
  char *query;

  query = g_strdup_printf ("DELETE FROM folders WHERE mId = '%s'", arg);

  dpop3_dquery (query);

  free (query);

  return;
}

void dpop3_dfrene (char *arg)
{
  char *query;
  char *tmp;

  tmp = strchr (arg, ' ');
  *tmp = '\0';
  tmp++;

  query = g_strdup_printf ("UPDATE folders SET mname = '%s' WHERE mId = '%s'", tmp, arg);

  dpop3_dquery (query);

  free (query);

  return;
}

void dpop3_dsync (GIOChannel *io)
{
  MYSQL mysql;
  MYSQL_RES *res;
  MYSQL_ROW row;
  char *tmp;
  char *query;
  int rows, i;
  buf_info *buffer;

  buffer = g_malloc (sizeof (buf_info));
  buffer->buf = g_malloc (MAX_SERVER_BUF);
  buffer->size = MAX_SERVER_BUF;

  query = g_strconcat ("SELECT sTimestamp,sCmd,sArgs FROM synclog ORDER BY sTimestamp", NULL);

  printf ("Query: '%s'\n", query);

  mysql = sql_connect ("treehouse");
  if (mysql_errno (&mysql)) {
    display_error (mysql_error (&mysql));
    g_free (buffer->buf);
    g_free (buffer);
    return;
  }

  mysql_query (&mysql, query);
  g_free (query);

  res = mysql_store_result (&mysql);
  rows = mysql_num_rows (res);

  for (i = 0; i < rows; i++) {
    row = mysql_fetch_row (res);
    tmp = g_strdup_printf ("%s %s\r\n", row[1], row[2]);
    printf ("  Writing line: '%s'\n", tmp);
    if (!(strncmp (row[1], "DMPUT", 5))) {
      tcp_read_write (io, buffer, tmp, TCP_WRITE);
      dpop3_dmput (io, row[2]);
    } else {
      tcp_read_write (io, buffer, tmp, TCP_BOTH);
    }
    if (buffer->buf[0] == '-') {
      display_error ("DPOP3 server rejected a command, check console");
      printf ("Buffer: '%s'\n", buffer->buf);
      g_free (tmp);
      continue;
    }
    query = g_strdup_printf ("DELETE FROM synclog WHERE sTimestamp = '%s'", row[0]);
    mysql_query (&mysql, query);
    g_free (query);
    g_free (tmp);
  }

  mysql_free_result (res);
  mysql_close (&mysql);

  dpop3_set_dsync (" ");
  
  return;
}

void dpop3_dmretr (GIOChannel *io, char *arg)
{
  buf_info *buffer;
  buf_info *mailbuf;
  char *cmd;
  char *tmp;
  char *msgid;

  mailbuf = g_malloc (sizeof (buf_info));
  buffer = g_malloc (sizeof (buf_info));
  buffer->buf = g_malloc (MAX_SERVER_BUF);
  buffer->size = MAX_SERVER_BUF;

  msgid = strdup (arg);
  tmp = strchr (msgid, ' ');
  if (tmp) {
    *tmp = '\0';
    tmp++;
  }

  if (dpop3_msg_exists (msgid) != 0) {
    g_free (buffer->buf);
    g_free (buffer);
    g_free (mailbuf);
    g_free (msgid);
    return;
  }

  cmd = g_strconcat ("DMSIZE", msgid, NULL);

  tcp_read_write (io, buffer, cmd, TCP_BOTH);

  g_free (cmd);

  if (buffer->buf[0] == '-') {
    display_error ("server rejected dmsize");
    return;
  }

  tmp = strchr (buffer->buf, ' ');
  if (tmp) {
    tmp++;
    cmd = strstr (tmp, "\r\n");
    if (!cmd) {
      display_error ("dmsize server message was garbled");
      display_error (buffer->buf);
      return;
    }
    *cmd = '\0';
  } else {
    display_error ("dmsize server message was garbled");
    display_error (buffer->buf);
    return;
  }

  mailbuf->buf = g_malloc (atoi (tmp) + 50);
  mailbuf->size = atoi (tmp) + 50;

  cmd = g_strconcat ("DMRETR", arg, NULL);
  tcp_read_write (io, mailbuf, cmd, TCP_BOTH);
  printf ("Sent: DMRETR %s\nGot: '%s'\n", arg, buffer->buf);

  g_free (buffer->buf);
  g_free (buffer);
  
  return;
}

void dpop3_dmput (GIOChannel *io, char *arg)
{
  char *tmp;
  char *body;
  char *bfile_txt;
  char *hfile_txt;
  gint bfile;
  buf_info *buffer;

  buffer = g_malloc (sizeof (buf_info));
  buffer->buf = g_malloc (MAX_SERVER_BUF);
  buffer->size = MAX_SERVER_BUF;

  printf ("About to DMPUT '%s'\n", arg);

  tmp = strchr (arg, ' ');
  *tmp = '\0';
  tmp++;

  bfile = open_file (arg, "b");
  bfile_txt = read_file (bfile);
  close (bfile);
  bfile = open_file (arg, "h");
  hfile_txt = read_file (bfile);
  close (bfile);

  body = g_strconcat (hfile_txt, "\n", bfile_txt, NULL);

  printf ("Prepared a mail of length '%d'\n", strlen (body));

  tcp_read_write (io, buffer, body, TCP_BOTH);

  printf ("Sent: '%s'\nGot: '%s'\n", body, buffer->buf);

  g_free (bfile_txt);
  g_free (hfile_txt);
  g_free (buffer->buf);
  g_free (buffer);
  g_free (body);

  return;
}

