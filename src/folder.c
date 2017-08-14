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

    folder.c

    Folder related functions
    $Header: /cvsroot/treehouse/treehouse/src/folder.c,v 1.6 2002/05/21 06:16:56 chrisjones Exp $
*/

/* Includes */
#include "header.h"

GnomeUIInfo mdata[] = {
  GNOMEUIINFO_MENU_NEW_ITEM ("New sub-folder", "Create a new folder", cb_add_new_folder, NULL),
  GNOMEUIINFO_MENU_NEW_ITEM ("Delete this folder", "Remove this folder", cb_delete_folder, NULL),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_NEW_ITEM ("Move folder to...", "Move this folder inside another", cb_move_folder, NULL),
  GNOMEUIINFO_END
};

/*
   fill_folder_tree - Puts a list of mail folders into the main window
  
  	INPUTS:
			w		-	structure used to store main window widgets
  	RETURNS:
  			0	-	Success
  			1	-	Failure
*/
gint fill_folder_tree (struct MainWindow * w)
{
  MYSQL mysql;
  MYSQL_RES *res;
  MYSQL_ROW row;

  gint i = 0, flag = 0;
  gchar *tmp1 = NULL;
  GtkWidget *item, *subtree, *popup;
  gint num_rows = 0;
  GtkWidget *foo;

  gtk_tree_remove_items (GTK_TREE (w->FolderTree), GTK_TREE (w->FolderTree)->children);

  mysql = sql_connect ("treehouse");

  if (mysql_errno (&mysql)) {
    display_error (_("Unable to establish database connection"));
    return (1);
  }

  mysql_query (&mysql, "SELECT mname,mparent FROM folders ORDER BY mname");

  res = mysql_store_result (&mysql);
  num_rows = (int) mysql_num_rows (res);

  foo = gtk_tree_item_new_with_label ("Treehouse");
  gtk_tree_append (GTK_TREE (w->FolderTree), foo);
  gtk_widget_show (foo);

  for (i = 0; i < num_rows; i++) {
    mysql_data_seek (res, i);
    row = mysql_fetch_row (res);

    tmp1 = g_strconcat ("Folder: '", row[0], "'", NULL);

    if (!(g_strcasecmp (row[1], "<NULL>"))) {
      subtree = gtk_tree_new ();

      item = gtk_tree_item_new_with_label (row[0]);

      popup = gnome_popup_menu_new (mdata);
      gnome_popup_menu_attach (popup, item, NULL);

      gtk_tree_append (GTK_TREE (w->FolderTree), item);
      gtk_widget_show (item);

      flag = fill_folder_tree_children (num_rows, &res, &subtree, w, row[0]);

      if (flag) {
	gtk_tree_item_set_subtree (GTK_TREE_ITEM (item), subtree);
	gtk_tree_item_expand (GTK_TREE_ITEM (item));

	flag = 0;
      }
      g_free (tmp1);
    }
  }

  mysql_free_result (res);
  mysql_close (&mysql);

  return (0);
}

