## Makefile config

gladedir = $(datadir)/treehouse/glade

INCLUDES = -I$(top_srcdir) -I$(includedir) $(GNOME_INCLUDEDIR) \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(top_srcdir)/intl -DG_LOG_DOMAIN=\"Treehouse\" \
        -DTREEHOUSE_GLADEDIR=\""$(gladedir)"\"

bin_PROGRAMS = treehouse
treehouse_SOURCES = 	treehouse.c \
			gui.c \
			smtp.c \
			pop3.c \
			sql.c \
			html.c \
			tcp.c \
			accounts.c \
			compose.c \
			folder.c \
			message.c \
			settings.c \
			fileio.c \
                        mime.c \
                        md5.c \
                        md5.h \
			header.h

treehouse_LDADD = $(GNOMEUI_LIBS) $(GNOME_LIBDIR) $(INTLLIBS)

glade_DATA = treehouse.glade

EXTRA_DIST = $(glade_DATA)

