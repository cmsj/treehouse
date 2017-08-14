/*
 * Gmail. A Gnome email client.
 * Copyright (C) 1999-2000 Wayne Schuller
 *
 * mime.c - gmail mime handling functions.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "header.h" 

static gchar base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

gchar *decode_base64 (gchar *data, gint *length)
{
	/* This function based on the base64 decoder by Jussi Junni of GnoMail */
	gchar *output, *workspace, *p;
	gulong pos = 0;
	gint i, a[4], len = 0;

	g_return_val_if_fail (data != NULL, NULL);

	workspace = g_malloc0(sizeof(gchar) * ((gint)(strlen(data) / 1.33) + 2));

	while (*data && len < *length) {
		for (i = 0; i < 4; i++, data++, len++) {
			if ((p = strchr (base64_chars, *data)))
				a[i] = (gint)(p - base64_chars);
			else
				i--;
		}

		workspace[pos]     = (((a[0] << 2) & 0xfc) | ((a[1] >> 4) & 0x03));
		workspace[pos + 1] = (((a[1] << 4) & 0xf0) | ((a[2] >> 2) & 0x0f));
		workspace[pos + 2] = (((a[2] << 6) & 0xc0) | (a[3] & 0x3f));
  
		if (a[2] == 64 && a[3] == 64) {
			workspace[pos + 1] = 0;
			pos -= 2;
		} else {
			if (a[3] == 64) {
				workspace[pos + 2] = 0;
				pos--;
			}
		}
		pos += 3;
	}

	output = g_malloc0(pos + 1);
	memcpy(output, workspace, pos);

	*length = pos;

	g_free (workspace);
    
	return output;
}

gchar *encode_base64(gchar *data, guint *length)
{
	gchar *encoded, *index, buffer[3];
	gint pos, len;

	/* Invalid inputs will cause this function to crash.  Return an error. */
	if (!length || !data)
		return 0;

	encoded = g_malloc0(sizeof(char)*(gint)(*length * 1.40)); /* it gets 33% larger */
	pos = 0;
	len = 0;
	index = data;
	while (index-data < *length) {
		/* There was a buffer overflow with the memcpy.  If length % 3 were not 0, it would read into someone
		   else's memory, or possibly just garbage.  It can mess up the last few bits of the base64 conversion. */
		if ( index-data+3 <= *length ) {
			memcpy(buffer, index, 3);
			*(encoded+pos)   = base64_chars[(buffer[0] >> 2) & 0x3f];
			*(encoded+pos+1) = base64_chars[((buffer[0] << 4) & 0x30) | ((buffer[1] >> 4) & 0xf)];
			*(encoded+pos+2) = base64_chars[((buffer[1] << 2) & 0x3c) | ((buffer[2] >> 6) & 0x3)];
			*(encoded+pos+3) = base64_chars[buffer[2] & 0x3f];
		} else
			if (index-data+2 == *length) {
				memcpy(buffer, index, 2);
				*(encoded+pos)   = base64_chars[(buffer[0] >> 2) & 0x3f];
				*(encoded+pos+1) = base64_chars[((buffer[0] << 4) & 0x30) | ((buffer[1] >> 4) & 0xf)];
				*(encoded+pos+2) = base64_chars[(buffer[1] << 2) & 0x3c];
				*(encoded+pos+3) = '=';
			} else
				if (index-data+1 == *length) {
					memcpy(buffer, index, 1);
					*(encoded+pos)   = base64_chars[(buffer[0] >> 2) & 0x3f];
					*(encoded+pos+1) = base64_chars[(buffer[0] << 4) & 0x30];
					*(encoded+pos+2) = '=';
					*(encoded+pos+3) = '=';
				} else {
					g_error("encode_base64(): corrupt data");
					return NULL;
				}

		len += 4;

		/* base64 can only have 76 chars per line */
		if (len >= 76) {
			*(encoded + pos + 4) = '\n';
			pos++;
			len = 0;
		}

		pos += 4;
		index += 3;
	}

	/* if there were less then a full triplet left, we pad the remaining
	 * encoded bytes with = */
	/*
	  if (*length % 3 == 1) {
	  *(encoded+pos-1) = '=';
	  *(encoded+pos-2) = '=';
	  }
	  if (*length % 3 == 2) {
	  *(encoded+pos-1) = '=';
	  }*/
	
	*(encoded+pos) = '\n';
	*(encoded+pos+1) = '\0';
	
	*length = strlen(encoded);
	
	return encoded;
}

