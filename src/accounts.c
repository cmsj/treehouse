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

    accounts.c

    Functions related to accounts management
    $Header: /cvsroot/treehouse/treehouse/src/accounts.c,v 1.5 2002/05/16 00:30:11 chrisjones Exp $
*/

/* Includes */
#include "header.h"

gchar *accounts_list_titles[] = {
  N_("Name:"),
  N_("Email Address:"),
  N_("Autonumber"),
  N_("Default:")
};

/*
    accounts_dialog_new - Display a list of mail accounts

	INPUTS:
			
	RETURNS:
			1	-	Success
			0	-	Failure
*/
gint cb_accounts_dialog_new (GtkWidget * widget, gpointer data)
{
  struct account_dialog *acc;

  acc = g_malloc (sizeof (*acc));

  acc->dlg = glade_xml_get_widget (xml, "AccountsWin");
  acc->rootpaned_left = glade_xml_get_widget (xml, "rootpaned_left");

  gtk_clist_set_column_auto_resize (GTK_CLIST (acc->rootpaned_left), 0, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (acc->rootpaned_left), 1, TRUE);
  gtk_clist_set_column_visibility (GTK_CLIST (acc->rootpaned_left), 2, FALSE);

  fill_accounts_list (acc);

  gtk_widget_show_all (acc->dlg);

  g_free (acc);

  return (1);
}