/*
   fill_folder_tree_children - Recursive part of fill_folder_tree
  
  	INPUTS:
			num_rows	-	how many items we are dealing with
			**res		-	SQL query result obtained in fill_folder_tree
			**tree		-	FolderTree widget
			*w		-	structure used to store main window widgets
			*name		-	Name of current folder so we know which children to insert
  	RETURNS:
  			0	-	Success, no items inserted
  			1	-	Success, items inserted
*/
gint fill_folder_tree_children (int num_rows, MYSQL_RES ** res, GtkWidget ** tree, struct MainWindow * w, char *name)
{
  MYSQL_ROW row;
  gint j = 0;
  gint flag = 0, res_flag = 0;
  gchar *tmp1 = NULL;
  gchar *tmp = NULL;
  GtkWidget *subitem, *subtree, *popup;

  for (j = 0; j < num_rows; j++) {
    mysql_data_seek (*res, j);
    row = mysql_fetch_row (*res);

    tmp1 = g_strconcat ("Folder: '", row[0], "'", NULL);

    tmp = g_strdup (dpop3_get_foldname (row[1]));
    if (!(g_strcasecmp (tmp, name))) {
      /* we have a match */

      /* This is a sub-item so we want to expand the parent later */
      flag = 1;

      subtree = gtk_tree_new ();
      subitem = gtk_tree_item_new_with_label (row[0]);

      popup = gnome_popup_menu_new (mdata);
      gnome_popup_menu_attach (popup, subitem, NULL);

      gtk_tree_append (GTK_TREE (*tree), subitem);
      gtk_widget_show (subitem);

      res_flag = fill_folder_tree_children (num_rows, res, &subtree, w, row[0]);

      if (res_flag) {
	gtk_tree_item_set_subtree (GTK_TREE_ITEM (subitem), subtree);
	gtk_tree_item_expand (GTK_TREE_ITEM (subitem));

	res_flag = 0;
      }
    }
    g_free (tmp);
    g_free (tmp1);
  }

  return (flag);
}

/*
   cb_select_folder - Callback for displaying the contents of a folder
  
  	INPUTS:
			*tree		-	The list of mail folders
  	RETURNS:
*/
void cb_select_folder (GtkWidget * tree)
{
  GList *i;
  gchar *name;

  gchar *loading_msg;

  i = GTK_TREE_SELECTION (tree);
  if (i) {
    GtkLabel *label;
    GtkWidget *item;

    item = GTK_WIDGET (i->data);
    label = GTK_LABEL (GTK_BIN (item)->child);
    gtk_label_get (label, &name);
  } else {
    return;
  }

  if (!(g_strcasecmp (w->last_selected_folder, name)))
    return;

  if (!(g_strcasecmp (name, "Treehouse"))) {
    fill_message_list (w, "");
    msg_pane_set_html (html_start);
    gnome_less_clear (GNOME_LESS (w->SourcePane));
    strcpy (w->last_selected_folder, name);
    strcpy (w->last_displayed_mail, "welcome");
    return;
  }

  loading_msg = g_strconcat ("<html><body bgcolor=\"", bm_cfg->html_back, "\" text=\"", bm_cfg->html_fore, "\">Please wait, loading folder \"",
		 name ? name : w->last_selected_folder, "\"...</body></html>", NULL);

  msg_pane_set_html (loading_msg);

  if (!(name)) {
    fill_message_list (w, w->last_selected_folder);
  } else {
    fill_message_list (w, name);
    strcpy (w->last_selected_folder, name);
  }

  g_free (loading_msg);

  msg_pane_set_html ("<html><body bgcolor=\"#FFFFFF\">&nbsp;</body></html>");
  gnome_less_clear (GNOME_LESS (w->SourcePane));
  strcpy (w->last_displayed_mail, "");
  gtk_clist_select_row (GTK_CLIST (w->MessageList), 0, 0);

  return;
}

