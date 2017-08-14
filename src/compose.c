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

    compose.c

    Composing new mails and replying/forwarding related functions
    $Header: /cvsroot/treehouse/treehouse/src/compose.c,v 1.7 2002/05/21 07:12:43 chrisjones Exp $
*/

/* Includes */
#include "header.h"

/*
   create_composewin - Create a window for composing a new mail
  
  	INPUTS:

  	RETURNS:
*/
void create_composewin (gchar * mId, gboolean forward)
{
  struct new_mail *cw;
  gchar *title, *name;
  GtkStyle *style;
  gchar *mail = NULL, *query = NULL, *quoted_text = NULL;
  MYSQL mysql;
  MYSQL_RES *res;
  MYSQL_ROW row;
  gchar *subject;
  gchar *reply_to = NULL;
  gchar *mname, *addr;

  gint sig_fp, bytes = 0;
  gchar *sig_file;
  gchar *sig_txt;
  struct stat *sig_inf;

  gint hfp, bfp;
  gchar *hfile_txt = NULL, *bfile_txt = NULL;


  /* Menus and toolbar */
  GnomeUIInfo comp_toolbar[] = {
    { GNOME_APP_UI_ITEM, 
      N_("Send"), 
      N_("Send this mail"), 
      (gpointer) cb_send_clicked,
      NULL, 
      NULL, 
      GNOME_APP_PIXMAP_STOCK, 
      GNOME_STOCK_PIXMAP_MAIL_RCV, 
      (guint) 0, 
      (GdkModifierType) 0, 
      NULL },
    GNOMEUIINFO_SEPARATOR, 
    { GNOME_APP_UI_ITEM, 
      N_("Save As"), 
      N_("Save the message text to a file"), 
      (gpointer) cb_save_as, 
      NULL,
      NULL, 
      GNOME_APP_PIXMAP_STOCK, 
      GNOME_STOCK_PIXMAP_SAVE_AS, 
      0, 
      (GdkModifierType) 0, 
      NULL },
    GNOMEUIINFO_END
  };

  cw = g_malloc (sizeof (struct new_mail));
  comp_toolbar[0].user_data = cw;
  comp_toolbar[2].user_data = cw;
  title = g_malloc (15);
  g_snprintf (title, 15, _("Compose (%d)"), g_list_length (compwins));
  name = g_malloc (10);
  g_snprintf (name, 10, "bmcomp%d", g_list_length (compwins));

  cw->CompWin = gnome_app_new (name, title);
  cw->RootVBox = gtk_vbox_new (FALSE, 0);
  cw->ToHBox = gtk_hbox_new (FALSE, 0);
  cw->CcHBox = gtk_hbox_new (FALSE, 0);
  cw->SubjHBox = gtk_hbox_new (FALSE, 0);

  cw->ToBut = gtk_button_new_with_label (_("  To:  "));
  cw->CcBut = gtk_button_new_with_label (_("  Cc:  "));
  cw->SubjLabel = gtk_label_new (_(" Subject: "));

  cw->ToStr = gnome_entry_new ("comp_to");
  cw->CcStr = gnome_entry_new ("comp_cc");
  cw->SubjStr = gnome_entry_new ("comp_subj");

  gtk_box_pack_start (GTK_BOX (cw->ToHBox), GTK_WIDGET (cw->ToBut), FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (cw->ToHBox), GTK_WIDGET (cw->ToStr), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (cw->CcHBox), GTK_WIDGET (cw->CcBut), FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (cw->CcHBox), GTK_WIDGET (cw->CcStr), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (cw->SubjHBox), GTK_WIDGET (cw->SubjLabel), FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (cw->SubjHBox), GTK_WIDGET (cw->SubjStr), TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (cw->RootVBox), GTK_WIDGET (cw->ToHBox), FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (cw->RootVBox), GTK_WIDGET (cw->CcHBox), FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (cw->RootVBox), GTK_WIDGET (cw->SubjHBox), FALSE, FALSE, 0);

  cw->TextHBox = gtk_hbox_new (FALSE, 0);
  cw->Text = gtk_text_new (NULL, NULL);
  cw->VAdj = gtk_vscrollbar_new (GTK_TEXT (cw->Text)->vadj);

  gtk_text_set_editable (GTK_TEXT (cw->Text), TRUE);
  gtk_text_set_word_wrap (GTK_TEXT (cw->Text), TRUE);
  style = gtk_style_new ();
  gdk_font_unref (style->font);
  style->font = gdk_font_load (bm_cfg->normal_font);
  gtk_widget_set_style (GTK_WIDGET (cw->Text), style);

  gtk_box_pack_start (GTK_BOX (cw->TextHBox), GTK_WIDGET (cw->Text), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (cw->TextHBox), GTK_WIDGET (cw->VAdj), FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (cw->RootVBox), GTK_WIDGET (cw->TextHBox), TRUE, TRUE, 0);

  gnome_app_set_contents (GNOME_APP (cw->CompWin), cw->RootVBox);
  gnome_app_create_toolbar (GNOME_APP (cw->CompWin), comp_toolbar);

  gtk_signal_connect (GTK_OBJECT (cw->CompWin), "destroy", GTK_SIGNAL_FUNC (comp_quit), cw->CompWin);

  gtk_window_set_wmclass (GTK_WINDOW (cw->CompWin), "treehousecomp", "TreehouseComp");

  compwins = g_list_append (compwins, cw->CompWin);

  if (mId) {
    query = g_strconcat ("SELECT mfrom,msubject FROM mail WHERE mId = '", mId, "'", NULL);

    mysql = sql_connect ("treehouse");
    if (mysql_errno (&mysql)) {
      display_error (_("Unable to establish database connection"));
      compwins = g_list_remove (compwins, cw->CompWin);
      return;
    }

    mysql_query (&mysql, query);
    res = mysql_store_result (&mysql);
    row = mysql_fetch_row (res);

    hfp = open_file (mId, "h");
    hfile_txt = read_file (hfp);
    close (hfp);
    bfp = open_file (mId, "b");
    bfile_txt = read_file (bfp);
    close (bfp);

    quoted_text = g_malloc (strlen (bfile_txt) * 2);
    quote_text (quoted_text, bfile_txt);
    mail = g_strconcat ("Hi\n\n", row[0], _(" wrote:\n> \n> "), quoted_text, "\n\n", NULL);

    if (g_strncasecmp (row[1], "re:", 3)) {
      subject = g_strconcat (_("Re: "), row[1], NULL);
    } else {
      subject = g_strdup (row[1]);
    }

    gtk_entry_set_text (GTK_ENTRY (gnome_entry_gtk_entry (GNOME_ENTRY (cw->SubjStr))), subject);

    if (!forward) {
      if (extract_header (&reply_to, hfile_txt, "\r\nReply-To:")) {
        /* We didn't find a Reply-To: header, so use the From: one */
	reply_to = g_strdup (row[0]);
      }
      parse_address (&mname, &addr, reply_to);
      gentry_set (cw->ToStr, addr);
      g_free (mname);
      g_free (addr);
      g_free (reply_to);
    }

    gtk_text_insert (GTK_TEXT (cw->Text), NULL, NULL, NULL, mail, -1);

    g_free (subject);
    g_free (quoted_text);
    g_free (mail);

    mysql_free_result (res);
    mysql_close (&mysql);
  }

  /* FIXME: Replace this with a set of configurable .sig options */
  if (getenv ("HOME")) {
    sig_file = g_strconcat (getenv ("HOME"), "/.signature", NULL);
    sig_fp = open (sig_file, O_RDONLY);
    if (sig_fp != -1) {
      sig_inf = g_malloc (sizeof (*sig_inf) + 1);
      fstat (sig_fp, sig_inf);
      sig_txt = g_malloc ((int)sig_inf->st_size + 1);

      while (bytes != -1) {
        bytes += read (sig_fp, sig_txt + bytes, sig_inf->st_size - bytes);
        if (bytes == sig_inf->st_size)
          break;
      }
      if (bytes != -1) {
        sig_txt[sig_inf->st_size] = '\0';

        gtk_text_insert (GTK_TEXT (cw->Text), NULL, NULL, NULL, " \n-- \n", -1);
        gtk_text_insert (GTK_TEXT (cw->Text), NULL, NULL, NULL, sig_txt, (int)sig_inf->st_size);
      }
      g_free (sig_txt);
      g_free (sig_inf);
      close (sig_fp);
    }
    g_free (sig_file);
  }

  gtk_widget_show_all (cw->CompWin);
  gtk_main ();

  g_free (title);
  g_free (name);
  g_free (cw);
  if (hfile_txt) g_free (hfile_txt);
  if (bfile_txt) g_free (bfile_txt);

  return;
}