/*
    accounts_dialog_add_new - Add a new mail accounts

	INPUTS:
			
	RETURNS:
			1	-	Success
			0	-	Failure
*/
gint cb_accounts_dialog_add_new (GtkWidget * widget, gpointer data)
{
  GtkWidget *dlg = glade_xml_get_widget (xml, "AccountsNewWin");
  GtkWidget *acc_name = glade_xml_get_widget (xml, "acc_name");
  GtkWidget *name = glade_xml_get_widget (xml, "name");
  GtkWidget *email = glade_xml_get_widget (xml, "email");
  GtkWidget *reply = glade_xml_get_widget (xml, "reply");
  GtkWidget *org = glade_xml_get_widget (xml, "org");
  GtkWidget *pop3server = glade_xml_get_widget (xml, "pop3server");
  GtkWidget *pop3port = glade_xml_get_widget (xml, "pop3port");
  GtkWidget *pop3user = glade_xml_get_widget (xml, "pop3user");
  GtkWidget *pop3pass = glade_xml_get_widget (xml, "pop3pass");
  GtkWidget *smtpserver = glade_xml_get_widget (xml, "smtpserver");
  GtkWidget *smtpport = glade_xml_get_widget (xml, "smtpport");
  GtkWidget *delmail = glade_xml_get_widget (xml, "delmail");
  GtkWidget *checkdef = glade_xml_get_widget (xml, "checkdef");
  GtkWidget *defacc = glade_xml_get_widget (xml, "defacc");
  gint i = 0;
  gboolean flag = FALSE;
  gchar *query = g_malloc (8192);
  MYSQL mysql;
  gchar *acctxt, *nametxt, *emailtxt, *orgtxt, *replytxt,
	*p3servertxt, *p3porttxt, *p3usertxt, *p3passtxt, *smtptxt,
	*smtpporttxt;


  gtk_widget_show_all (dlg);

  for (;;) {
    i = gnome_dialog_run (GNOME_DIALOG (dlg));

    flag = FALSE;
    if (i == 0) {		/* Ok pressed */
      /* Get info out of the GUI */
      acctxt = gentry_get (acc_name);
      nametxt = gentry_get (name);
      emailtxt = gentry_get (email);
      replytxt = gentry_get (reply);
      orgtxt = gentry_get (org);
      p3servertxt = gentry_get (pop3server);
      p3porttxt = gentry_get (pop3port);
      p3usertxt = gentry_get (pop3user);
      p3passtxt = gentry_get (pop3pass);
      smtptxt = gentry_get (smtpserver);
      smtpporttxt = gentry_get (smtpport);

      if (!strlen (acctxt) || !strlen (nametxt) || !strlen (emailtxt) || !strlen (p3servertxt) || !strlen (p3porttxt) || !strlen (p3usertxt) || !strlen (p3passtxt) || !strlen (smtptxt) || !strlen (smtpporttxt)) {
	display_error (_("You have not filled in all required fields"));
	flag = TRUE;
      }

      if (!flag) {
	/* Connect to the mySQL database */
	mysql = sql_connect ("treehouse");

	if (mysql_errno (&mysql)) {
	  g_free (acctxt);
	  g_free (nametxt);
	  g_free (emailtxt);
	  g_free (replytxt);
	  g_free (orgtxt);
	  g_free (p3servertxt);
	  g_free (p3porttxt);
	  g_free (p3usertxt);
	  g_free (p3passtxt);
	  g_free (smtptxt);
	  g_free (smtpporttxt);
	  g_free (query);
	  return (0);
	}

        if (GTK_TOGGLE_BUTTON (defacc)->active) {
          query = g_strdup ("UPDATE accounts SET def_acc = 0");
          if (mysql_real_query (&mysql, query, strlen (query))) {
            g_print ("'%s'\n", (char *)mysql_error (&mysql));
            display_error (_("SQL Query failed"));
            flag = FALSE;
          }
          g_free (query);
        } 

	g_snprintf (query, 8192,
		    "INSERT INTO accounts (label, name, email, org, replyto, pop3, pop3port, smtp, smtpport, pop3user, pop3passwd, checkdef, delmail, def_acc) VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%d', '%d', '%d')",
		    acctxt, nametxt, emailtxt, orgtxt, replytxt,
		    p3servertxt, p3porttxt, smtptxt, smtpporttxt,
		    p3usertxt, p3passtxt,
		    GTK_TOGGLE_BUTTON (checkdef)->active,
		    GTK_TOGGLE_BUTTON (delmail)->active,
                    GTK_TOGGLE_BUTTON (defacc)->active);

	if (mysql_real_query (&mysql, query, strlen (query))) {
	  g_print ("'%s'\n", (char *) mysql_error (&mysql));
	  display_error (_("SQL Query failed"));
	  flag = FALSE;
	}

	mysql_close (&mysql);
      }

      if (acctxt) g_free (acctxt);
      if (nametxt) g_free (nametxt);
      if (emailtxt) g_free (emailtxt);
      if (replytxt) g_free (replytxt);
      if (orgtxt) g_free (orgtxt);
      if (p3servertxt) g_free (p3servertxt);
      if (p3porttxt) g_free (p3porttxt);
      if (p3usertxt) g_free (p3usertxt);
      if (p3passtxt) g_free (p3passtxt);
      if (smtptxt) g_free (smtptxt);
      if (smtpporttxt) g_free (smtpporttxt);
      if (query) g_free (query);

      if (flag)
	continue;

      gnome_dialog_close (GNOME_DIALOG (dlg));
      fill_accounts_list (NULL);

      return (1);
    } else if (i < 0) {		/* Closed window */
      break;
    } else if (i == 1) {	/* Cancel pressed */
      gnome_dialog_close (GNOME_DIALOG (dlg));
      break;
    }
  }

  return (1);
}