gchar *decode_quoted_printable (gchar *message, gint *length)
{
	gchar *buffer, *index, ch[2];
	gint i = 0, temp;

	buffer = g_malloc0(*length + 1);
	index = message;

	while (index - message < *length) {
		if (*index == '=') {
			index++;
			if (*index != '\n') {
				sscanf(index, "%2x", &temp);
				sprintf(ch, "%c", temp);
				buffer[i] = ch[0];
			} else
				buffer[i] = index[1];
			i++;
			index += 2;
		} else {
			buffer[i] = *index;
			i++;
			index++;
		}
	}
	buffer[i] = '\0';

	*length = strlen(buffer);

	return buffer;
}

GList *mime_parse (gchar *headers, gchar *body)
{
  gchar *info_start, *info_end, *content_type, *type, *subtype, *parameter, *boundary, *tmp = body;
  GList *tmp_list = NULL;
  struct mime_part *part;

  if (!(extract_header (&content_type, headers, "\r\nContent-Type:"))) {
    if (mime_parse_content_type (content_type, &type, &subtype, &parameter)) {
      g_print ("mime_parse_content_type was passed a null value, bailing\n");
      return (0);
    }
  } else {
    type = g_strdup ("text");
    subtype = g_strdup ("plain");
    parameter = g_strdup (" ");
  }

#ifdef DEBUG
  g_print ("MIME type for message: '%s' (subtype: '%s', parameter: '%s')\n", type, subtype, parameter);
#endif

  if (!g_strcasecmp (type, "multipart")) {
    /* Message is multipart - find them all */
    boundary = mime_get_parameter_value (parameter, "boundary");
    if (!boundary) {
      /* Mail is broken  - try some emergency recovery */
g_print ("WARNING: This mail is probably broken, trying to recover\n");
      extract_header (&boundary, headers, "boundary=\"");
      if (boundary) {
        /* looks like we may well have recovered */
        if (*(boundary + strlen (boundary) - 1) == '"') {
          *(boundary + strlen (boundary) - 1) = '\0';
g_print ("Recovery may have succeeded, found boundary: '%s'\n", parameter);
        }
      } else {
        /* We failed to save ourselves */
        g_print ("Invalid MIME format - giving up.\n");
        g_free (parameter);
        g_free (type);
        g_free (subtype);
        return (NULL);
      }
    }

    /* Loop round and collect data about the parts until there are no more parts */
    while ((info_start = strcasestr (tmp, boundary))) {
      info_start += strlen (boundary) + 2;
      /* Have we hit the final boundary? If so, stop */
      if (!g_strncasecmp (info_start - 2, "--", 2))
        break;

      /* Grab the details of this part and store them */
      info_end = strstr (info_start, "\r\n\r\n");
      if (!info_end) {
        /* Some mailers are broken and don't put enough \r\n's in */
#ifdef DEBUG
        g_print ("Possible broken mailer - unable to find info_end, trying to recover\n");
#endif
        info_end = strstr (info_start, "\r\n");
        if (!info_end) {
          /* Other mailer is very broken */
#ifdef DEBUG
          g_print ("Mailer is definitely broken - complain to them. Giving up MIME parse\n");
#endif
          g_free (type);
          g_free (subtype);
          g_free (parameter);
          g_free (boundary);
          return (NULL);
        } else {
          info_end += 2;
        }
      } else {
        info_end += 4;
      }

      /* Start storing info about this part */
      part = g_malloc0 (sizeof (struct mime_part));
      part->offset = strlen (body) - strlen (info_end);

#ifdef DEBUG
      g_print ("Calculating part length:\n");
      g_print ("  length of offset: '%d'\n", strlen (body + part->offset));
      g_print ("  does boundary exist after info_end?: '%c'\n", strstr (info_end, boundary) ? 'y' : 'n');
      g_print ("  length of message after boundary: '%d'\n", strlen (strstr (info_end, boundary)));
#endif
      part->len = strlen (body + part->offset) - strlen (strstr (info_end, boundary));
#ifdef DEBUG
      g_print ("  length of part: '%d'\n", part->len);
#endif

      if (type) g_free (type);
      extract_header (&type, info_start - 2, "\r\nContent-Type:");
      if (mime_parse_content_type (type, &(part->type), &(part->subtype), &(part->parameter))) {
        g_print ("mime_parse_content_type was passed a null value, bailing\n");
        g_print ("info_start contains: '%s'\n", info_start);
        return (0);
      }
      g_free (type);
      type = g_strdup (info_start);
      *(type + (strlen (info_start) - strlen (info_end))) = '\0';
      extract_header (&(part->encoding), type, "\r\nContent-Transfer-Encoding:");
      if (!(part->encoding)) {
        part->encoding = g_strdup (" ");
      }
      extract_header (&(part->disposition), type, "\r\nContent-Disposition:"); /* type was info_start */
      if (!(part->disposition)) {
        part->disposition = g_strdup (" ");
      }
      g_free (type);
      type = NULL;

#ifdef DEBUG
      g_print ("  subpart type: '%s', subtype: '%s', length: '%d', offset: '%d', encoding: '%s', disposition: '%s'\n", part->type, part->subtype, part->len, part->offset, part->encoding, part->disposition);
#endif

      /* Add this part to our list */
      tmp_list = g_list_append (tmp_list, part);

#ifdef DEBUG
      g_print ("'%d' MIME parts found so far\n", g_list_length (tmp_list));
#endif

      /* Update where we are in the file */
      tmp = info_end;
    }
  } else {
    /* Message is not multipart */
    part = g_malloc0 (sizeof (struct mime_part));
    part->type = g_strdup (type);
    part->subtype = g_strdup (subtype);
    part->len = strlen (body);
    part->offset = 0;
    part->parameter = g_strdup (parameter);
    extract_header (&(part->encoding), headers, "\r\nContent-Transfer-Encoding:");
    if (!(part->encoding)) {
      part->encoding = g_strdup (" ");
    }
    extract_header (&(part->disposition), headers, "\r\nContent-Disposition:");
    if (!(part->disposition)) {
      part->disposition = g_strdup (" ");
    }
    tmp_list = g_list_append (tmp_list, part);
  }

  return (tmp_list);
}