/*
   comp_quit - Close a compose window
  
  	INPUTS:

  	RETURNS:
*/
void comp_quit (GtkWidget * widget, GtkWidget * CompWin)
{
  compwins = g_list_remove (compwins, CompWin);
  gtk_main_quit ();
}

/*
   cb_send_clicked - Queue a mail for delivery

	INPUTS:

	RETURNS:
*/
void cb_send_clicked (GtkWidget * widget, struct new_mail *cw)
{
  MYSQL mysql;
  gchar *cmd;
  MYSQL_RES *res;
  MYSQL_ROW msg_list_row;
  gint num_rows = 0;

  gchar *cc, *to, *subject, *body, *from, *replyto, *org, *key;
  gchar *mcc, *mto, *msubject, *mfrom, *mreplyto, *morg, *mheaders;

  time_t epoch;
  gchar epochtime[1024];
  gint bfp, hfp;

  cmd = g_strconcat ("SELECT * FROM accounts WHERE def_acc = '1'", NULL);

  /* Connect to the mySQL database */
  mysql = sql_connect ("treehouse");

  if (mysql_errno (&mysql))
    return;

  if (mysql_real_query (&mysql, cmd, strlen (cmd))) {
    g_print ("'%s'\n", (char *) mysql_error (&mysql));
    display_error (_("SQL Query failed"));
    mysql_close (&mysql);
    return;
  }
  res = mysql_store_result (&mysql);
  num_rows = (int) mysql_num_rows (res);

  if ((num_rows == 0) || (num_rows > 1)) {
    display_error (_("You must set one of your mail accounts as default\nPlease set a default account with the following dialog..."));
    cb_accounts_dialog_new (NULL, NULL);
    mysql_close (&mysql);
    g_free (cmd);
    mysql_free_result (res);
    return;
  }

  msg_list_row = mysql_fetch_row (res);

  g_free (cmd);

  to = gentry_get (cw->ToStr);

  if (!g_strcasecmp (to, "")) {
    display_error (_("You have not specified an email address\n"));
    return;
  }

  cc = gentry_get (cw->CcStr);
  subject = gentry_get (cw->SubjStr);

  body = gtk_editable_get_chars (GTK_EDITABLE (cw->Text), 0, -1);
  from = g_strconcat (msg_list_row[2], " <", msg_list_row[3], ">", NULL);
  replyto = g_strdup (msg_list_row[5]);
  org = g_strdup (msg_list_row[4]);
  key = g_strdup (msg_list_row[1]);

  /* Wrap the body text */
  col_wrap (&body, bm_cfg->wrap_col);

  /* Allow space for the mysql escaping */
  mcc = g_malloc ((strlen (cc) * 2) + 1);
  mto = g_malloc ((strlen (to) * 2) + 1);
  msubject = g_malloc ((strlen (subject) * 2) + 1);
  mfrom = g_malloc ((strlen (from) * 2) + 1);
  mreplyto = g_malloc ((strlen (replyto) * 2) + 1);
  morg = g_malloc ((strlen (org) * 2) + 1);

  mysql_escape_string (mcc, cc, (unsigned int) strlen (cc));
  mysql_escape_string (mto, to, (unsigned int) strlen (to));
  mysql_escape_string (msubject, subject, (unsigned int) strlen (subject));
  mysql_escape_string (mfrom, from, (unsigned int) strlen (from));
  mysql_escape_string (mreplyto, replyto, (unsigned int) strlen (replyto));
  mysql_escape_string (morg, org, (unsigned int) strlen (org));

  /* FIXME: Add more randomness to the time() ? */
  epoch = time (NULL);
  sprintf (epochtime, "%d", (int)epoch);

  cmd = g_strconcat ("INSERT INTO mail (mfrom, mto, msubject, mfolder, mread, mId, mreceived, maccount) VALUES ('",
     mfrom, "','", mto, "','", msubject,
     "','", "OutBox", "','", "0", "',", epochtime, ",",
     "NOW()", ", '", key, "')\0", NULL);

  mheaders = g_strconcat ("\nCc: ", mcc, "\r\n\r\n", NULL);

  bfp = open_filew (epochtime, "b");
  write_file (bfp, body);
  close (bfp);
  hfp = open_filew (epochtime, "h");
  write_file (bfp, mheaders);
  close (hfp);

  sprintf (epochtime, "%d %d", (int)epoch, strlen (mheaders) + strlen (body) + 1);
  dpop3_log_add ("DMPUT", epochtime);

  g_free (mheaders);

  /* Connect to the mySQL database */
  mysql_close (&mysql);
  mysql = sql_connect ("treehouse");

  if (mysql_errno (&mysql)) {
    return;
  }

  if (mysql_real_query (&mysql, cmd, strlen (cmd))) {
    g_print ("'%s'\n", (char *) mysql_error (&mysql));
    display_error (_("SQL Query failed"));
    mysql_close (&mysql);
    return;
  }

  g_free (to);
  g_free (cc);
  g_free (subject);
  g_free (from);
  g_free (mto);
  g_free (mcc);
  g_free (msubject);
  g_free (mfrom);
  g_free (cmd);
  mysql_close (&mysql);

  gtk_object_destroy (GTK_OBJECT (cw->CompWin));
  gtk_main_quit ();

  /* Refresh the Outbox if it is selected */
  if (!(g_strcasecmp (w->last_selected_folder, "Outbox"))) {
    fill_message_list (w, w->last_selected_folder);
  }

  return;
}

