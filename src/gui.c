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

   gui.c

   GUI related functions
   $Header: /cvsroot/treehouse/treehouse/src/gui.c,v 1.6 2001/06/26 00:39:47 chrisjones Exp $
*/

/* Includes */
#include "header.h"
#include "../pixmaps/mini_icon.xpm"

gchar *filter_list_titles[] = {
  N_("Name"),
  N_("Description"),
  N_("Target folder"),
  N_("mId")
};

/*
   cb_about - Callback to display About box
  
  	INPUTS:
			*widget		-	Unused
			data		-	Unused
  	RETURNS:
*/
void cb_about (GtkWidget * widget, gpointer data)
{
  GtkWidget *about;
  const gchar *authors[] = {
    "Chris Jones <chris@black-sun.co.uk>",
    NULL
  };

  about = gnome_about_new ("Treehouse", VERSION, "(c)1999 Chris Jones", authors,
		     _("MySQL driven mail client\n Homepage: http://www.black-sun.co.uk/treehouse\n Released under the GNU Public License"),
		     "treehouse-logo-small.png");

  gtk_window_set_position (GTK_WINDOW (about), GTK_WIN_POS_CENTER);
  gtk_widget_show (about);

  return;
}

/*
   cb_columns - Callback to provide column width persistance

	INPUTS:

	RETURNS:
*/
void cb_columns (GtkCList * clist, gint column, gint width, gpointer user_data)
{
  w->widths[column] = width;
  return;
}

/*
   create_mainwin - Prepare the main window for display
  
  	INPUTS:
			w		-	structure used to store widgets
  	RETURNS:
*/
gint create_mainwin (struct MainWindow * w)
{
  gint i = 0, j = 0;
  gchar *tmp;
  GdkPixmap *icon;
  GdkBitmap *icon_mask;

  /* Get a ref to important widgets from libglade */
  w->MainWin = glade_xml_get_widget (xml, "MainWin");
  w->MessageList = glade_xml_get_widget (xml, "MessageList");
  w->MessageListPane = glade_xml_get_widget (xml, "MessageListPane");
  w->FolderTree = glade_xml_get_widget (xml, "FolderTree");
  w->MailPopup = glade_xml_get_widget (xml, "MailPopup");
  w->StatusBar = glade_xml_get_widget (xml, "StatusBar");
  w->SourcePane = glade_xml_get_widget (xml, "SourcePane");
  w->RootPaned = glade_xml_get_widget (xml, "RootPaned");
  w->RootPaned_Left = glade_xml_get_widget (xml, "RootPaned_Left");
  w->RootPaned_Right = glade_xml_get_widget (xml, "RootPaned_Right");
  w->MessagePaneContainer = glade_xml_get_widget (xml, "MessagePaneContainer");

  w->MessagePane = gtk_html_new ();
  w->normal_font = gtk_style_new ();

  gtk_container_add (GTK_CONTAINER (w->MessagePaneContainer), w->MessagePane);

  gnome_appbar_set_default (GNOME_APPBAR (w->StatusBar), _(" Status: Ready"));
  gtk_clist_column_titles_active (GTK_CLIST (w->MessageList));
  gtk_clist_set_selection_mode (GTK_CLIST (w->MessageList), GTK_SELECTION_EXTENDED);
  gtk_clist_set_sort_column (GTK_CLIST (w->MessageList), 3);
  gtk_clist_set_sort_type (GTK_CLIST (w->MessageList), GTK_SORT_DESCENDING);
  gnome_less_set_fixed_font (GNOME_LESS (w->SourcePane), TRUE);

  for (i = 0; i < 4; i++) {
    tmp = g_strdup_printf ("/treehouse/config/column%d=-1", i);
    j = gnome_config_get_int_with_default (tmp, NULL);
    g_free (tmp);
    if (j != -1) {
      gtk_clist_set_column_width (GTK_CLIST (w->MessageList), i, j);
      w->widths[i] = j;
    }
  }

  gtk_clist_set_column_visibility (GTK_CLIST (w->MessageList), 4, FALSE);

  /* setup signal handlers */
  gtk_signal_connect (GTK_OBJECT (w->MessagePane), "url_requested", GTK_SIGNAL_FUNC (cb_url_requested), NULL);
  gtk_signal_connect (GTK_OBJECT (w->MessagePane), "link_clicked", GTK_SIGNAL_FUNC (cb_url_clicked), NULL);

  /* Fill the list of folders and sub folders */
  if (fill_folder_tree (w)) {
    display_error (_("Unable to generate folder list"));
    return (1);
  }

  w->last_sorted_column = 3;
  w->last_column_sort_type = GTK_SORT_DESCENDING;
  w->last_selected_folder[0] = '\0';

  gnome_popup_menu_attach (w->MailPopup, w->MessageList, NULL);

  /* Initialise the HTML widget */
  msg_pane_set_html (html_start);

  g_print ("Loading panes to: '%d', '%d'\n", gnome_config_get_int_with_default ("/treehouse/config/hpaned=-1", NULL), gnome_config_get_int_with_default ("/treehouse/config/vpaned=-1", NULL));

  gtk_paned_set_position (GTK_PANED (w->RootPaned), gnome_config_get_int ("/treehouse/config/hpaned=-1"));
  gtk_paned_set_position (GTK_PANED (w->RootPaned_Right), gnome_config_get_int ("/treehouse/config/vpaned=-1"));

  /* Set up our icon for tasklists, wm's, etc. */
  gtk_widget_realize (w->MainWin);
  icon = gdk_pixmap_create_from_xpm_d (w->MainWin->window, &icon_mask, &w->MainWin->style->bg[GTK_STATE_NORMAL], mini_icon_xpm);
  gdk_window_set_icon (w->MainWin->window, w->MainWin->window, icon, icon_mask);

  return (0);
}

