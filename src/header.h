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
*/

/*
   header.h

   Includes, function prototypes, structure definitions for Treehouse
   $Header: /cvsroot/treehouse/treehouse/src/header.h,v 1.10 2002/05/21 08:42:57 chrisjones Exp $
*/

#ifndef __BM_HEADER__
#define __BM_HEADER__

/* Defines */
#define TCP_READ	0
#define TCP_WRITE	1
#define TCP_BOTH	2
#define MAX_SERVER_BUF	1048576	/* Maximum bytes of data we can receive from a server  */
					/* This is only used for server responses and they are */
					/* VERY unlikely to return more than 1KB at a time     */
					/* so 8KB is plenty                                    */
#define _GNU_SOURCE

/* Specify the type of compose window to be created */
#define BM_COMP		0
#define BM_REPLY	1
#define BM_REPLYALL	2
#define	BM_FORWARD	3

/* FIXME: This should be a proper setting */
#define TAB_BACK        "AAAAAA"

/* Extra macro that GNOME should include, but doesn't */
/* Insert an item with a stock icon and pass data to the callback */
#define GNOMEUIINFO_ITEM_STOCK_WITH_DATA(label, tooltip, callback, data, stock_id) \
        { GNOME_APP_UI_ITEM, label, tooltip, (gpointer)callback, data, NULL, \
                GNOME_APP_PIXMAP_STOCK, stock_id, 0, (GdkModifierType) 0, NULL }

/* Includes */
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <ctype.h>
#include "mysql.h"
#include "mysqld_error.h"
#include <gnome.h>
#include <glade/glade.h>
#include <gdk/gdkx.h>
#include <gtkhtml/gtkhtml.h>
#ifndef _GTKHTML_STREAM_H_
  #include <gtkhtml/gtkhtml-stream.h>
#endif
#include <ghttp.h>
#include "config.h"

/* Structures */
struct MainWindow {
  GtkWidget *MainWin;
  GtkWidget *Root;
  GtkWidget *RootPaned;
  GtkWidget *RootPaned_Left;
  GtkWidget *RootPaned_Right;
  GtkWidget *MessageListPane;
  GtkWidget *MessagePaneContainer;
  GtkWidget *MessageNotebook;
  GtkWidget *MessagePane;
  GtkWidget *SourcePane;
  GtkWidget *FolderTree;
  GtkWidget *FolderPopup;
  GtkWidget *MessageList;
  GtkWidget *StatusBar;
  GtkWidget *MailPopup;
  GtkStyle *normal_font;
  GtkStyle *bold_font;

  gchar last_selected_folder[1024];
  gchar last_displayed_mail[1024];
  gint last_sorted_column;
  GtkSortType last_column_sort_type;

  GtkHTMLStream *handle;

  gint widths[5];
};

struct sql_config {
  GtkWidget *dlg;
  GtkWidget *serverbox;
  GtkWidget *userbox;
  GtkWidget *passwdbox;
  GtkWidget *serverlabel;
  GtkWidget *userlabel;
  GtkWidget *passwdlabel;
  GtkWidget *server;
  GtkWidget *user;
  GtkWidget *passwd;
  GtkWidget *server_gnome;
  GtkWidget *user_gnome;
  GtkWidget *passwd_gnome;
  GtkWidget *save_passwd;
};

struct account_dialog {
  GtkWidget *dlg;
  GtkWidget *root;
  GtkWidget *rootpaned;
  GtkWidget *rootpaned_left;
  GtkWidget *rootpaned_right;
  GtkWidget *root_bottom;
  GtkWidget *account_list;
  GtkWidget *right_vbox;
  GtkWidget *button;
};

struct smtp_mail {
  gchar *dummy;
  gchar *to;			/* Recipient(s) of the message (comma delimited)      */
  gchar *to_proper;		/* Recipient(s) of the message (normal format)        */
  gchar *from;			/* Sender of the message (email address only)         */
  gchar *from_proper;		/* Sender of the message (full format)                */
  gchar *cc;			/* Cc recipient(s) of the message (comma delimited)   */
  gchar *cc_proper;		/* Cc recipient(s) of the message (normal format)     */
  gchar *bcc;			/* Bcc recipient(s) of the message (comma delimited)  */
  gchar *org;			/* Organisation of the sender                         */
  gchar *replyto;		/* Reply-To of the sender                             */
  gchar *message;		/* Text of message                                    */
  gchar *subject;		/* Subject of message                                 */
  gchar *server;		/* Mail server to use to send the mail                */
  gint port;			/* TCP Port to use on the mail server                 */
};