/*
   fill_accounts_list - Fill accounts dialog CList with account details

	INPUTS:
			*acc		-	Window structure
	RETURNS:
			0	-	Success
			1	-	Failure
*/
gint fill_accounts_list (struct account_dialog * acc)
{
  MYSQL mysql;
  MYSQL_RES *res;
  MYSQL_ROW msg_list_row;

  GtkWidget *clist = glade_xml_get_widget (xml, "rootpaned_left");

  gint i = 0;
  gchar *empty_acc[] = { N_("No accounts found."), "", "<none>", "" };
  gint num_rows = 0;

  mysql = sql_connect ("treehouse");

  if (mysql_errno (&mysql)) {
    display_error (_("Unable to establish database connection"));
    return (1);
  }

  mysql_query (&mysql, "SELECT label,email,number,def_acc FROM accounts ORDER BY label");

  res = mysql_store_result (&mysql);

  num_rows = (int) mysql_num_rows (res);

  gtk_clist_clear (GTK_CLIST (clist));
  gtk_clist_freeze (GTK_CLIST (clist));

  if (num_rows == 0) {
    /* Nothing found */
    gtk_clist_append (GTK_CLIST (clist), empty_acc);
  } else {
    for (i = 0; i < num_rows; i++) {
      mysql_data_seek (res, i);
      msg_list_row = mysql_fetch_row (res);

      gtk_clist_append (GTK_CLIST (clist), msg_list_row);
    }
  }

  gtk_clist_thaw (GTK_CLIST (clist));

  return (1);
}

/*
    cb_accounts_dialog_delete - Delete an account

	INPUTS:
		*widget		-	unused
		data		-	the dialog
			
	RETURNS:
*/
void cb_accounts_dialog_delete (GtkWidget * widget, struct account_dialog *data)
{
  gint row = 0;
  GList *foo;
  gchar *key;
  MYSQL mysql;
  gchar *cmd;
  GtkWidget *clist = glade_xml_get_widget (xml, "rootpaned_left");

  if (!(foo = (GTK_CLIST (clist)->selection)))
    return;

  row = GPOINTER_TO_INT (foo->data);

  gtk_clist_get_text (GTK_CLIST (clist), row, 2, &key);
  if (!g_strcasecmp ((char *) key, "<NONE>"))
    return;

  cmd = g_strconcat ("DELETE FROM accounts WHERE number = ", (char *) key, NULL);

  mysql = sql_connect ("treehouse");

  if (mysql_errno (&mysql)) {
    display_error (_("Unable to establish database connection"));
    g_free (cmd);
    return;
  }

  mysql_query (&mysql, cmd);
  if (mysql_errno (&mysql))
    display_error ((char *) mysql_error (&mysql));
  else
    fill_accounts_list (data);
  mysql_close (&mysql);

  g_free (cmd);

  return;
}