/*
   cb_save_as - Save an unsent email as text to a file

	INPUTS:
			*widget		-	Unused
			*cw		-	Dialog to use
	RETURNS:
*/
void cb_save_as (GtkWidget * widget, struct new_mail *cw)
{
  gchar *text, *file;
  GtkWidget *dlg;
  GtkWidget *file_entry;
  gint i = 0;
  int fp;

  text = g_strdup (gtk_editable_get_chars (GTK_EDITABLE (cw->Text), 0, -1));

  dlg = gnome_dialog_new (_("Save message as..."), GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_CANCEL, NULL);
  file_entry = gnome_file_entry_new ("bm_compose_save_as", _("Choose file to save message as..."));

  gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dlg)->vbox), GTK_WIDGET (file_entry), TRUE, TRUE, 0);
  gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);

  gtk_widget_show_all (dlg);

  i = gnome_dialog_run (GNOME_DIALOG (dlg));

  if (i != 0) {
    /* User didn't click "Ok", so bail */
    gnome_dialog_close (GNOME_DIALOG (dlg));
    g_free (text);
    return;
  }

  file = gentry_get (file_entry);

  fp = open (file, O_WRONLY | O_CREAT);

  if (fp == -1) {
    display_error (_("Unable to open file for writing"));
    g_free (file);
    g_free (text);
    gnome_dialog_close (GNOME_DIALOG (dlg));
    return;
  }

  while (i != strlen (text)) {
    i += write (fp, text + i, strlen (text));
    if (i == -1) {
      display_error (_("Writing failed"));
      break;
    }
  }

  g_free (file);
  g_free (text);
  close (fp);
  gnome_dialog_close (GNOME_DIALOG (dlg));

  return;
}