/*
   msg_pane_set_html - Display some HTML in the message widget
  
  	INPUTS:
			*source		-	HTML source to display
  	RETURNS:
*/
void msg_pane_set_html (gchar * source)
{
  w->handle = gtk_html_begin (GTK_HTML (w->MessagePane));
  gtk_html_write (GTK_HTML (w->MessagePane), w->handle, source, strlen (source));
  gtk_html_end (GTK_HTML (w->MessagePane), w->handle, GTK_HTML_STREAM_OK);
}

/*
   cb_snd_rcv - Callback to send waiting mails and collect new ones
  
  	INPUTS:
			*widget		-	Unused
			data		-	Unused
  	RETURNS:
*/
void cb_snd_rcv (GtkWidget * widget, gpointer data)
{
  cb_send ();
  cb_receive ();

  return;
}

/*
    cb_click_title - Callback to handle mouse clicks to the MessageList column titles

	INPUTS:
			*clist			-	MessageList
			column			-	Column clicked on
	RETURNS:
*/
void cb_click_title (GtkCList * clist, gint column)
{

  gtk_clist_set_sort_column (clist, column);

  if (w->last_sorted_column == column) {
    if (w->last_column_sort_type == GTK_SORT_DESCENDING) {
      gtk_clist_set_sort_type (clist, GTK_SORT_ASCENDING);
      w->last_column_sort_type = GTK_SORT_ASCENDING;
    } else {
      gtk_clist_set_sort_type (clist, GTK_SORT_DESCENDING);
      w->last_column_sort_type = GTK_SORT_DESCENDING;
    }
  } else {
    gtk_clist_set_sort_type (clist, GTK_SORT_DESCENDING);
  }
  gtk_clist_sort (clist);

  w->last_sorted_column = column;

  return;
}