struct add_mail {
  char *from;
  char *to;
  char *cc;
  char *subject;
  char *date;
  char *header;
  char *body;
  char *mailserver;
  char *username;
  char *folder;
  char *uidl;
};

struct new_mail {
  GtkWidget *CompWin;

  GtkWidget *RootVBox;

  GtkWidget *ToHBox;
  GtkWidget *ToBut;
  GtkWidget *ToStr;

  GtkWidget *CcHBox;
  GtkWidget *CcBut;
  GtkWidget *CcStr;

  GtkWidget *SubjHBox;
  GtkWidget *SubjLabel;
  GtkWidget *SubjStr;

  GtkWidget *TextHBox;
  GtkWidget *Text;
  GtkWidget *VAdj;

  GtkWidget *AttachLabel;
  GtkWidget *AttachCList;
};

struct bm_config {
  gchar *quote_char;
  gint wrap_col;

  gchar *normal_font;
  gchar *bold_font;

  gint html_back_r;
  gint html_back_g;
  gint html_back_b;
  gchar *html_back;

  gint html_fore_r;
  gint html_fore_g;
  gint html_fore_b;
  gchar *html_fore;
};

typedef struct {
  char *buf;
  size_t size;
} buf_info;

struct mime_part {
  gchar *type;
  gchar *subtype;
  gchar *parameter;
  gchar *disposition;
  gchar *encoding;
  guint offset,len;
};

enum {
  OPEN_FILE_READ,
  OPEN_FILE_WRITE
};

/* Global variables (yuck) */
GladeXML *xml;
struct MainWindow *w;
struct sql_config *sql;
struct bm_config *bm_cfg;
MYSQL mysql;
gchar *sql_server;
gchar *sql_database;
gchar *sql_user;
gchar *sql_passwd;
GList *compwins;
GdkPixmap *open_xpm, *closed_xpm;
gchar *html_start;
gint unread;

/* Function prototypes (sorted by the .c file in which they are defined) */
/* accounts.c */
gint fill_accounts_list (struct account_dialog *acc);
gint cb_accounts_dialog_new (GtkWidget * widget, gpointer data);
gint cb_accounts_dialog_add_new (GtkWidget * widget, gpointer data);
gint cb_accounts_dialog_edit (GtkWidget * widget, struct account_dialog *data);
void cb_accounts_dialog_delete (GtkWidget * widget, struct account_dialog *data);
void cb_accounts_set_default (GtkWidget * widget, struct account_dialog *data);

/* treehouse.c */
gint main (gint argc, gchar * argv[]);
void close_application (GtkWidget * widget, GdkEvent * event, gpointer data);
gint display_error (gchar * text);
void cb_null (void);
char *dpop3_get_foldid (char *folder);
char *dpop3_get_foldname (char *id);
gint dpop3_log_add (char *cmd, char *arg);
char *dpop3_md5 (char *text);
int dpop3_set_dsync (char *tstamp);
char *dpop3_last_dsync ();
int dpop3_msg_exists (char *msgid);

/* compose.c */
void create_composewin (gchar * mId, gboolean forward);
void cb_compose (void);
void cb_reply (void);
void cb_forward (void);
void quote_text (gchar * quoted_text, gchar * text);
void comp_quit (GtkWidget * widget, GtkWidget * CompWin);
void cb_send_clicked (GtkWidget * widget, struct new_mail *cw);
void cb_save_as (GtkWidget * widget, struct new_mail *cw);
void col_wrap (gchar ** body, gint cols);

/* fileio.c */
gint open_real (gchar *name, gchar *type, int mode);
gint open_file (gchar *name, gchar *type);
gint open_filew (gchar *name, gchar *type);
gchar *read_file (gint fp);
gint write_file (gint fp, char *text);

/* folder.c */
gint fill_folder_tree (struct MainWindow *w);
gint fill_folder_tree_children (int num_rows, MYSQL_RES ** res, GtkWidget ** tree, struct MainWindow *w, char *name);
void cb_add_new_folder (gchar * parent);
void cb_delete_folder (gchar * folder);
void cb_move_folder (void);
void cb_select_folder (GtkWidget * tree);