/*
    accounts_dialog_edit - Edit an existing account

	INPUTS:
			
	RETURNS:
			1	-	Success
			0	-	Failure
*/
/* FIXME: This function should be merged with accounts_dialog_add_new() */
gint cb_accounts_dialog_edit (GtkWidget * widget, struct account_dialog *data)
{
  GtkWidget *dlg = glade_xml_get_widget (xml, "AccountsNewWin");
  GtkWidget *acc_name = glade_xml_get_widget (xml, "acc_name");
  GtkWidget *name = glade_xml_get_widget (xml, "name");
  GtkWidget *email = glade_xml_get_widget (xml, "email");
  GtkWidget *reply = glade_xml_get_widget (xml, "reply");
  GtkWidget *org = glade_xml_get_widget (xml, "org");
  GtkWidget *pop3server = glade_xml_get_widget (xml, "pop3server");
  GtkWidget *pop3port = glade_xml_get_widget (xml, "pop3port");
  GtkWidget *pop3user = glade_xml_get_widget (xml, "pop3user");
  GtkWidget *pop3pass = glade_xml_get_widget (xml, "pop3pass");
  GtkWidget *smtpserver = glade_xml_get_widget (xml, "smtpserver");
  GtkWidget *smtpport = glade_xml_get_widget (xml, "smtpport");
  GtkWidget *delmail = glade_xml_get_widget (xml, "delmail");
  GtkWidget *checkdef = glade_xml_get_widget (xml, "checkdef");

  gint i = 0;
  gboolean flag = FALSE;
  gchar *query = g_malloc (8192);
  MYSQL mysql;
  GList *selection;
  gpointer *tmp;
  gchar *number;
  gint row_num;
  gchar *cmd;
  MYSQL_RES *res;
  MYSQL_ROW msg_list_row;
  gint num_rows;

  GtkWidget *clist = glade_xml_get_widget (xml, "rootpaned_left");

  selection = GTK_CLIST (clist)->selection;
  if (!selection)
    return (1);

  tmp = selection->data;
  row_num = GPOINTER_TO_INT (tmp);
  gtk_clist_get_text (GTK_CLIST (clist), row_num, 2, &number);

  cmd = g_strconcat ("SELECT * FROM accounts WHERE number = '", number, "'", NULL);

  mysql = sql_connect ("treehouse");
  if (mysql_errno (&mysql)) {
    display_error (_("Unable to establish database connection"));
    g_free (cmd);
    return (0);
  }

  mysql_query (&mysql, cmd);
  if (mysql_errno (&mysql))
    display_error ((char *) mysql_error (&mysql));

  res = mysql_store_result (&mysql);
  num_rows = (int) mysql_num_rows (res);
  if (num_rows > 1) {
    display_error (_("Grave error in the accounts table, please contact the author"));
    mysql_close (&mysql);
    g_free (cmd);
    g_free (query);
    return (0);
  }
  msg_list_row = mysql_fetch_row (res);

  /* Start creating the dialog */
  gentry_set (acc_name, msg_list_row[1]);
  gentry_set (name, msg_list_row[2]);
  gentry_set (email, msg_list_row[3]);
  gentry_set (reply, msg_list_row[5]);
  gentry_set (org, msg_list_row[4]);
  gentry_set (pop3server, msg_list_row[6]);
  gentry_set (pop3port, msg_list_row[7]);
  gentry_set (pop3user, msg_list_row[10]);
  gentry_set (pop3pass, msg_list_row[11]);
  gentry_set (smtpserver, msg_list_row[8]);
  gentry_set (smtpport, msg_list_row[9]);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (delmail), atoi (msg_list_row[14]));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkdef), atoi (msg_list_row[13]));

  gtk_widget_show_all (dlg);
  mysql_close (&mysql);

  for (;;) {
    i = gnome_dialog_run (GNOME_DIALOG (dlg));
    flag = FALSE;
    if (i == 0) {		/* Ok pressed */
      /* Get info out of the GUI */
      gchar *acctxt, *nametxt, *emailtxt, *orgtxt, *replytxt,
	*p3servertxt, *p3porttxt, *p3usertxt, *p3passtxt, *smtptxt,
	*smtpporttxt;

      acctxt = gentry_get (acc_name);
      nametxt = gentry_get (name);
      emailtxt = gentry_get (email);
      replytxt = gentry_get (reply);
      orgtxt = gentry_get (org);
      p3servertxt = gentry_get (pop3server);
      p3porttxt = gentry_get (pop3port);
      p3usertxt = gentry_get (pop3user);
      p3passtxt = gentry_get (pop3pass);
      smtptxt = gentry_get (smtpserver);
      smtpporttxt = gentry_get (smtpport);

      if ((strlen (acctxt) == 0) || (strlen (nametxt) == 0)
	  || (strlen (emailtxt) == 0) || (strlen (p3servertxt) == 0)
	  || (strlen (p3porttxt) == 0) || (strlen (p3usertxt) == 0)
	  || (strlen (p3passtxt) == 0) || (strlen (smtptxt) == 0)
	  || (strlen (smtpporttxt) == 0)) {
	display_error (_("You have not filled in all required fields"));
	flag = TRUE;
      }

      if (!flag) {
	cb_accounts_dialog_delete (NULL, data);
	g_snprintf (query, 8192,
		    "INSERT INTO accounts (label, name, email, org, replyto, pop3, pop3port, smtp, smtpport, pop3user, pop3passwd, checkdef, delmail, def_acc) VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%d', '%d', '%d')",
		    acctxt, nametxt, emailtxt, orgtxt, replytxt,
		    p3servertxt, p3porttxt, smtptxt, smtpporttxt,
		    p3usertxt, p3passtxt,
		    GTK_TOGGLE_BUTTON (checkdef)->active,
		    GTK_TOGGLE_BUTTON (delmail)->active, 0);

	/* Connect to the mySQL database */
	mysql = sql_connect ("treehouse");

	if (mysql_errno (&mysql)) {
	  g_free (acctxt);
	  g_free (nametxt);
	  g_free (emailtxt);
	  g_free (replytxt);
	  g_free (orgtxt);
	  g_free (p3servertxt);
	  g_free (p3porttxt);
	  g_free (p3usertxt);
	  g_free (p3passtxt);
	  g_free (smtptxt);
	  g_free (smtpporttxt);
	  g_free (query);
	  return (0);
	}

	if (mysql_real_query (&mysql, query, strlen (query))) {
	  g_print ("'%s'\n", (char *) mysql_error (&mysql));
	  display_error (_("SQL Query failed"));
	  flag = FALSE;
	}

	mysql_close (&mysql);
      }

      if (acctxt) g_free (acctxt);
      if (nametxt) g_free (nametxt);
      if (emailtxt) g_free (emailtxt);
      if (replytxt) g_free (replytxt);
      if (orgtxt) g_free (orgtxt);
      if (p3servertxt) g_free (p3servertxt);
      if (p3porttxt) g_free (p3porttxt);
      if (p3usertxt) g_free (p3usertxt);
      if (p3passtxt) g_free (p3passtxt);
      if (smtptxt) g_free (smtptxt);
      if (smtpporttxt) g_free (smtpporttxt);
      if (query) g_free (query);

      if (flag)
	continue;

      fill_accounts_list (data);
      gnome_dialog_close (GNOME_DIALOG (dlg));
      return (1);
    } else if (i < 0) {		/* Closed window */
      break;
    } else if (i == 1) {	/* Cancel pressed */
      gnome_dialog_close (GNOME_DIALOG (dlg));
      break;
    }
  }

  return (1);
}

