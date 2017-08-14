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

   sql.c

   SQL related functions
   $Header: /cvsroot/treehouse/treehouse/src/sql.c,v 1.5 2002/05/16 00:30:11 chrisjones Exp $
*/

/* Include our header file */
#include "header.h"

/*
	sql_connect - Establish a connection to the sql server

	INPUTS:
			gchar *dbase	-	The database we should connect to
	RETURNS:
			MYSQL		-	The valid, open database connection

*/
MYSQL sql_connect (gchar * dbase)
{
  MYSQL mysql;

  mysql_options (&mysql, MYSQL_OPT_COMPRESS, 0);
  mysql_init (&mysql);

  if (!(mysql_real_connect (&mysql, sql_server, sql_user, sql_passwd, "tmp_treehouse", 0, NULL, 0))) {
    display_error ((char *) mysql_error (&mysql));
    mysql_close (&mysql);
    return (mysql);
  }

  return mysql;
}

/*
	add_mail - Inserts a mail into our sql database
  
  	INPUTS:
  			*mail	-	Mail structure
  	RETURNS:
  			0	-	Success.
  			1	-	Failed, unable to connect to database.
  			2	-	Failed, Treehouse database not found.
  			3	-	Failed, query failed
*/
gint add_mail (struct add_mail * mail)
{
  gchar *sql_cmd = "";
  gchar *sql_subject = "";
  gchar *sql_to = "";
  gchar *sql_from = "";
  gchar *folder = NULL;

  gint bfile, hfile;

  MYSQL mysql;
  MYSQL_RES *res;
  MYSQL_ROW row;
  gint num_rows, i;
  gchar *header = NULL, *tmp = NULL;

  /* Prepare our SQL insert statement */
  /* Allocate some memory for the SQL command */
  sql_subject = g_malloc ((strlen (mail->subject) * 2) + 1);
  sql_to = g_malloc ((strlen (mail->to) * 2) + 1);
  sql_from = g_malloc ((strlen (mail->from) * 2) + 2);

  /* We have to escape all the odd characters in our header and body so as to not confuse the query command. */
  mysql_escape_string (sql_subject, mail->subject, (unsigned int) strlen (mail->subject));
  mysql_escape_string (sql_to, mail->to, (unsigned int) strlen (mail->to));
  mysql_escape_string (sql_from, mail->from, (unsigned int) strlen (mail->from));

  /* Connect to the mySQL database */
  mysql = sql_connect ("treehouse");

  /* FIXME: This should really come from the database */
  if (!folder) folder = g_strdup ("Inbox");

  g_print ("moving mail '%s' to folder '%s'\n", mail->uidl, folder);

  /* Prepare the command string */
  sql_cmd = g_strdup_printf (
	   "INSERT INTO mail (mId, mfrom, mto, msubject, mreceived, msent, mfolder, mread, mreplied) VALUES ('%s', '%s', '%s', '%s', NOW(), '%s', '%s', '%d', '%d')",
	   mail->uidl, sql_from, sql_to, sql_subject, mail->date, folder, 0, 0);
printf ("Query: '%s'\n", sql_cmd);
  g_free (folder);

  if (mysql_errno (&mysql)) {
    display_error (_("Unable to establish database connection"));
    g_free (sql_cmd);
    g_free (sql_subject);
    return (1);
  }

  if (mysql_real_query (&mysql, sql_cmd, strlen (sql_cmd))) {
    g_print ("'%s'\n", (char *) mysql_error (&mysql));
    display_error (_("SQL Query failed"));
    g_free (sql_cmd);
    g_free (sql_subject);
    mysql_close (&mysql);
    return (3);
  }

  hfile = open_filew (mail->uidl, "h");
  write_file (hfile, mail->header);
  close (hfile);

  bfile = open_filew (mail->uidl, "b");
  write_file (hfile, mail->body);
  close (bfile);

  g_free (sql_cmd);
  g_free (sql_subject);
  mysql_close (&mysql);
  return (0);
}