/* gui.c */
void cb_about (GtkWidget * widget, gpointer data);
void msg_pane_set_html (gchar * source);
gint create_mainwin (struct MainWindow *w);
void cb_columns (GtkCList * clist, gint column, gint width, gpointer user_data);
void cb_click_title (GtkCList * clist, gint column);
void cb_snd_rcv (GtkWidget * widget, gpointer data);
void cb_url_requested (GtkHTML *html, const char *url, GtkHTMLStream *handle);
void cb_url_clicked (GtkHTML *html, const char *url, gpointer data);
void cb_filters_dialog_new (GtkWidget * widget, gpointer data);
gchar * gentry_get (GtkWidget *widget);
void gentry_set (GtkWidget *widget, char *txt);

/* html.c */
void text_to_html (gchar * html_text, gchar * text);
gint html_len (gchar * html);

/* message.c */
void cb_show_message (GtkWidget * widget, gint row, gint column, GdkEventButton * event, gpointer data);
void fill_message_list (struct MainWindow *w, gchar * name);
void cb_delete_mail (GtkWidget * widget, gpointer * data);
void cb_move_to_folder (GtkWidget * widget, gpointer * data);
void cb_mark_read (struct MainWindow *w, gint row, gchar * mId);
void cb_mark_read_context (GtkWidget * widget, gpointer * data);
void cb_mark_unread (struct MainWindow *w, gint row, gchar * mId);
void cb_mark_unread_context (GtkWidget * widget, gpointer * data);
void cb_mark_all_read (GtkWidget * widget, gpointer * data);
void cb_mark_all_unread (GtkWidget * widget, gpointer * data);
void dpop3_dmretr (GIOChannel *io, char *arg);
void dpop3_dmput (GIOChannel *io, char *arg);
void dpop3_dmmove (char *arg);
void dpop3_dmdele (char *arg);
void dpop3_dfnew (char *arg);
void dpop3_dfmove (char *arg);
void dpop3_dfrene (char *arg);
void dpop3_dfdele (char *arg);
void dpop3_dsync (GIOChannel *io);

/* mime.c */
/* 
 * The following functions are taken from Spruce (by way of gmail).
 * Copyright Jeffrey Stedfast 1999-2000.
 * Some of the code has been re-written by Chris Jones and is Copyright 2000-2001 Chris Jones
 */
gchar *decode_base64 (gchar *data, gint *length);
gchar *encode_base64 (gchar *data, guint *length);
gchar *decode_quoted_printable (gchar *message, gint *length);
GList *mime_parse (gchar *headers, gchar *body);
gint mime_parse_content_type (gchar *source, gchar **type, gchar **subtype, gchar **parameter);
gchar *mime_get_parameter_value (gchar *source, gchar *parameter);
gchar *mime_get_part (gchar *message, GList *mime_parts, gint num, gint *len);
gint mime_get_first_text (GList *mime_parts);
/*
gint mime_parse_content_type (gchar *content_type, gchar *type, gchar *subtype, gchar *parameter);
gint mime_get_parameter_value (gchar *parameter, gchar *in_name, gchar *out);
gchar *mime_get_boundary (void);
gint mime_insert_part (gchar **message, gint index, gchar *boundary, gchar *file);
gchar *mesg_get_header_field (gchar *message, gchar *field);
gchar *unquote (gchar *string);
*/
/* 
 * End of Spruce functions.
 */

/* pop3.c */
gint get_mail (gchar * mailserver, gchar * username, gchar * password, gint port, gint delete, gchar * account);
gint getnword (gchar * s, gint n, gchar * word);
void cb_receive (void);
gint extract_header (gchar ** header, gchar * source, gchar * name);
gint parse_address (gchar ** name, gchar ** addr, gchar * source);
gchar *strstrcase (gchar *haystack, gchar *needle);

/* settings.c */
void load_settings (void);
void save_settings (void);
void get_hex_as_string (int r, int g, int b, gchar **res);
void create_settings_dialog (void);

/* smtp.c */
gint send_mail (struct smtp_mail *mail);
void cb_send (void);

/* sql.c */
gint init_sql_config (gboolean force);
extern gint add_mail (struct add_mail *mail);
gint create_sql_config_win (struct sql_config *sql);
MYSQL sql_connect (gchar * dbase);

/* tcp.c */
gint tcp_connect (GIOChannel *io, gchar *host, gint port);
gint tcp_read_write (GIOChannel *io, buf_info *buffer, gchar *cmd, gint flags);

#endif /* #define __BM_HEADER__ */