/*
   cb_accounts_set_default - Designate the default mail account

	INPUTS:
			*widget		-	Unused
			*data		-	Dialog structure to use
	RETURNS:
*/
void cb_accounts_set_default (GtkWidget * widget, struct account_dialog *data)
{
  gint row = 0;
  GList *foo;
  gchar *key;
  MYSQL mysql;
  gchar *cmd;

  GtkWidget *clist = glade_xml_get_widget (xml, "rootpaned_left");

  foo = GTK_CLIST (clist)->selection;
  if (!foo)
    return;

  row = GPOINTER_TO_INT (foo->data);

  gtk_clist_get_text (GTK_CLIST (clist), row, 2, &key);
  if (!g_strcasecmp ((char *) key, "<NONE>"))
    return;

  cmd = g_strconcat ("UPDATE accounts SET def_acc = ''", NULL);

  mysql = sql_connect ("treehouse");

  if (mysql_errno (&mysql)) {
    display_error (_("Unable to establish database connection"));
    g_free (cmd);
    return;
  }

  mysql_query (&mysql, cmd);

  if (mysql_errno (&mysql)) {
    display_error ((char *) mysql_error (&mysql));
    g_free (cmd);
    return;
  }

  g_free (cmd);
  cmd = g_strconcat ("UPDATE accounts SET def_acc = '1' WHERE number ='", key, "'", NULL);

  mysql_query (&mysql, cmd);

  if (mysql_errno (&mysql)) {
    display_error ((char *) mysql_error (&mysql));
    g_free (cmd);
    return;
  } else {
    fill_accounts_list (data);
  }

  mysql_close (&mysql);

  g_free (cmd);

  return;
}