/*
   create_sql_config_win - Prepare the sql config window for display
  
  	INPUTS:
			sql		-	structure used to store widgets
  	RETURNS:
  			0		-	user cancelled, exit Treehouse
  			1		-	user provided information. continue loading
*/
gint
create_sql_config_win (struct sql_config * sql)
{
  gint i;

  sql->dlg = gnome_dialog_new (_("MySQL Configuration"), GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_CANCEL, NULL);

  sql->serverbox = gtk_hbox_new (FALSE, 0);
  sql->userbox = gtk_hbox_new (FALSE, 0);
  sql->passwdbox = gtk_hbox_new (FALSE, 0);

  sql->serverlabel = gtk_label_new (_("MySQL Server: "));
  sql->userlabel = gtk_label_new (_("MySQL Username: "));
  sql->passwdlabel = gtk_label_new (_("MySQL Password: "));

  sql->server_gnome = gnome_entry_new ("sql_server");
  sql->server = gnome_entry_gtk_entry (GNOME_ENTRY (sql->server_gnome));
  gtk_entry_set_text (GTK_ENTRY (sql->server), sql_server);

  sql->user_gnome = gnome_entry_new ("sql_user");
  sql->user = gnome_entry_gtk_entry (GNOME_ENTRY (sql->user_gnome));
  gtk_entry_set_text (GTK_ENTRY (sql->user), sql_user);

  sql->passwd_gnome = gnome_entry_new (NULL);
  sql->passwd = gnome_entry_gtk_entry (GNOME_ENTRY (sql->passwd_gnome));
  if (g_strcasecmp (sql_passwd, "none")) {
    gtk_entry_set_text (GTK_ENTRY (sql->passwd), sql_passwd);
  }
  gtk_entry_set_visibility (GTK_ENTRY (sql->passwd), FALSE);

  sql->save_passwd = gtk_check_button_new_with_label (_("Save password"));

  gtk_box_pack_start (GTK_BOX (sql->serverbox), GTK_WIDGET (sql->serverlabel),
		      TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (sql->serverbox),
		      GTK_WIDGET (sql->server_gnome), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (sql->userbox), GTK_WIDGET (sql->userlabel),
		      TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (sql->userbox), GTK_WIDGET (sql->user_gnome),
		      TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (sql->passwdbox), GTK_WIDGET (sql->passwdlabel),
		      TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (sql->passwdbox),
		      GTK_WIDGET (sql->passwd_gnome), TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (sql->dlg)->vbox),
		      GTK_WIDGET (sql->serverbox), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (sql->dlg)->vbox),
		      GTK_WIDGET (sql->userbox), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (sql->dlg)->vbox),
		      GTK_WIDGET (sql->passwdbox), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (sql->dlg)->vbox),
		      GTK_WIDGET (sql->save_passwd), TRUE, TRUE, 0);

  gtk_window_set_position (GTK_WINDOW (sql->dlg), GTK_WIN_POS_CENTER);

  gtk_widget_show_all (sql->dlg);

  for (;;) {
    i = gnome_dialog_run (GNOME_DIALOG (sql->dlg));

    if (i == 0) {		/* User pressed OK */
      if (sql_server) g_free (sql_server);
      if (sql_user) g_free (sql_user);
      if (sql_passwd) g_free (sql_passwd);
      sql_server = g_strdup (gtk_entry_get_text (GTK_ENTRY (sql->server)));
      sql_user = g_strdup (gtk_entry_get_text (GTK_ENTRY (sql->user)));
      sql_passwd = g_strdup (gtk_entry_get_text (GTK_ENTRY (sql->passwd)));

      if (!strlen (sql_server) || !strlen (sql_user) || !strlen (sql_passwd)) {
	/* User did not complete all the settings options */
	display_error (_("Error: You MUST specify all three settings"));
	continue;
      } else {
	gnome_dialog_close (GNOME_DIALOG (sql->dlg));
	/* Save these settings */
        gnome_config_push_prefix ("/treehouse/config/");
	gnome_config_private_set_string ("server", sql_server);
	gnome_config_private_set_string ("user", sql_user);
	if (GTK_TOGGLE_BUTTON (sql->save_passwd)->active)
	  gnome_config_private_set_string ("passwd", sql_passwd);
        gnome_config_pop_prefix ();
	gnome_config_sync ();
	return (0);
	break;
      }
    } else if (i < 0) {		/* User closed window */
      return (1);
      break;
    } else if (i == 1) {	/* User pressed CANCEL */
      gnome_dialog_close (GNOME_DIALOG (sql->dlg));
      return (1);
      break;
    }
  }
}

gint init_sql_config (gboolean force)
{
  gint res = 0;
  gboolean flag = FALSE;
  gboolean def_set = FALSE;

  /* Check for our config */
  gnome_config_push_prefix ("/treehouse/config/");
  sql_server = gnome_config_private_get_string_with_default ("server=none", &def_set);
  if (def_set)
    flag = TRUE;
  sql_user = gnome_config_private_get_string_with_default ("user=none", &def_set);
  if (def_set)
    flag = TRUE;
  sql_passwd = gnome_config_private_get_string_with_default ("passwd=none", &def_set);
  if (def_set)
    flag = TRUE;
  gnome_config_pop_prefix ();

  if (flag || force) {
    sql = g_malloc (sizeof (struct sql_config));
    res = create_sql_config_win (sql);
    g_free (sql);
  }

  return (res);
}