/*
   cb_add_new_folder - Create a new folder

	INPUTS:
			*parent		-	The folder within which the new folder will live
	RETURNS:
*/
void cb_add_new_folder (gchar * parent)
{
  GtkWidget *dlg;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;
  gint i = 0;
  gchar *name, *query = NULL;
  MYSQL mysql;

  if (!(g_strcasecmp (w->last_selected_folder, ""))) {
    display_error (_("Please select a folder first"));
    return;
  }

  name = g_strconcat (_("Create new folder in '"), w->last_selected_folder, "'", NULL);
  dlg = gnome_dialog_new (name, GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_CANCEL, NULL);
  g_free (name);
  hbox = gtk_hbox_new (FALSE, 0);
  label = gtk_label_new (_("Folder name:"));
  entry = gnome_entry_new ("new_folder_name");

  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (label), TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (entry), FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dlg)->vbox), GTK_WIDGET (hbox), TRUE, TRUE, 0);
  gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);

  gtk_widget_show_all (dlg);

  i = gnome_dialog_run (GNOME_DIALOG (dlg));

  if (i == 0) {			/* User pressed "Ok" */
    name = gentry_get (entry);

    mysql = sql_connect ("treehouse");

    if (mysql_errno (&mysql)) {
      display_error (_("Unable to establish database connection"));
      g_free (name);
      g_free (query);
      gnome_dialog_close (GNOME_DIALOG (dlg));
      return;
    }

    query = g_strconcat ("INSERT INTO folders (mid,mname,mdesc,mparent) VALUES('", dpop3_md5(name), "','", name, "','Folder','", dpop3_get_foldid (w->last_selected_folder), "')", NULL);
printf ("Query: '%s'\n", query);

    mysql_query (&mysql, query);

    if (mysql_errno (&mysql)) {
      if (mysql_errno (&mysql) == ER_DUP_ENTRY) {
	display_error (_("A folder with this name already exists\nFolder names must be unique"));
	mysql_close (&mysql);
	g_free (name);
	g_free (query);
	gnome_dialog_close (GNOME_DIALOG (dlg));
	return;
      } else {
	display_error ((char *) mysql_error (&mysql));
      }
    }

    mysql_close (&mysql);

    dpop3_log_add ("DFNEW", g_strconcat (dpop3_md5 (name), " ", name, " ", dpop3_get_foldid (w->last_selected_folder), NULL));

    g_free (name);
    g_free (query);

    fill_folder_tree (w);
  }

  gnome_dialog_close (GNOME_DIALOG (dlg));

  return;
}

/*
   cb_delete_folder - Delete a folder

	INPUTS:
			*folder		-	Name of the folder to delete
	RETURNS:
*/
void cb_delete_folder (gchar * folder)
{
  GtkWidget *dlg;
  GtkWidget *label;
  gint i = 0;
  gchar *name, *query = NULL;
  MYSQL mysql;

  if (!(g_strcasecmp (w->last_selected_folder, ""))) {
    display_error (_("Please select a folder first"));
    return;
  }

  name = g_strconcat (_("Delete folder '"), w->last_selected_folder, "'", NULL);
  dlg = gnome_dialog_new (name, GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_CANCEL, NULL);
  label = gtk_label_new (_("Are you sure you want to delete this folder?\nAnything in this folder will be moved to 'Deleted Items'"));
  g_free (name);

  gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dlg)->vbox), GTK_WIDGET (label), TRUE, TRUE, 0);
  gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);

  gtk_widget_show_all (dlg);

  i = gnome_dialog_run (GNOME_DIALOG (dlg));

  if (i == 0) {			/* User pressed "Ok" */
    mysql = sql_connect ("treehouse");

    if (mysql_errno (&mysql)) {
      display_error (_("Unable to establish database connection"));
      gnome_dialog_close (GNOME_DIALOG (dlg));
      return;
    }

    printf ("logging dfdele for folder: '%s'\n", w->last_selected_folder);
    dpop3_log_add ("DFDELE", dpop3_get_foldid (w->last_selected_folder));

    query = g_strconcat ("DELETE FROM folders WHERE mname = '", w->last_selected_folder, "'", NULL);

    mysql_query (&mysql, query);

    if (mysql_errno (&mysql)) {
      display_error ((char *) mysql_error (&mysql));
      g_free (query);
      mysql_close (&mysql);
      gnome_dialog_close (GNOME_DIALOG (dlg));
      return;
    }

    g_free (query);

    query = g_strconcat ("UPDATE mail SET mfolder = 'DeletedItems' WHERE mfolder = '", w->last_selected_folder, "'", NULL);

    mysql_query (&mysql, query);

    if (mysql_errno (&mysql)) {
      display_error ((char *) mysql_error (&mysql));
    }

    mysql_close (&mysql);

    fill_folder_tree (w);

    g_free (query);
  }

  gnome_dialog_close (GNOME_DIALOG (dlg));

  return;
}

