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

    treehouse.c

    Main program function and misc functions
    $Header: /cvsroot/treehouse/treehouse/src/treehouse.c,v 1.8 2002/05/21 06:16:56 chrisjones Exp $
*/

/* Includes */
#include "header.h"
#include "md5.h"

/*
   main - Program entry point
  
  	INPUTS:
			argc		-	Argument count (supplied by the system)
			*argv[]		-	Arguments (supplied by the system)
  	RETURNS:
  			0	-	Success
  			1	-	Failed
*/
gint main (gint argc, gchar * argv[])
{
  gchar *tmp = NULL;
  MYSQL mysql;

  gnome_init ("Treehouse", VERSION, argc, argv);
  gdk_rgb_init();

  glade_gnome_init ();
  xml = glade_xml_new (TREEHOUSE_GLADEDIR "/treehouse.glade", NULL);
  if (!xml) {
    g_print ("Unable to locate glade file.\n");
    exit (0);
  }
  glade_xml_signal_autoconnect (xml);

  if (getuid () == 0)
    display_error ("Treehouse has not been security audited.\nIt could be DANGEROUS to run it as root");

  w = g_malloc (sizeof (struct MainWindow));
  w->handle = g_malloc (sizeof (GtkHTMLStream));
  bm_cfg = g_malloc (sizeof (struct bm_config));

  load_settings ();

  if (!init_sql_config (FALSE)) {
    /* Establish a database connection */
    /* This isn't actually used, but it makes the database load our tables */
    /*  so once the window appears it will work a bit quicker at the start */
    mysql = sql_connect ("treehouse");
    if (mysql_errno (&mysql)) {
      display_error ("Unable to establish database connection\nPlease check the server is running and check your\nsettings in the following window");
      if (init_sql_config (TRUE)) {
        g_free (w->handle);
	g_free (w);
	exit (0);
      }
    }
    mysql_close (&mysql);

    /* Initial HTML page to display */
    tmp = gnome_pixmap_file ("treehouse-logo-small.png");
    get_hex_as_string (bm_cfg->html_back_r, bm_cfg->html_back_g, bm_cfg->html_back_b, &bm_cfg->html_back);
    get_hex_as_string (bm_cfg->html_fore_r, bm_cfg->html_fore_g, bm_cfg->html_fore_b, &bm_cfg->html_fore);
    html_start = g_strconcat ("<html><head><title>Treehouse</title></head><body bgcolor=\"#", bm_cfg->html_back, "\" text=\"#", bm_cfg->html_fore,
			 "\"><br>&nbsp;<br>&nbsp;<center><img src=\"", tmp, "\" alt=\"Treehouse\"><h2>Version: " VERSION
			 " (alpha)</h2><br>", "<h5>$Header: /cvsroot/treehouse/treehouse/src/treehouse.c,v 1.8 2002/05/21 06:16:56 chrisjones Exp $</h5>", "<br><br><br><h2><font color=\"#FF0000\">WARNING: This is alpha software and will definitely break.</font></h2></center></body></html>", NULL);
    g_free (tmp);

    if (create_mainwin (w)) {
      g_free (w->handle);
      g_free (w);
      g_free (bm_cfg->html_back);
      g_free (bm_cfg->html_fore);
      g_free (bm_cfg);
      g_free (sql_server);
      g_free (sql_user);
      g_free (sql_passwd);
      g_free (html_start);
      return (1);
    }

    tmp = gnome_pixmap_file ("mail_open.xpm");
    open_xpm = gdk_pixmap_create_from_xpm (gtk_widget_get_parent_window (w->MessageList), NULL, NULL, tmp);
    g_free (tmp);
    tmp = gnome_unconditional_pixmap_file ("mail_closed.xpm");
    closed_xpm = gdk_pixmap_create_from_xpm (gtk_widget_get_parent_window (w->MessageList), NULL, NULL, tmp);
    g_free (tmp);

    gtk_widget_show_all (w->MainWin);
    gtk_main ();
  }

  g_free (w->handle);
  g_free (w);
  g_free (bm_cfg);
  g_free (sql_server);
  g_free (sql_user);
  g_free (sql_passwd);
  g_free (html_start);

  printf ("Exiting.\n");
  exit (0);
  return (0);
}