gint mime_parse_content_type (gchar *source, gchar **type, gchar **subtype, gchar **parameter)
{
  gchar *tmp = NULL;
  gchar *ptr = NULL;

  if (!source)
    return (1);
 
g_print ("Parsing: '%s'\n", source);
  
  tmp = g_strdup (source);
  ptr = strchr (tmp, ';');
  if (ptr) {
    *parameter = g_strdup (ptr + 2);
    *ptr = '\0';
  } else {
    *parameter = g_strdup (" ");
  }

  ptr = strchr (tmp, '/') + 1;
  *subtype = g_strdup (ptr);

  *(ptr - 1) = '\0';
  *type = g_strdup (tmp);

  return (0);
}

gchar *mime_get_parameter_value (gchar *source, gchar *parameter)
{
  gchar *tmp, *ret;

  tmp = strcasestr (source, parameter);
  if (!tmp)
    return (NULL);
  tmp += strlen (parameter) + 1;

  if (*tmp == '"')
    tmp++;

  ret = g_strdup (tmp);
  if (*(ret + strlen (ret) - 1) == '"')
    *(ret + strlen (ret) - 1) = '\0';

  return (ret);
}

/* returns the num'th part (if existant) */
gchar *mime_get_part(gchar *message, GList *mime_parts, gint num, gint *in_len)
{
	gchar *part = NULL, *index = NULL, *ptr;
	GList *tmp;
	gint i, len;
	struct mime_part *m_part;

	tmp = g_list_nth (mime_parts, num);

        if (!tmp) {
          g_print ("Requested part ('%d') does not exist in '%d' length list\n", num, g_list_length (mime_parts));
          return (NULL);
        }

	m_part = tmp->data;

	index = message + m_part->offset;
	if (!g_strcasecmp(m_part->encoding, "base64")) {
		len = m_part->len;
		/* remove trailing boundary if there is one */
		for (ptr = index + len; ptr > index; ptr--)

			/* I added in an extra newline check, which made it work.
			 * The attachments I was getting had two newlines.
			 * I don't know how why it worked in Spruce. Maybe because
			 * it parses differently to us.
			 * Wayne.
                        */
			if (!strncmp(ptr, "\n\n--", 4))
			{
				len = (gint)(ptr - index);
				*ptr = '\0';
				break;
			}
		part = decode_base64(index, &len);
		*in_len = len;
	} else
		if (!g_strcasecmp(m_part->encoding, "quoted-printable")) {
			len = m_part->len;
			part = decode_quoted_printable(index, &len);
			*in_len = len;
		} else {
			part = g_malloc0 (m_part->len + 1);
			i = 0;
			while (*index != '\0' && i < m_part->len) {
				part[i] = *index;
				index++;
				i++;
			}
			part[i] = '\0';
			*in_len = strlen(part);
		}

	return (gchar *)part;
}