void cb_url_requested (GtkHTML *html, const char *url, GtkHTMLStream *handle)
{
  int fd;
  int bytes, part;
  char buf[1024];
  gchar *query = NULL;
  gchar *txt_h = NULL, *txt_b = NULL;
  gchar *mime_txt = NULL;
  gchar *tmp = NULL;
  GList *mime;
  ghttp_request *request = NULL;

  if (!g_strncasecmp (url, "mime://", 7)) {
    g_print ("MIME part requested ('%s')\n", url);
    query = g_strdup (url + 7);
    tmp = strstr (query, "?part=");
    if (!tmp) {
      /* We have a problem, this url is invalid */
      printf ("Invalid MIME url, bailing\n");
      g_free (query);
      return;
    }
    printf ("Determining part number from: '%s'\n", tmp + strlen ("?part="));
    part = atoi (tmp + strlen ("?part=")) - 1;
    *tmp = '\0';

    fd = open_file (query, "h");
    if (fd < 1) {
      g_free (query);
      return;
    }
    txt_h = read_file (fd);
    close (fd);

    fd = open_file (query, "b");
    if (fd < 1) {
      g_free (query);
      return;
    }
    txt_b = read_file (fd);
    close (fd);

    mime = mime_parse (txt_h, txt_b);
    mime_txt = mime_get_part (txt_b, mime, part, &bytes);
    g_print ("Got '%d' byte part - mime_get_part returned : '%d'\n", strlen (mime_txt), bytes);
    gtk_html_write (html, handle, mime_txt, bytes);
    gtk_html_end (html, handle, GTK_HTML_STREAM_OK);

    g_free (query);
    g_free (mime_txt);
    return;
  }

  if (!g_strncasecmp (url, "http://", 7)) {
    printf ("HTTP request: '%s'\n", url);
    request = ghttp_request_new ();
    if (ghttp_set_uri (request, (char *)url) == -1) {
      ghttp_request_destroy (request);
      return;
    }
    ghttp_set_header (request, http_hdr_Connection, "close");
    ghttp_prepare (request);
    ghttp_process (request);
    gtk_html_write (html, handle, ghttp_get_body (request), ghttp_get_body_len (request));
    gtk_html_end (html, handle, GTK_HTML_STREAM_OK);
    ghttp_request_destroy (request);
    return;
  }

  g_print ("URL requested: '%s'\n", url);

  fd = open (url, O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    return;
  }

  do {
    bytes = read(fd, buf, 1023);
    if (bytes) gtk_html_write(html, handle, buf, bytes);
  } while (bytes || errno == EAGAIN);

  close (fd);
  gtk_html_end (html, handle, GTK_HTML_STREAM_OK);
}

void cb_url_clicked (GtkHTML *html, const char *url, gpointer data)
{
  g_print ("URL requested: '%s'\n", url);
}

void cb_filters_dialog_new (GtkWidget *widget, gpointer data)
{
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *clist;
  GtkWidget *button;

  MYSQL mysql;
  MYSQL_RES *res;
  MYSQL_ROW row;
  gchar *query;
  gint num_rows = 0;

  gint i = 0;

  dialog = gnome_dialog_new (_("Treehouse filters"), GNOME_STOCK_BUTTON_OK, NULL);

  hbox = gtk_hbox_new (FALSE, 0);
  vbox = gtk_vbox_new (FALSE, 0);
  clist = gtk_clist_new_with_titles (4, filter_list_titles);

  button = gtk_button_new_with_label ("New");
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (button), FALSE, FALSE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (cb_null), NULL);

  button = gtk_button_new_with_label ("Edit");
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (button), FALSE, FALSE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (cb_null), NULL);

  button = gtk_button_new_with_label ("Delete");
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (button), FALSE, FALSE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (cb_null), NULL);


  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (clist), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (vbox), FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dialog)->vbox), GTK_WIDGET (hbox), TRUE, TRUE, 0);

  gtk_widget_show_all (GNOME_DIALOG (dialog)->vbox);

  mysql = sql_connect ("treehouse");
  query = g_strdup ("SELECT fname,fdesc,ffolder FROM filters");
  mysql_query (&mysql, query);
  g_free (query);

  if (mysql_errno (&mysql)) {
    display_error ((char *)mysql_error (&mysql));
    mysql_close (&mysql);
    return;
  }

  res = mysql_store_result (&mysql);

  for (i = 0; i < num_rows; i++) {
    row = mysql_fetch_row (res);
    gtk_clist_append (GTK_CLIST (clist), row);
  }

  i = gnome_dialog_run (GNOME_DIALOG (dialog));

  if (i == 0) { /* Ok button pressed */
    gnome_dialog_close (GNOME_DIALOG (dialog));
  }

  return;
}

gchar * gentry_get (GtkWidget *widget)
{
  return (g_strdup (gtk_entry_get_text (GTK_ENTRY (gnome_entry_gtk_entry (GNOME_ENTRY (widget))))));
}

void gentry_set (GtkWidget *widget, char *txt)
{
  gtk_entry_set_text (GTK_ENTRY (gnome_entry_gtk_entry (GNOME_ENTRY (widget))), txt);
  return;
}