/*
   cb_compose - Prepare to compose a new mail and call the appropriate function

	INPUTS:
	RETURNS:
*/
void cb_compose (void)
{
  create_composewin (NULL, FALSE);
  return;
}

void cb_reply (void)
{
  gchar *mId;
  gint row;
  GList *selection;

  selection = GTK_CLIST (w->MessageList)->selection;

  /* Check if there is a multiple selection - if so, we don't want to do anything */
  if (g_list_length (selection) != 1)
    return;

  row = GPOINTER_TO_INT (selection->data);
  gtk_clist_get_text (GTK_CLIST (w->MessageList), row, 4, &mId);

  if (mId == NULL || mId == '\0') {
    display_error (_("Error: Unable to locate UIDL in entry"));
    return;
  }

  /* Check to see if we are being asked to display the "No messages" placeholder */
  if (g_strcasecmp ((char *) mId, "<NONE>") == 0) {
    return;
  }

  create_composewin (mId, FALSE);

  return;
}

void cb_forward (void)
{
  gchar *mId;
  gint row;
  GList *selection;

  selection = GTK_CLIST (w->MessageList)->selection;

  /* Check if there is a multiple selection - if so, we don't want to do anything */
  if (g_list_length (selection) != 1)
    return;

  row = GPOINTER_TO_INT (selection->data);
  gtk_clist_get_text (GTK_CLIST (w->MessageList), row, 4, &mId);

  if (mId == NULL || mId == '\0') {
    display_error (_("Error: Unable to locate UIDL in entry"));
    return;
  }

  /* Check to see if we are being asked to display the "No messages" placeholder */
  if (g_strcasecmp ((char *) mId, "<NONE>") == 0) {
    return;
  }

  create_composewin (mId, TRUE);

  return;
}

