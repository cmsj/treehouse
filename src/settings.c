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

    settings.c

    Configuration related functions
    $Header: /cvsroot/treehouse/treehouse/src/settings.c,v 1.6 2001/06/26 00:39:47 chrisjones Exp $
*/

/* Includes */
#include "header.h"

void load_settings (void)
{
  gboolean def = FALSE;

  gnome_config_push_prefix ("/treehouse/config/");
  bm_cfg->wrap_col    = gnome_config_get_int_with_default ("wrap_col=76", &def);
  bm_cfg->quote_char  = gnome_config_get_string_with_default ("wrap_char=>", &def);

  bm_cfg->normal_font = gnome_config_get_string_with_default ("normal_font=-Adobe-Courier-Medium-R-Normal--12-120-75-75-M-70-ISO8859-1", &def);
  bm_cfg->bold_font   = gnome_config_get_string_with_default ("bold_font=-Adobe-Courier-Bold-R-Normal--12-120-75-75-M-70-ISO8859-1", &def);

  bm_cfg->html_back_r = gnome_config_get_int_with_default ("html_back_r=255", &def);
  bm_cfg->html_back_g = gnome_config_get_int_with_default ("html_back_g=255", &def);
  bm_cfg->html_back_b = gnome_config_get_int_with_default ("html_back_b=255", &def);

  bm_cfg->html_fore_r = gnome_config_get_int_with_default ("html_fore_r=0", &def);
  bm_cfg->html_fore_g = gnome_config_get_int_with_default ("html_fore_g=0", &def);
  bm_cfg->html_fore_b = gnome_config_get_int_with_default ("html_fore_b=0", &def);
  if (def)
    printf ("Some config values were not found - using defaults\n");
  gnome_config_pop_prefix ();

  return;
}

void save_settings (void)
{
  gnome_config_push_prefix ("/treehouse/config/");
  gnome_config_set_int ("wrap_col", bm_cfg->wrap_col);
  gnome_config_set_string ("wrap_char", bm_cfg->quote_char);

  gnome_config_set_string ("normal_font", bm_cfg->normal_font);
  gnome_config_set_string ("bold_font", bm_cfg->bold_font);

  gnome_config_set_int ("html_back_r", bm_cfg->html_back_r);
  gnome_config_set_int ("html_back_g", bm_cfg->html_back_g);
  gnome_config_set_int ("html_back_b", bm_cfg->html_back_b);

  gnome_config_set_int ("html_fore_r", bm_cfg->html_fore_r);
  gnome_config_set_int ("html_fore_g", bm_cfg->html_fore_g);
  gnome_config_set_int ("html_fore_b", bm_cfg->html_fore_b);
  gnome_config_pop_prefix ();

  gnome_config_sync ();
  gnome_config_drop_all ();

  printf ("Settings saved\n");
  return;
}

void get_hex_as_string (int r, int g, int b, gchar **res)
{
  *res = g_strdup_printf ("%0x%0x%0x", r, g, b);
  return;
}