/*
   close_application - Cleans up any outstanding data and tells gtk to quit
  
  	INPUTS:
			*widget		-	Unused
			*event		-	Unused
			data		-	Unused
  	RETURNS:
*/
void close_application (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  gint i = 0;
  gchar tmp[1024];

  if (g_list_length (compwins) > 0) {
    do {
      compwins = g_list_first (compwins);
      gtk_object_destroy (GTK_OBJECT (compwins->data));
    } while (g_list_length (compwins) > 0);
  }

  /* Save data on the column widths */
  gnome_config_push_prefix ("/treehouse/config/columns");
  for (i = 0; i < 4; i++) {
    g_snprintf (tmp, 2, "%d", i);
    gnome_config_set_int (tmp, w->widths[i]);
  }
  gnome_config_pop_prefix ();

  g_print ("Storing panes as: '%d', '%d'\n", w->RootPaned_Left->allocation.width, w->MessageListPane->allocation.height);
  gnome_config_set_int ("/treehouse/config/hpaned", w->RootPaned_Left->allocation.width);
  gnome_config_set_int ("/treehouse/config/vpaned", w->MessageListPane->allocation.height);

  save_settings ();

  gtk_main_quit ();

  return;
}

/*
   display_error - Show an error in a GTK message box
  
  	INPUTS:
			text	-	error to display
  	RETURNS:
  			returns the result of the message box (not much use to anyone though)
*/
gint display_error (gchar * text)
{
  GtkWidget *dlg;

  dlg = gnome_message_box_new (text, GNOME_MESSAGE_BOX_ERROR, GNOME_STOCK_BUTTON_OK, NULL);

  gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);

  return (gnome_dialog_run (GNOME_DIALOG (dlg)));
}

/*
   cb_null - Null callback - used for unimplemented functions

	INPUTS:
	RETURNS:
*/
void
cb_null (void)
{
  printf ("cb_null called\n");
  return;
}

gint dpop3_log_add (char *cmd, char *arg)
{
  MYSQL mysql;
  char *query;

  mysql = sql_connect ("treehouse");

  query = g_strdup_printf ("INSERT INTO synclog (sTimestamp,sCmd,sArgs) VALUES (UNIX_TIMESTAMP(),'%s','%s')", cmd, arg);
printf ("Query: '%s'\n", query);

  if (mysql_real_query (&mysql, query, strlen (query))) {
    printf ("Query failed: '%s'\n", mysql_error (&mysql));
    display_error ("DPOP3 synclog failed, check console");
    g_free (query);
    mysql_close (&mysql);
    return (0);
  } else {
    g_free (query);
    mysql_close (&mysql);
    return (1);
  }

  return (1);
}

gchar *dpop3_get_foldid (char *folder)
{
  MYSQL mysql;
  MYSQL_RES *res;
  MYSQL_ROW row;
  char *query;
  int num;

  mysql = sql_connect ("treehouse");

  query = g_strdup_printf ("SELECT mid from folders WHERE mname = '%s'", folder);
  printf ("_get_foldid Query: '%s'\n", query);

  if (mysql_real_query (&mysql, query, strlen (query))) {
    printf ("Query failed: '%s'\n", mysql_error (&mysql));
    display_error ("DPOP3 get_foldid failed, check console");
    g_free (query);
    mysql_close (&mysql);
    return ("<NULL>");
  } else {
    g_free (query);
    res = mysql_store_result (&mysql);
    num = mysql_num_rows (res);
    if (num > 0) {
      row = mysql_fetch_row (res);
      printf ("Found mId: '%s'\n", row[0]);
      mysql_free_result (res);
      mysql_close (&mysql);
      return (row[0]);
    } else {
      printf ("Found an odd number of rows: '%d'\n", num);
      mysql_free_result (res);
      mysql_close (&mysql);
      return ("<NULL>");
    }
  }

  return (NULL);
}