gint mime_parse_content_type2(gchar *content_type, gchar *type, gchar *subtype, gchar *parameter)
{
	gchar *index, *start;
	gint len, i;

	if (!content_type)
		return 0;

	len = strlen(content_type);

	start = index = content_type;

	/* copy the type */
	i = 0;
	while (*index != '/' && index - start < len) {
		type[i] = *index;
		i++;
		index++;
	}
	type[i] = '\0';

	/* copy the subtype */
	index++;
	i = 0;
	while (*index != ';' && index - start < len) {
		subtype[i] = *index;
		index++;
		i++;
	}
	subtype[i] = '\0';

	index++;
	while (isspace(*index))
		index++;

	/* copy the parameters */
	i = 0;
	while (*index != '\0' && index - start < len) {
		parameter[i] = *index;
		index++;
		i++;
	}
	parameter[i] = '\0';

	return 1;
}

gint mime_get_parameter_value2 (gchar *parameter, gchar *in_name, gchar *out)
{
	gchar *index, val[128];
	gint i;

	index = strstrcase(parameter, in_name);
	if (index) {
		for ( ; *index && *index != '=' && *index != ';'; index++);
		if (*index == '=')
			index++;
		memset(val, 0, sizeof(val));
		for (i = 0; *index && *index != ';' && i < 127; i++, index++)
			val[i] = *index;

		/* parameters can be either name=val or name="val", we can handle both */
	/*	unquote(val);*/
		strncpy(out, val, 127);
		return 1;
	}

	return 0;
}

/* returns the number of the first text/ part */
gint mime_get_first_text(GList *mime_parts)
{
	GList *tmp;
	gint i = 0;

	tmp = mime_parts;
	while (tmp != NULL) {
		if (!g_strcasecmp(((struct mime_part *)tmp->data)->type, "text"))
			return i;
		i++;
		tmp = tmp->next;
	}

	return 0;
}

/* generates a random boundary */
gchar *mime_get_boundary (void)
{
	static gchar boundary[128];
	gchar *index;
	gint i;

	srand(time(NULL) + getpid());
	memset(boundary, 0, sizeof(boundary));
	
	sprintf(boundary, "gmail");
	index = boundary + 6;
	i = 0;
	while (i < 20) {
		*(index+i) = (rand() % 26) + 65;
		i++;
	}
	*(index+i) = '\0';
	
	return boundary;
}