/*
   cb_move_folder - Move a folder into another folder

	INPUTS:
	RETURNS:
*/
void cb_move_folder (void)
{
  struct MainWindow *move;
  GtkWidget *dlg;
  GtkWidget *Root;
  gint i = 0;
  GtkLabel *label;
  GtkWidget *item;
  GList *selection;
  gchar *folder = NULL;
  MYSQL mysql;
  MYSQL_RES *res;
  MYSQL_ROW row;
  gchar *cmd = NULL, *tmp = NULL;

  if (!(g_strcasecmp (w->last_selected_folder, ""))) {
    display_error (_("Please select a folder first"));
    return;
  }

  move = g_malloc (sizeof (*move));

  move->FolderTree = gtk_tree_new ();
  tmp = g_strconcat (_("Move folder '"), w->last_selected_folder, "'...", NULL);
  dlg = gnome_dialog_new (tmp, GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_CANCEL, NULL);
  g_free (tmp);
  Root = gtk_frame_new (NULL);

  gtk_container_add (GTK_CONTAINER (Root), GTK_WIDGET (move->FolderTree));
  gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (dlg)->vbox), GTK_WIDGET (Root), TRUE, TRUE, 0);
  gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);

  fill_folder_tree (move);

  gtk_widget_show_all (dlg);

  i = gnome_dialog_run (GNOME_DIALOG (dlg));

  if (i == 0) {			/* User pressed "Ok" */
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
    if (tmp) {
      *tmp = '\0';
    }

    if (!(g_strcasecmp (w->last_selected_folder, folder))) {
      display_error (_("You can't move a folder inside itself!"));

      gnome_dialog_close (GNOME_DIALOG (dlg));
      g_free (move);
      g_free (folder);
      return;
    }

    mysql = sql_connect ("treehouse");

    if (mysql_errno (&mysql)) {
      display_error (_("Unable to establish database connection"));
      gnome_dialog_close (GNOME_DIALOG (dlg));
      g_free (move);
      g_free (folder);
      return;
    }

    cmd = g_strconcat ("SELECT mparent FROM folders WHERE mname = '", w->last_selected_folder, "'", NULL);

    mysql_query (&mysql, cmd);

    g_free (cmd);

    if (mysql_errno (&mysql)) {
      display_error ((char *) mysql_error (&mysql));
      gnome_dialog_close (GNOME_DIALOG (dlg));
      mysql_close (&mysql);
      g_free (move);
      g_free (folder);
      return;
    }

    res = mysql_store_result (&mysql);
    row = mysql_fetch_row (res);

    if (!(g_strcasecmp (row[0], "<NULL>"))) {
      display_error (_("Only subfolders can be moved"));
      mysql_free_result (res);
      mysql_close (&mysql);
      gnome_dialog_close (GNOME_DIALOG (dlg));
      g_free (move);
      g_free (folder);
      return;
    }

    cmd = g_strconcat ("UPDATE folders SET mparent = '", dpop3_get_foldid (folder), "' WHERE mname = '", w->last_selected_folder, "'", NULL);

    mysql_query (&mysql, cmd);

    g_free (cmd);

    if (mysql_errno (&mysql)) {
      display_error ((char *) mysql_error (&mysql));
      gnome_dialog_close (GNOME_DIALOG (dlg));
      mysql_close (&mysql);
      g_free (move);
      g_free (folder);
      return;
    }

    cmd = g_strdup (dpop3_get_foldid (w->last_selected_folder));
    dpop3_log_add ("DFMOVE", g_strconcat (cmd, " ", dpop3_get_foldid (folder), NULL));
    g_free (cmd);

    g_free (folder);
    mysql_close (&mysql);
  }

  gnome_dialog_close (GNOME_DIALOG (dlg));
  g_free (move);

  fill_folder_tree (w);

  return;
}