/*
   quote_text - Re-write a string with "> " at the beginning of each line

	INPUTS:
			*quoted_text	-	Pre-malloc'd storage for the quoted string
			*text		-	String to quote
	RETURNS:
*/
void quote_text (gchar * quoted_text, gchar * text)
{
  gchar *src = text;
  gchar *dst = quoted_text;
  *quoted_text = 0;

  while (*src) {
    switch (*src) {
    case '\n':
      sprintf(dst, "\n%1s ", bm_cfg->quote_char);
      dst += 3;
      break;

    case '\r':
      *dst++ = ' ';
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

void col_wrap (gchar ** body, gint cols)
{
  gint num_rows = 0;		/* Maximum possible number of rows AFTER wrapping */
  gint i = 0, src_len;
  gchar *src, *dst;
  gchar *tmp;

  src_len = strlen (*body);
  num_rows = src_len / cols;

  /* This is wildly inefficient, but it covers the worst case scenario  */
  /*  of having to wrap after 1 character (obviously a dumb thing to do */
  /*  but it does work). A realloc() should really appear somewhere.    */
  tmp = g_malloc0 ((src_len * 2) + (num_rows * 2) + 1);

  src = *body;
  dst = tmp;

  /* This is probably a very bad thing to do, we shouldn't just be strcat()ing to strings we didn't make */
  /* So make sure every string that you pass to this function is null terminated already! */
  if (src[src_len - 1] != '\0')
    strcat (src, "\0");

  while (*src) {
    if ((*(src - 1) == '\n') && (*src == '>')) {
      /* Quoted text. Copy the rest of the line without wordbreaking */
      while ((*src != '\n') && (*src)) {
	*dst++ = *src++;
      }
      continue;
    }

    if (i == cols) {
      while ((*src != ' ') && (i > 0) && (*src) && (*src != '\n')) {
	src--;
	dst--;
	i--;
      }

      if (i == 0) {
	while ((*src != '\n') && (*src)) {
	  *dst++ = *src++;
	}
	continue;
      }

      *dst++ = '\n';
      src++;
      i = 0;
      continue;
    }

    *dst++ = *src;
    i++;
    if (*src == '\n') {
      i = 0;
    }
    src++;
  }

  g_free (*body);
  *body = g_strdup (tmp);

  return;
}