gint mime_insert_part (gchar **message, gint index, gchar *boundary, gchar *file)
{
	FILE *fp;
	struct stat st;
	gint ret, i, x;
	guint size;
	gchar *buffer, *encoded;
	gchar content_type[64];
	gchar content_transfer_encoding[64];

	ret = stat(file, &st);
	if (ret < 0) {
		printf (_("Couldn't stat %s"), file);
		return 0;
	}

	size = st.st_size;
	if (size == 0)
		return 0;
	
	/* this should be autodetected , so we would use text/plain and quoted-printable
	 * for attached text files and so on */
	g_snprintf(content_type, 64, "application/octet-stream; name=\"%s\"", g_basename(file));
	strcpy(content_transfer_encoding, "base64");

	fp = fopen(file, "rt");
	if (fp == NULL) {
		printf(_("Couldn't open %s\n"), file);
		return 0;
	}

	/* todo: it isn't very convinient to load all the file into memory at once 
	 * if the file is very large, 10 meg or something. (But who 
	 * would attach such a file?) */
	buffer = g_malloc0(sizeof(gchar) * (size + 1));

	fread (buffer, 1, size, fp);
	fclose(fp);
	buffer[size] = '\0';

	encoded = encode_base64(buffer, &size);
	g_free(buffer);
	

	/* we realloc the string so it can hold the encoded data + the boundary + 
	 * the part header */
	message[0] = g_realloc(message[0], strlen(message[0]) + strlen(boundary) + strlen(encoded) +
			       strlen(content_type) + strlen(content_transfer_encoding) +
			       strlen(g_basename(file)) + 100);

	i = x = 0;
	/* ok, here goes the part header */
	i += sprintf(message[0]+index+i, "--%s\n", boundary);
	i += sprintf(message[0]+index+i, "Content-Type: %s\nContent-Transfer-Encoding: %s\n",
		     content_type, content_transfer_encoding);
	i += sprintf(message[0]+index+i, "Content-Disposition: attachment; filename=\"%s\"\n\n",
		     g_basename(file));

	while (*(encoded+x) != '\0') {
		*(message[0]+index+i) = *(encoded+x);
		i++;
		x++;
	}
	*(message[0]+index+i) = '\0';

	g_free(encoded);
	
	return strlen(message[0]+index);
}


gchar *mesg_get_header_field (gchar *message, gchar *field)
{
	static gchar buffer[512];
	gchar *header, *index, temp_line[512];
	gchar *mesg_header; 
	gint i, done, first;

	/* lets find the end of the header info */
	index = strstr(message, "\r\n\r\n");
	if (index == NULL)
		return (gchar *)NULL;

	index += 2;

	/* now lets make a copy of the header info */
	mesg_header = g_malloc0(index - message + 1);
	strncpy(mesg_header, message, index - message);
	mesg_header[index - message] = '\0';


	index = strstrcase(mesg_header, field);
	if (index == NULL) {
		g_free(mesg_header);
		return NULL;
	}

	/* NULL out out buffer */
	memset(buffer, 0, sizeof(buffer));

	/* advance to the start of the header field */
	index += strlen(field) + 1;

	done = 0;
	first = 1;
	/* we'll get data until the first char in the line (except the first line) 
	 * is not a tab or a space (indicates a new header field) */
	while (done == 0) {
		i = 0;
		while (*index != '\n' && i < sizeof(temp_line)-1) {
			temp_line[i] = *index;
			i++;
			index++;
		}
		temp_line[i] = '\0';

		if (!first) {
			if (temp_line[0] != '\t' && temp_line[0] != ' ')
				done = 1;
		}
			
		if (!done) {
			i = 0;
			while (isspace (temp_line[i]))
				i++;
			strcat (buffer, " ");
			strncat(buffer, temp_line + i, sizeof(buffer)-1);
			index++;
			first = 0;
		}
	}

	g_strstrip(buffer);

/* 	parse_8bit(buffer); Do we really need this? */

	header = buffer;

	g_free(mesg_header);

	return header;
}

#if 0
gchar *strstrcase (gchar *haystack, gchar *needle)
{
  /* find the needle in the haystack neglecting case */
  gchar *ptr;
  guint len;

  g_return_val_if_fail (haystack != NULL, NULL);
  g_return_val_if_fail (needle != NULL, NULL);

  len = strlen(needle);
  if (len > strlen(haystack))
    return NULL;

  for (ptr = haystack; *(ptr + len - 1) != '\0'; ptr++)
    if (!g_strncasecmp(ptr, needle, len))
      return ptr;

  return NULL;
}
#endif

gchar *unquote (gchar *string)
{
  /* if the string is quoted, unquote it */

  if (!string)
    return NULL;

  if (*string == '"' && *(string + strlen(string) - 1) == '"') {
    *(string + strlen(string) - 1) = '\0';
    if (*string)
      memmove(string, string+1, strlen(string));
  }

  return string;
}