gchar *dpop3_get_foldname (char *id)
{
  MYSQL mysql;
  MYSQL_RES *res;
  MYSQL_ROW row;
  char *query;
  int num;

  mysql = sql_connect ("treehouse");

  query = g_strdup_printf ("SELECT mname from folders WHERE mid = '%s'", id);

  if (mysql_real_query (&mysql, query, strlen (query))) {
    printf ("Query failed: '%s'\n", mysql_error (&mysql));
    display_error ("DPOP3 get_foldid failed, check console");
    g_free (query);
    mysql_close (&mysql);
    return (NULL);
  } else {
    g_free (query);
    res = mysql_store_result (&mysql);
    num = mysql_num_rows (res);
    if (num > 0) {
      row = mysql_fetch_row (res);
      mysql_free_result (res);
      mysql_close (&mysql);
      return (row[0]);
    } else {
      mysql_free_result (res);
      mysql_close (&mysql);
      return ("");
    }
  }

  return (NULL);
}

int dpop3_msg_exists (char *mid)
{
  MYSQL mysql;
  MYSQL_RES *res;
  char *query = NULL;
  int num;
  
  mysql = sql_connect ("treehouse");

  query = g_strdup_printf ("SELECT mid FROM mail WHERE mid = '%s'", mid);

  if (mysql_real_query (&mysql, query, strlen (query))) {
    printf ("Query failed '%s'\n", mysql_error (&mysql));
    display_error ("DPOP3 msg_exists failed, check console");
    g_free (query);
    mysql_close (&mysql);
    return (0);
  } else {
    g_free (query);
    res = mysql_store_result (&mysql);
    num = mysql_num_rows (res);
    mysql_free_result (res);
    mysql_close (&mysql);
    return (num);
  }

  return (0);
}

char *dpop3_md5 (char *text)
{
  md5_byte_t digest[16];
  md5_state_t state;
  char *tmp;
  int i = 0;

  md5_init (&state);
  md5_append (&state, (const md5_byte_t *)text, strlen (text));
  md5_finish (&state, digest);

  /* Allocate enough memory for 16 tuples with a null terminator */
  tmp = g_malloc (16 * 2 + 1);
  for (i = 0; i < 16; ++i)
    sprintf (tmp + i * 2, "%02x", digest[i]);
  *(tmp + 16 * 2) = '\0';
                  
  return (tmp);
}

char *dpop3_last_dsync ()
{
  MYSQL mysql;
  MYSQL_RES *res;
  MYSQL_ROW row;
  char *query = NULL;
  int num_rows;
 
  mysql = sql_connect ("treehouse");

  query = strdup ("SELECT mValue FROM misc WHERE mKey = 'last_dsync'");
printf ("Query: '%s'\n", query);
  if (mysql_real_query (&mysql, query, strlen (query))) {
    printf ("Query failed: '%s'\n", mysql_error (&mysql));
    g_free (query);
    mysql_close (&mysql);
  } else {
    g_free (query);
    res = mysql_store_result (&mysql);
    num_rows = mysql_num_rows (res);
    if (num_rows == 0) {
      mysql_free_result (res);
      mysql_close (&mysql);
      return ("0");
    } else {
      row = mysql_fetch_row (res);
      mysql_free_result (res);
      mysql_close (&mysql);
      return (row[0]);
    }
  }

  return ("0");
}

int dpop3_set_dsync (char *tstamp)
{
  MYSQL mysql;
  int rows_affected;
  char *query = NULL;

  mysql = sql_connect ("treehouse");

  query = g_strdup_printf ("UPDATE misc SET mValue = UNIX_TIMESTAMP() WHERE mKey = 'last_dsync'");

  printf ("Query: '%s'\n", query);

  if (mysql_real_query (&mysql, query, strlen (query))) {
    printf ("Query failed: '%s'\n", mysql_error (&mysql));
    g_free (query);
    mysql_close (&mysql);
    return (0);
  } else {
    g_free (query);
    mysql_close (&mysql);
    return (1);
  }

  return (0);
}

