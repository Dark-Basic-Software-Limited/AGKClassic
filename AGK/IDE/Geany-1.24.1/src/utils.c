/*
 *      utils.c - this file is part of Geany, a fast and lightweight IDE
 *
 *      Copyright 2005-2012 Enrico Tröger <enrico(dot)troeger(at)uvena(dot)de>
 *      Copyright 2006-2012 Nick Treleaven <nick(dot)treleaven(at)btinternet(dot)com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along
 *      with this program; if not, write to the Free Software Foundation, Inc.,
 *      51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * General utility functions, non-GTK related.
 */

#include "geany.h"

#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <glib/gstdio.h>

#include <gio/gio.h>

#include "prefs.h"
#include "support.h"
#include "document.h"
#include "filetypes.h"
#include "dialogs.h"
#include "win32.h"
#include "project.h"
#include "ui_utils.h"
#include "templates.h"

#include "utils.h"

/**
 *  Tries to open the given URI in a browser.
 *  On Windows, the system's default browser is opened.
 *  On non-Windows systems, the browser command set in the preferences dialog is used. In case
 *  that fails or it is unset, the user is asked to correct or fill it.
 *
 *  @param uri The URI to open in the web browser.
 *
 *  @since 0.16
 **/
void utils_open_browser(const gchar *uri)
{
    g_return_if_fail(uri != NULL);
    
#ifdef G_OS_WIN32
	win32_open_browser(uri);
#elif __APPLE__
    gchar *cmdline = g_strconcat("open", " \"", uri, "\"", NULL);
    g_spawn_command_line_async(cmdline, NULL);
    g_free(cmdline);
#else
	gboolean again = TRUE;

	while (again)
	{
		gchar *cmdline = g_strconcat(tool_prefs.browser_cmd, " \"", uri, "\"", NULL);

		if (g_spawn_command_line_async(cmdline, NULL))
			again = FALSE;
		else
		{
			gchar *new_cmd = dialogs_show_input(_("Select Browser"), GTK_WINDOW(main_widgets.window),
				_("Failed to spawn the configured browser command. "
				  "Please correct it or enter another one."),
				tool_prefs.browser_cmd);

			if (new_cmd == NULL) /* user canceled */
				again = FALSE;
			else
				SETPTR(tool_prefs.browser_cmd, new_cmd);
		}
		g_free(cmdline);
	}
#endif
}


/* taken from anjuta, to determine the EOL mode of the file */
gint utils_get_line_endings(const gchar* buffer, gsize size)
{
	gsize i;
	guint cr, lf, crlf, max_mode;
	gint mode;

	cr = lf = crlf = 0;

	for (i = 0; i < size ; i++)
	{
		if (buffer[i] == 0x0a)
		{
			/* LF */
			lf++;
		}
		else if (buffer[i] == 0x0d)
		{
			if (i >= (size - 1))
			{
				/* Last char, CR */
				cr++;
			}
			else
			{
				if (buffer[i + 1] != 0x0a)
				{
					/* CR */
					cr++;
				}
				else
				{
					/* CRLF */
					crlf++;
				}
				i++;
			}
		}
	}

	/* Vote for the maximum */
	mode = SC_EOL_LF;
	max_mode = lf;
	if (crlf > max_mode)
	{
		mode = SC_EOL_CRLF;
		max_mode = crlf;
	}
	if (cr > max_mode)
	{
		mode = SC_EOL_CR;
		max_mode = cr;
	}

	return mode;
}


gboolean utils_isbrace(gchar c, gboolean include_angles)
{
	switch (c)
	{
		case '<':
		case '>':
		return include_angles;

		case '(':
		case ')':
		case '{':
		case '}':
		case '[':
		case ']': return TRUE;
		default:  return FALSE;
	}
}


gboolean utils_is_opening_brace(gchar c, gboolean include_angles)
{
	switch (c)
	{
		case '<':
		return include_angles;

		case '(':
		case '{':
		case '[':  return TRUE;
		default:  return FALSE;
	}
}


/**
 * Writes @a text into a file named @a filename.
 * If the file doesn't exist, it will be created.
 * If it already exists, it will be overwritten.
 *
 * @warning You should use @c g_file_set_contents() instead if you don't need
 * file permissions and other metadata to be preserved, as that always handles
 * disk exhaustion safely.
 *
 * @param filename The filename of the file to write, in locale encoding.
 * @param text The text to write into the file.
 *
 * @return 0 if the file was successfully written, otherwise the @c errno of the
 *         failed operation is returned.
 **/
gint utils_write_file(const gchar *filename, const gchar *text)
{
	g_return_val_if_fail(filename != NULL, ENOENT);
	g_return_val_if_fail(text != NULL, EINVAL);

	if (file_prefs.use_safe_file_saving)
	{
		GError *error = NULL;
		if (! g_file_set_contents(filename, text, -1, &error))
		{
			geany_debug("%s: could not write to file %s (%s)", G_STRFUNC, filename, error->message);
			g_error_free(error);
			return EIO;
		}
	}
	else
	{
		FILE *fp;
		gsize bytes_written, len;
		gboolean fail = FALSE;

		if (filename == NULL)
			return ENOENT;

		len = strlen(text);
		errno = 0;
		fp = g_fopen(filename, "w");
		if (fp == NULL)
			fail = TRUE;
		else
		{
			bytes_written = fwrite(text, sizeof(gchar), len, fp);

			if (len != bytes_written)
			{
				fail = TRUE;
				geany_debug(
					"utils_write_file(): written only %"G_GSIZE_FORMAT" bytes, had to write %"G_GSIZE_FORMAT" bytes to %s",
					bytes_written, len, filename);
			}
			if (fclose(fp) != 0)
				fail = TRUE;
		}
		if (fail)
		{
			geany_debug("utils_write_file(): could not write to file %s (%s)",
				filename, g_strerror(errno));
			return FALLBACK(errno, EIO);
		}
	}
	return 0;
}


/** Searches backward through @a size bytes looking for a '<'.
 * @param sel .
 * @param size .
 * @return The tag name (newly allocated) or @c NULL if no opening tag was found.
 */
gchar *utils_find_open_xml_tag(const gchar sel[], gint size)
{
	const gchar *cur, *begin;
	gsize len;

	cur = utils_find_open_xml_tag_pos(sel, size);
	if (cur == NULL)
		return NULL;

	cur++; /* skip the bracket */
	begin = cur;
	while (strchr(":_-.", *cur) || isalnum(*cur))
		cur++;

	len = (gsize)(cur - begin);
	return len ? g_strndup(begin, len) : NULL;
}


/** Searches backward through @a size bytes looking for a '<'.
 * @param sel .
 * @param size .
 * @return pointer to '<' of the found opening tag within @a sel, or @c NULL if no opening tag was found.
 */
const gchar *utils_find_open_xml_tag_pos(const gchar sel[], gint size)
{
	/* stolen from anjuta and modified */
	const gchar *begin, *cur;

	if (G_UNLIKELY(size < 3))
	{	/* Smallest tag is "<p>" which is 3 characters */
		return NULL;
	}
	begin = &sel[0];
	cur = &sel[size - 1];

	/* Skip to the character before the closing brace */
	while (cur > begin)
	{
		if (*cur == '>')
			break;
		--cur;
	}
	--cur;
	/* skip whitespace */
	while (cur > begin && isspace(*cur))
		cur--;
	if (*cur == '/')
		return NULL; /* we found a short tag which doesn't need to be closed */
	while (cur > begin)
	{
		if (*cur == '<')
			break;
		/* exit immediately if such non-valid XML/HTML is detected, e.g. "<script>if a >" */
		else if (*cur == '>')
			break;
		--cur;
	}

	/* if the found tag is an opening, not a closing tag or empty <> */
	if (*cur == '<' && *(cur + 1) != '/' && *(cur + 1) != '>')
		return cur;

	return NULL;
}


/* Returns true if tag_name is a self-closing tag */
gboolean utils_is_short_html_tag(const gchar *tag_name)
{
	const gchar names[][20] = {
		"area",
		"base",
		"basefont",	/* < or not < */
		"br",
		"col",
		"command",
		"embed",
		"frame",
		"hr",
		"img",
		"input",
		"keygen",
		"link",
		"meta",
		"param",
		"source",
		"track",
		"wbr"
	};

	if (tag_name)
	{
		if (bsearch(tag_name, names, G_N_ELEMENTS(names), 20,
			(GCompareFunc)g_ascii_strcasecmp))
				return TRUE;
	}
	return FALSE;
}


const gchar *utils_get_eol_name(gint eol_mode)
{
	switch (eol_mode)
	{
		case SC_EOL_CRLF: return _("Win (CRLF)"); break;
		case SC_EOL_CR: return _("Mac (CR)"); break;
		default: return _("Unix (LF)"); break;
	}
}


const gchar *utils_get_eol_char(gint eol_mode)
{
	switch (eol_mode)
	{
		case SC_EOL_CRLF: return "\r\n"; break;
		case SC_EOL_CR: return "\r"; break;
		default: return "\n"; break;
	}
}


/* Converts line endings to @a target_eol_mode. */
void utils_ensure_same_eol_characters(GString *string, gint target_eol_mode)
{
	const gchar *eol_str = utils_get_eol_char(target_eol_mode);

	/* first convert data to LF only */
	utils_string_replace_all(string, "\r\n", "\n");
	utils_string_replace_all(string, "\r", "\n");

	if (target_eol_mode == SC_EOL_LF)
		return;

	/* now convert to desired line endings */
	utils_string_replace_all(string, "\n", eol_str);
}


gboolean utils_atob(const gchar *str)
{
	if (G_UNLIKELY(str == NULL))
		return FALSE;
	else if (strcmp(str, "TRUE") == 0 || strcmp(str, "true") == 0)
		return TRUE;
	return FALSE;
}


/* NULL-safe version of g_path_is_absolute(). */
gboolean utils_is_absolute_path(const gchar *path)
{
	if (G_UNLIKELY(EMPTY(path)))
		return FALSE;

	return g_path_is_absolute(path);
}

/* returns the path to the file name relative to the base path
   if there is no overlap then file_name is returned 
   both must be absolute paths
   always returns a new string that must be freed */
gchar* utils_get_relative_path(const gchar* base_path, const gchar* file_name )
{
	guint length = strlen(file_name)*2+1;
	if ( length < 256 ) length = 256;
	gchar *tmp = g_new( gchar, length );
	const gchar* base_ptr = base_path;
	const gchar* file_ptr = file_name;

	*tmp = 0;

	if ( *base_ptr != *file_ptr )
	{
		strcpy( tmp, file_name );
		return tmp;
	}

	const gchar* last_file_slash = file_name;

	while( *base_ptr && *file_ptr )
	{
		base_ptr++;
		file_ptr++;

		if ( *base_ptr == *file_ptr )
		{
			if ( *file_ptr == '/' || *file_ptr == '\\' ) 
				last_file_slash = file_ptr+1;
		}
		else
		{
			if ( !*base_ptr && (*file_ptr == '/' || *file_ptr == '\\') ) 
				last_file_slash = file_ptr+1;

			break;
		}
	}

	while( *base_ptr )
	{
		if ( *base_ptr == '/' || *base_ptr == '\\' || *base_ptr == 0 )
		{
			strcat( tmp, "../" );
		}
		base_ptr++;
	}

	strcat( tmp, last_file_slash );
	return tmp;
}

/* removes any ../ from the path 
   returns a new string that needs to be freed */
// use utils_tidy_path instead
/*
gchar* utils_get_real_path(const gchar* file_name)
{
	guint length = strlen(file_name)+1;
	gchar *tmp = g_new( gchar, length );

	*tmp = 0;

	gchar* tmp_ptr = tmp;
	const gchar* file_ptr = file_name;

	while ( *file_ptr && tmp_ptr < (tmp+length) )
	{
		if ( (*file_ptr == '/' || *file_ptr == '\\') && *(file_ptr+1) == '.' && *(file_ptr+2) == '.' && (*(file_ptr+3) == '/' || *(file_ptr+3) == '\\') )
		{
			do
			{
				if ( tmp_ptr > tmp ) tmp_ptr--;	
			} while ( tmp_ptr > tmp && *tmp_ptr != '/' && *tmp_ptr != '\\' );

			file_ptr += 3;
		}
		else
		{
			*tmp_ptr = *file_ptr;
			tmp_ptr++;
			file_ptr++;
		}
	}

	*tmp_ptr = 0;
	return tmp;
}
*/

/* Skips root if path is absolute, do nothing otherwise.
 * This is a relative-safe version of g_path_skip_root().
 */
const gchar *utils_path_skip_root(const gchar *path)
{
	const gchar *path_relative;

	path_relative = g_path_skip_root(path);

	return (path_relative != NULL) ? path_relative : path;
}


gdouble utils_scale_round(gdouble val, gdouble factor)
{
	/*val = floor(val * factor + 0.5);*/
	val = floor(val);
	val = MAX(val, 0);
	val = MIN(val, factor);

	return val;
}


/* like g_utf8_strdown() but if @str is not valid UTF8, convert it from locale first.
 * returns NULL on charset conversion failure */
gchar *utf8_strdown(const gchar *str)
{
	gchar *down;

	if (g_utf8_validate(str, -1, NULL))
		down = g_utf8_strdown(str, -1);
	else
	{
		down = g_locale_to_utf8(str, -1, NULL, NULL, NULL);
		if (down)
			SETPTR(down, g_utf8_strdown(down, -1));
	}

	return down;
}


/**
 *  A replacement function for g_strncasecmp() to compare strings case-insensitive.
 *  It converts both strings into lowercase using g_utf8_strdown() and then compare
 *  both strings using strcmp().
 *  This is not completely accurate regarding locale-specific case sorting rules
 *  but seems to be a good compromise between correctness and performance.
 *
 *  The input strings should be in UTF-8 or locale encoding.
 *
 *  @param s1 Pointer to first string or @c NULL.
 *  @param s2 Pointer to second string or @c NULL.
 *
 *  @return an integer less than, equal to, or greater than zero if @a s1 is found, respectively,
 *          to be less than, to match, or to be greater than @a s2.
 *
 *  @since 0.16
 */
gint utils_str_casecmp(const gchar *s1, const gchar *s2)
{
	gchar *tmp1, *tmp2;
	gint result;

	g_return_val_if_fail(s1 != NULL, 1);
	g_return_val_if_fail(s2 != NULL, -1);

	/* ensure strings are UTF-8 and lowercase */
	tmp1 = utf8_strdown(s1);
	if (! tmp1)
		return 1;
	tmp2 = utf8_strdown(s2);
	if (! tmp2)
	{
		g_free(tmp1);
		return -1;
	}

	/* compare */
	result = strcmp(tmp1, tmp2);

	g_free(tmp1);
	g_free(tmp2);
	return result;
}

gint utils_str_n_casecmp(const gchar *s1, const gchar *s2, gint len)
{
	gchar *tmp1, *tmp2;
	gint result;

	g_return_val_if_fail(s1 != NULL, 1);
	g_return_val_if_fail(s2 != NULL, -1);

	/* ensure strings are UTF-8 and lowercase */
	tmp1 = utf8_strdown(s1);
	if (! tmp1)
		return 1;
	tmp2 = utf8_strdown(s2);
	if (! tmp2)
	{
		g_free(tmp1);
		return -1;
	}

	/* compare */
	result = strncmp(tmp1, tmp2, len);

	g_free(tmp1);
	g_free(tmp2);
	return result;
}

/**
 *  Truncates the input string to a given length.
 *  Characters are removed from the middle of the string, so the start and the end of string
 *  won't change.
 *
 *  @param string Input string.
 *  @param truncate_length The length in characters of the resulting string.
 *
 *  @return A copy of @a string which is truncated to @a truncate_length characters,
 *          should be freed when no longer needed.
 *
 *  @since 0.17
 */
/* This following function is taken from Gedit. */
gchar *utils_str_middle_truncate(const gchar *string, guint truncate_length)
{
	GString *truncated;
	guint length;
	guint n_chars;
	guint num_left_chars;
	guint right_offset;
	guint delimiter_length;
	const gchar *delimiter = "\342\200\246";

	g_return_val_if_fail(string != NULL, NULL);

	length = strlen(string);

	g_return_val_if_fail(g_utf8_validate(string, length, NULL), NULL);

	/* It doesnt make sense to truncate strings to less than the size of the delimiter plus 2
	 * characters (one on each side) */
	delimiter_length = g_utf8_strlen(delimiter, -1);
	if (truncate_length < (delimiter_length + 2))
		return g_strdup(string);

	n_chars = g_utf8_strlen(string, length);

	/* Make sure the string is not already small enough. */
	if (n_chars <= truncate_length+1) // if only removing 2 chars then ellipses take up as much room as the chars, so don't bother
		return g_strdup (string);

	/* Find the 'middle' where the truncation will occur. */
	num_left_chars = (truncate_length - delimiter_length) / 2;
	right_offset = n_chars - truncate_length + num_left_chars + delimiter_length;

	truncated = g_string_new_len(string, g_utf8_offset_to_pointer(string, num_left_chars) - string);
	g_string_append(truncated, delimiter);
	g_string_append(truncated, g_utf8_offset_to_pointer(string, right_offset));

	return g_string_free(truncated, FALSE);
}


/**
 *  @c NULL-safe string comparison. Returns @c TRUE if both @a a and @a b are @c NULL
 *  or if @a a and @a b refer to valid strings which are equal.
 *
 *  @param a Pointer to first string or @c NULL.
 *  @param b Pointer to second string or @c NULL.
 *
 *  @return @c TRUE if @a a equals @a b, else @c FALSE.
 **/
gboolean utils_str_equal(const gchar *a, const gchar *b)
{
	/* (taken from libexo from os-cillation) */
	if (a == NULL && b == NULL) return TRUE;
	else if (a == NULL || b == NULL) return FALSE;

	while (*a == *b++)
		if (*a++ == '\0')
			return TRUE;

	return FALSE;
}


/**
 *  Removes the extension from @a filename and return the result in a newly allocated string.
 *
 *  @param filename The filename to operate on.
 *
 *  @return A newly-allocated string, should be freed when no longer needed.
 **/
gchar *utils_remove_ext_from_filename(const gchar *filename)
{
	gchar *last_dot;
	gchar *result;
	gsize len;

	g_return_val_if_fail(filename != NULL, NULL);

	last_dot = strrchr(filename, '.');
	if (! last_dot)
		return g_strdup(filename);

	len = (gsize) (last_dot - filename);
	result = g_malloc(len + 1);
	memcpy(result, filename, len);
	result[len] = 0;

	return result;
}


gchar utils_brace_opposite(gchar ch)
{
	switch (ch)
	{
		case '(': return ')';
		case ')': return '(';
		case '[': return ']';
		case ']': return '[';
		case '{': return '}';
		case '}': return '{';
		case '<': return '>';
		case '>': return '<';
		default: return '\0';
	}
}


/* Checks whether the given file can be written. locale_filename is expected in locale encoding.
 * Returns 0 if it can be written, otherwise it returns errno */
gint utils_is_file_writable(const gchar *locale_filename)
{
	gchar *file;
	gint ret;

	if (! g_file_test(locale_filename, G_FILE_TEST_EXISTS) &&
		! g_file_test(locale_filename, G_FILE_TEST_IS_DIR))
		/* get the file's directory to check for write permission if it doesn't yet exist */
		file = g_path_get_dirname(locale_filename);
	else
		file = g_strdup(locale_filename);

#ifdef G_OS_WIN32
	/* use _waccess on Windows, access() doesn't accept special characters */
	ret = win32_check_write_permission(file);
#else

	/* access set also errno to "FILE NOT FOUND" even if locale_filename is writeable, so use
	 * errno only when access() explicitly returns an error */
	if (access(file, R_OK | W_OK) != 0)
		ret = errno;
	else
		ret = 0;
#endif
	g_free(file);
	return ret;
}


/* Replaces all occurrences of needle in haystack with replacement.
 * Warning: *haystack must be a heap address; it may be freed and reassigned.
 * Note: utils_string_replace_all() will always be faster when @a replacement is longer
 * than @a needle.
 * All strings have to be NULL-terminated.
 * See utils_string_replace_all() for details. */
void utils_str_replace_all(gchar **haystack, const gchar *needle, const gchar *replacement)
{
	GString *str;

	g_return_if_fail(*haystack != NULL);

	str = g_string_new(*haystack);

	g_free(*haystack);
	utils_string_replace_all(str, needle, replacement);

	*haystack = g_string_free(str, FALSE);
}

void utils_str_replace_char(gchar *haystack, gchar needle, gchar replacement)
{
	while( *haystack )
	{
		if ( *haystack == needle ) *haystack = replacement;
		haystack++;
	}
}

gint utils_strpos(const gchar *haystack, const gchar *needle)
{
	const gchar *sub;

	if (! *needle)
		return -1;

	sub = strstr(haystack, needle);
	if (! sub)
		return -1;

	return sub - haystack;
}


/**
 *  Retrieves a formatted date/time string from strftime().
 *  This function should be preferred to directly calling strftime() since this function
 *  works on UTF-8 encoded strings.
 *
 *  @param format The format string to pass to strftime(3). See the strftime(3)
 *                documentation for details, in UTF-8 encoding.
 *  @param time_to_use The date/time to use, in time_t format or NULL to use the current time.
 *
 *  @return A newly-allocated string, should be freed when no longer needed.
 *
 *  @since 0.16
 */
gchar *utils_get_date_time(const gchar *format, time_t *time_to_use)
{
	const struct tm *tm;
	static gchar date[1024];
	gchar *locale_format;
	gsize len;

	g_return_val_if_fail(format != NULL, NULL);

	if (! g_utf8_validate(format, -1, NULL))
	{
		locale_format = g_locale_from_utf8(format, -1, NULL, NULL, NULL);
		if (locale_format == NULL)
			return NULL;
	}
	else
		locale_format = g_strdup(format);

	if (time_to_use != NULL)
		tm = localtime(time_to_use);
	else
	{
		time_t tp = time(NULL);
		tm = localtime(&tp);
	}

	len = strftime(date, 1024, locale_format, tm);
	g_free(locale_format);
	if (len == 0)
		return NULL;

	if (! g_utf8_validate(date, len, NULL))
		return g_locale_to_utf8(date, len, NULL, NULL, NULL);
	else
		return g_strdup(date);
}


gchar *utils_get_initials(const gchar *name)
{
	gint i = 1, j = 1;
	gchar *initials = g_malloc0(5);

	initials[0] = name[0];
	while (name[i] != '\0' && j < 4)
	{
		if (name[i] == ' ' && name[i + 1] != ' ')
		{
			initials[j++] = name[i + 1];
		}
		i++;
	}
	return initials;
}


/**
 *  Wraps g_key_file_get_integer() to add a default value argument.
 *
 *  @param config A GKeyFile object.
 *  @param section The group name to look in for the key.
 *  @param key The key to find.
 *  @param default_value The default value which will be returned when @a section or @a key
 *         don't exist.
 *
 *  @return The value associated with @a key as an integer, or the given default value if the value
 *          could not be retrieved.
 **/
gint utils_get_setting_integer(GKeyFile *config, const gchar *section, const gchar *key,
							   const gint default_value)
{
	gint tmp;
	GError *error = NULL;

	g_return_val_if_fail(config, default_value);

	tmp = g_key_file_get_integer(config, section, key, &error);
	if (error)
	{
		g_error_free(error);
		return default_value;
	}
	return tmp;
}


/**
 *  Wraps g_key_file_get_boolean() to add a default value argument.
 *
 *  @param config A GKeyFile object.
 *  @param section The group name to look in for the key.
 *  @param key The key to find.
 *  @param default_value The default value which will be returned when @c section or @c key
 *         don't exist.
 *
 *  @return The value associated with @a key as a boolean, or the given default value if the value
 *          could not be retrieved.
 **/
gboolean utils_get_setting_boolean(GKeyFile *config, const gchar *section, const gchar *key,
								   const gboolean default_value)
{
	gboolean tmp;
	GError *error = NULL;

	g_return_val_if_fail(config, default_value);

	tmp = g_key_file_get_boolean(config, section, key, &error);
	if (error)
	{
		g_error_free(error);
		return default_value;
	}
	return tmp;
}


/**
 *  Wraps g_key_file_get_string() to add a default value argument.
 *
 *  @param config A GKeyFile object.
 *  @param section The group name to look in for the key.
 *  @param key The key to find.
 *  @param default_value The default value which will be returned when @a section or @a key
 *         don't exist.
 *
 *  @return A newly allocated string, either the value for @a key or a copy of the given
 *          default value if it could not be retrieved.
 **/
gchar *utils_get_setting_string(GKeyFile *config, const gchar *section, const gchar *key,
								const gchar *default_value)
{
	gchar *tmp;

	g_return_val_if_fail(config, g_strdup(default_value));

	tmp = g_key_file_get_string(config, section, key, NULL);
	if (!tmp)
	{
		return g_strdup(default_value);
	}
	return tmp;
}


gchar *utils_get_hex_from_color(GdkColor *color)
{
	g_return_val_if_fail(color != NULL, NULL);

	return g_strdup_printf("#%02X%02X%02X",
		(guint) (utils_scale_round(color->red / 256, 255)),
		(guint) (utils_scale_round(color->green / 256, 255)),
		(guint) (utils_scale_round(color->blue / 256, 255)));
}


/* Get directory from current file in the notebook.
 * Returns dir string that should be freed or NULL, depending on whether current file is valid.
 * Returned string is in UTF-8 encoding */
gchar *utils_get_current_file_dir_utf8(void)
{
	GeanyDocument *doc = document_get_current();

	if (doc != NULL)
	{
		/* get current filename */
		const gchar *cur_fname = doc->file_name;

		if (cur_fname != NULL)
		{
			/* get folder part from current filename */
			return g_path_get_dirname(cur_fname); /* returns "." if no path */
		}
	}

	return NULL; /* no file open */
}


/* very simple convenience function */
void utils_beep(void)
{
	if (prefs.beep_on_errors)
		gdk_beep();
}


/* taken from busybox, thanks */
gchar *utils_make_human_readable_str(guint64 size, gulong block_size,
									 gulong display_unit)
{
	/* The code will adjust for additional (appended) units. */
	static const gchar zero_and_units[] = { '0', 0, 'K', 'M', 'G', 'T' };
	static const gchar fmt[] = "%Lu %c%c";
	static const gchar fmt_tenths[] = "%Lu.%d %c%c";

	guint64 val;
	gint frac;
	const gchar *u;
	const gchar *f;

	u = zero_and_units;
	f = fmt;
	frac = 0;

	val = size * block_size;
	if (val == 0)
		return g_strdup(u);

	if (display_unit)
	{
		val += display_unit/2;	/* Deal with rounding. */
		val /= display_unit;	/* Don't combine with the line above!!! */
	}
	else
	{
		++u;
		while ((val >= 1024) && (u < zero_and_units + sizeof(zero_and_units) - 1))
		{
			f = fmt_tenths;
			++u;
			frac = ((((gint)(val % 1024)) * 10) + (1024 / 2)) / 1024;
			val /= 1024;
		}
		if (frac >= 10)
		{		/* We need to round up here. */
			++val;
			frac = 0;
		}
	}

	/* If f==fmt then 'frac' and 'u' are ignored. */
	return g_strdup_printf(f, val, frac, *u, 'b');
}


/* converts a color representation using gdk_color_parse(), with additional
 * support of the "0x" prefix as a synonym for "#" */
gboolean utils_parse_color(const gchar *spec, GdkColor *color)
{
	gchar buf[64] = {0};

	g_return_val_if_fail(spec != NULL, -1);

	if (spec[0] == '0' && (spec[1] == 'x' || spec[1] == 'X'))
	{
		/* convert to # format for GDK to understand it */
		buf[0] = '#';
		strncpy(buf + 1, spec + 2, sizeof(buf) - 2);
		spec = buf;
	}

	return gdk_color_parse(spec, color);
}


/* converts a GdkColor to the packed 24 bits BGR format, as understood by Scintilla
 * returns a 24 bits BGR color, or -1 on failure */
gint utils_color_to_bgr(const GdkColor *c)
{
	g_return_val_if_fail(c != NULL, -1);
	return (c->red / 256) | ((c->green / 256) << 8) | ((c->blue / 256) << 16);
}


/* parses @p spec using utils_parse_color() and convert it to 24 bits BGR using
 * utils_color_to_bgr() */
gint utils_parse_color_to_bgr(const gchar *spec)
{
	GdkColor color;
	if (utils_parse_color(spec, &color))
		return utils_color_to_bgr(&color);
	else
		return -1;
}


/* Returns: newly allocated string with the current time formatted HH:MM:SS. */
gchar *utils_get_current_time_string(void)
{
	const time_t tp = time(NULL);
	const struct tm *tmval = localtime(&tp);
	gchar *result = g_malloc0(9);

	strftime(result, 9, "%H:%M:%S", tmval);
	result[8] = '\0';
	return result;
}


GIOChannel *utils_set_up_io_channel(
				gint fd, GIOCondition cond, gboolean nblock, GIOFunc func, gpointer data)
{
	GIOChannel *ioc;
	/*const gchar *encoding;*/

	#ifdef G_OS_WIN32
	ioc = g_io_channel_win32_new_fd(fd);
	#else
	ioc = g_io_channel_unix_new(fd);
	#endif

	if (nblock)
		g_io_channel_set_flags(ioc, G_IO_FLAG_NONBLOCK, NULL);

	g_io_channel_set_encoding(ioc, NULL, NULL);
/*
	if (! g_get_charset(&encoding))
	{	// hope this works reliably
		GError *error = NULL;
		g_io_channel_set_encoding(ioc, encoding, &error);
		if (error)
		{
			geany_debug("%s: %s", G_STRFUNC, error->message);
			g_error_free(error);
			return ioc;
		}
	}
*/
	/* "auto-close" ;-) */
	g_io_channel_set_close_on_unref(ioc, TRUE);

	g_io_add_watch(ioc, cond, func, data);
	g_io_channel_unref(ioc);

	return ioc;
}


gchar **utils_read_file_in_array(const gchar *filename)
{
	gchar **result = NULL;
	gchar *data;

	g_return_val_if_fail(filename != NULL, NULL);

	g_file_get_contents(filename, &data, NULL, NULL);

	if (data != NULL)
	{
		result = g_strsplit_set(data, "\r\n", -1);
		g_free(data);
	}

	return result;
}


/* Contributed by Stefan Oltmanns, thanks.
 * Replaces \\, \r, \n, \t and \uXXX by their real counterparts.
 * keep_backslash is used for regex strings to leave '\\' and '\?' in place */
gboolean utils_str_replace_escape(gchar *string, gboolean keep_backslash)
{
	gsize i, j, len;
	guint unicodechar;

	g_return_val_if_fail(string != NULL, FALSE);

	j = 0;
	len = strlen(string);
	for (i = 0; i < len; i++)
	{
		if (string[i]=='\\')
		{
			if (i++ >= strlen(string))
			{
				return FALSE;
			}
			switch (string[i])
			{
				case '\\':
					if (keep_backslash)
						string[j++] = '\\';
					string[j] = '\\';
					break;
				case 'n':
					string[j] = '\n';
					break;
				case 'r':
					string[j] = '\r';
					break;
				case 't':
					string[j] = '\t';
					break;
#if 0
				case 'x': /* Warning: May produce illegal utf-8 string! */
					i += 2;
					if (i >= strlen(string))
					{
						return FALSE;
					}
					if (isdigit(string[i - 1])) string[j] = string[i - 1] - 48;
					else if (isxdigit(string[i - 1])) string[j] = tolower(string[i - 1])-87;
					else return FALSE;
					string[j] <<= 4;
					if (isdigit(string[i])) string[j] |= string[i] - 48;
					else if (isxdigit(string[i])) string[j] |= tolower(string[i])-87;
					else return FALSE;
					break;
#endif
				case 'u':
				{
					i += 2;
					if (i >= strlen(string))
					{
						return FALSE;
					}
					if (isdigit(string[i - 1])) unicodechar = string[i - 1] - 48;
					else if (isxdigit(string[i - 1])) unicodechar = tolower(string[i - 1])-87;
					else return FALSE;
					unicodechar <<= 4;
					if (isdigit(string[i])) unicodechar |= string[i] - 48;
					else if (isxdigit(string[i])) unicodechar |= tolower(string[i])-87;
					else return FALSE;
					if (((i + 2) < strlen(string)) && (isdigit(string[i + 1]) || isxdigit(string[i + 1]))
						&& (isdigit(string[i + 2]) || isxdigit(string[i + 2])))
					{
						i += 2;
						unicodechar <<= 8;
						if (isdigit(string[i - 1])) unicodechar |= ((string[i - 1] - 48) << 4);
						else unicodechar |= ((tolower(string[i - 1])-87) << 4);
						if (isdigit(string[i])) unicodechar |= string[i] - 48;
						else unicodechar |= tolower(string[i])-87;
					}
					if (((i + 2) < strlen(string)) && (isdigit(string[i + 1]) || isxdigit(string[i + 1]))
						&& (isdigit(string[i + 2]) || isxdigit(string[i + 2])))
					{
						i += 2;
						unicodechar <<= 8;
						if (isdigit(string[i - 1])) unicodechar |= ((string[i - 1] - 48) << 4);
						else unicodechar |= ((tolower(string[i - 1])-87) << 4);
						if (isdigit(string[i])) unicodechar |= string[i] - 48;
						else unicodechar |= tolower(string[i])-87;
					}
					if (unicodechar < 0x80)
					{
						string[j] = unicodechar;
					}
					else if (unicodechar < 0x800)
					{
						string[j] = (unsigned char) ((unicodechar >> 6) | 0xC0);
						j++;
						string[j] = (unsigned char) ((unicodechar & 0x3F) | 0x80);
					}
					else if (unicodechar < 0x10000)
					{
						string[j] = (unsigned char) ((unicodechar >> 12) | 0xE0);
						j++;
						string[j] = (unsigned char) (((unicodechar >> 6) & 0x3F) | 0x80);
						j++;
						string[j] = (unsigned char) ((unicodechar & 0x3F) | 0x80);
					}
					else if (unicodechar < 0x110000) /* more chars are not allowed in unicode */
					{
						string[j] = (unsigned char) ((unicodechar >> 18) | 0xF0);
						j++;
						string[j] = (unsigned char) (((unicodechar >> 12) & 0x3F) | 0x80);
						j++;
						string[j] = (unsigned char) (((unicodechar >> 6) & 0x3F) | 0x80);
						j++;
						string[j] = (unsigned char) ((unicodechar & 0x3F) | 0x80);
					}
					else
					{
						return FALSE;
					}
					break;
				}
				default:
					/* unnecessary escapes are allowed */
					if (keep_backslash)
						string[j++] = '\\';
					string[j] = string[i];
			}
		}
		else
		{
			string[j] = string[i];
		}
		j++;
	}
	while (j < i)
	{
		string[j] = 0;
		j++;
	}
	return TRUE;
}


/* Wraps a string in place, replacing a space with a newline character.
 * wrapstart is the minimum position to start wrapping or -1 for default */
gboolean utils_wrap_string(gchar *string, gint wrapstart)
{
	gchar *pos, *linestart;
	gboolean ret = FALSE;

	if (wrapstart < 0)
		wrapstart = 80;

	for (pos = linestart = string; *pos != '\0'; pos++)
	{
		if (pos - linestart >= wrapstart && *pos == ' ')
		{
			*pos = '\n';
			linestart = pos;
			ret = TRUE;
		}
	}
	return ret;
}


/**
 *  Converts the given UTF-8 encoded string into locale encoding.
 *  On Windows platforms, it does nothing and instead it just returns a copy of the input string.
 *
 *  @param utf8_text UTF-8 encoded text.
 *
 *  @return The converted string in locale encoding, or a copy of the input string if conversion
 *    failed. Should be freed with g_free(). If @a utf8_text is @c NULL, @c NULL is returned.
 **/
gchar *utils_get_locale_from_utf8(const gchar *utf8_text)
{
#ifdef G_OS_WIN32
	/* just do nothing on Windows platforms, this ifdef is just to prevent unwanted conversions
	 * which would result in wrongly converted strings */
	return g_strdup(utf8_text);
#else
	gchar *locale_text;

	if (! utf8_text)
		return NULL;
	locale_text = g_locale_from_utf8(utf8_text, -1, NULL, NULL, NULL);
	if (locale_text == NULL)
		locale_text = g_strdup(utf8_text);
	return locale_text;
#endif
}


/**
 *  Converts the given string (in locale encoding) into UTF-8 encoding.
 *  On Windows platforms, it does nothing and instead it just returns a copy of the input string.
 *
 *  @param locale_text Text in locale encoding.
 *
 *  @return The converted string in UTF-8 encoding, or a copy of the input string if conversion
 *    failed. Should be freed with g_free(). If @a locale_text is @c NULL, @c NULL is returned.
 **/
gchar *utils_get_utf8_from_locale(const gchar *locale_text)
{
#ifdef G_OS_WIN32
	/* just do nothing on Windows platforms, this ifdef is just to prevent unwanted conversions
	 * which would result in wrongly converted strings */
	return g_strdup(locale_text);
#else
	gchar *utf8_text;

	if (! locale_text)
		return NULL;
	utf8_text = g_locale_to_utf8(locale_text, -1, NULL, NULL, NULL);
	if (utf8_text == NULL)
		utf8_text = g_strdup(locale_text);
	return utf8_text;
#endif
}


/* Pass pointers to free after arg_count.
 * The last argument must be NULL as an extra check that arg_count is correct. */
void utils_free_pointers(gsize arg_count, ...)
{
	va_list a;
	gsize i;
	gpointer ptr;

	va_start(a, arg_count);
	for (i = 0; i < arg_count; i++)
	{
		ptr = va_arg(a, gpointer);
		g_free(ptr);
	}
	ptr = va_arg(a, gpointer);
	if (ptr)
		g_warning("Wrong arg_count!");
	va_end(a);
}


/* currently unused */
#if 0
/* Creates a string array deep copy of a series of non-NULL strings.
 * The first argument is nothing special.
 * The list must be ended with NULL.
 * If first is NULL, NULL is returned. */
gchar **utils_strv_new(const gchar *first, ...)
{
	gsize strvlen, i;
	va_list args;
	gchar *str;
	gchar **strv;

	g_return_val_if_fail(first != NULL, NULL);

	strvlen = 1;	/* for first argument */

	/* count other arguments */
	va_start(args, first);
	for (; va_arg(args, gchar*) != NULL; strvlen++);
	va_end(args);

	strv = g_new(gchar*, strvlen + 1);	/* +1 for NULL terminator */
	strv[0] = g_strdup(first);

	va_start(args, first);
	for (i = 1; str = va_arg(args, gchar*), str != NULL; i++)
	{
		strv[i] = g_strdup(str);
	}
	va_end(args);

	strv[i] = NULL;
	return strv;
}
#endif


/**
 *  Creates a directory if it doesn't already exist.
 *  Creates intermediate parent directories as needed, too.
 *  The permissions of the created directory are set 0700.
 *
 *  @param path The path of the directory to create, in locale encoding.
 *  @param create_parent_dirs Whether to create intermediate parent directories if necessary.
 *
 *  @return 0 if the directory was successfully created, otherwise the @c errno of the
 *          failed operation is returned.
 **/
gint utils_mkdir(const gchar *path, gboolean create_parent_dirs)
{
	gint mode = 0700;
	gint result;

	if (path == NULL || strlen(path) == 0)
		return EFAULT;

	result = (create_parent_dirs) ? g_mkdir_with_parents(path, mode) : g_mkdir(path, mode);
	if (result != 0)
		return errno;
	return 0;
}


/**
 * Gets a list of files from the specified directory.
 * Locale encoding is expected for @a path and used for the file list. The list and the data
 * in the list should be freed after use, e.g.:
 * @code
 * g_slist_foreach(list, (GFunc) g_free, NULL);
 * g_slist_free(list); @endcode
 *
 * @note If you don't need a list you should use the foreach_dir() macro instead -
 * it's more efficient.
 *
 * @param path The path of the directory to scan, in locale encoding.
 * @param full_path Whether to include the full path for each filename in the list. Obviously this
 * will use more memory.
 * @param sort Whether to sort alphabetically (UTF-8 safe).
 * @param error The location for storing a possible error, or @c NULL.
 *
 * @return A newly allocated list or @c NULL if no files were found. The list and its data should be
 * freed when no longer needed.
 * @see utils_get_file_list().
 **/
GSList *utils_get_file_list_full(const gchar *path, gboolean full_path, gboolean sort, GError **error)
{
	GSList *list = NULL;
	GDir *dir;
	const gchar *filename;

	if (error)
		*error = NULL;
	g_return_val_if_fail(path != NULL, NULL);

	dir = g_dir_open(path, 0, error);
	if (dir == NULL)
		return NULL;

	foreach_dir(filename, dir)
	{
		list = g_slist_prepend(list, full_path ?
			g_build_path(G_DIR_SEPARATOR_S, path, filename, NULL) : g_strdup(filename));
	}
	g_dir_close(dir);
	/* sorting last is quicker than on insertion */
	if (sort)
		list = g_slist_sort(list, (GCompareFunc) utils_str_casecmp);
	return list;
}


/**
 * Gets a sorted list of files from the specified directory.
 * Locale encoding is expected for @a path and used for the file list. The list and the data
 * in the list should be freed after use, e.g.:
 * @code
 * g_slist_foreach(list, (GFunc) g_free, NULL);
 * g_slist_free(list); @endcode
 *
 * @param path The path of the directory to scan, in locale encoding.
 * @param length The location to store the number of non-@c NULL data items in the list,
 *               unless @c NULL.
 * @param error The location for storing a possible error, or @c NULL.
 *
 * @return A newly allocated list or @c NULL if no files were found. The list and its data should be
 *         freed when no longer needed.
 * @see utils_get_file_list_full().
 **/
GSList *utils_get_file_list(const gchar *path, guint *length, GError **error)
{
	GSList *list = utils_get_file_list_full(path, FALSE, TRUE, error);

	if (length)
		*length = g_slist_length(list);
	return list;
}


/* returns TRUE if any letter in str is a capital, FALSE otherwise. Should be Unicode safe. */
gboolean utils_str_has_upper(const gchar *str)
{
	gunichar c;

	if (EMPTY(str) || ! g_utf8_validate(str, -1, NULL))
		return FALSE;

	while (*str != '\0')
	{
		c = g_utf8_get_char(str);
		/* check only letters and stop once the first non-capital was found */
		if (g_unichar_isalpha(c) && g_unichar_isupper(c))
			return TRUE;
		/* FIXME don't write a const string */
		str = g_utf8_next_char(str);
	}
	return FALSE;
}


/* end can be -1 for haystack->len.
 * returns: position of found text or -1. */
gint utils_string_find(GString *haystack, gint start, gint end, const gchar *needle)
{
	gint pos;

	g_return_val_if_fail(haystack != NULL, -1);
	if (haystack->len == 0)
		return -1;

	g_return_val_if_fail(start >= 0, -1);
	if (start >= (gint)haystack->len)
		return -1;

	g_return_val_if_fail(!EMPTY(needle), -1);

	if (end < 0)
		end = haystack->len;

	pos = utils_strpos(haystack->str + start, needle);
	if (pos == -1)
		return -1;

	pos += start;
	if (pos >= end)
		return -1;
	return pos;
}


/* Replaces @len characters from offset @a pos.
 * len can be -1 to replace the remainder of @a str.
 * returns: pos + strlen(replace). */
gint utils_string_replace(GString *str, gint pos, gint len, const gchar *replace)
{
	g_string_erase(str, pos, len);
	if (replace)
	{
		g_string_insert(str, pos, replace);
		pos += strlen(replace);
	}
	return pos;
}


/**
 * Replaces all occurrences of @a needle in @a haystack with @a replace.
 * As of Geany 0.16, @a replace can match @a needle, so the following will work:
 * @code utils_string_replace_all(text, "\n", "\r\n"); @endcode
 *
 * @param haystack The input string to operate on. This string is modified in place.
 * @param needle The string which should be replaced.
 * @param replace The replacement for @a needle.
 *
 * @return Number of replacements made.
 **/
guint utils_string_replace_all(GString *haystack, const gchar *needle, const gchar *replace)
{
	guint count = 0;
	gint pos = 0;
	gsize needle_length = strlen(needle);

	while (1)
	{
		pos = utils_string_find(haystack, pos, -1, needle);

		if (pos == -1)
			break;

		pos = utils_string_replace(haystack, pos, needle_length, replace);
		count++;
	}
	return count;
}


/**
 * Replaces only the first occurrence of @a needle in @a haystack
 * with @a replace.
 * For details, see utils_string_replace_all().
 *
 * @param haystack The input string to operate on. This string is modified in place.
 * @param needle The string which should be replaced.
 * @param replace The replacement for @a needle.
 *
 * @return Number of replacements made.
 *
 *  @since 0.16
 */
guint utils_string_replace_first(GString *haystack, const gchar *needle, const gchar *replace)
{
	gint pos = utils_string_find(haystack, 0, -1, needle);

	if (pos == -1)
		return 0;

	utils_string_replace(haystack, pos, strlen(needle), replace);
	return 1;
}


/* Similar to g_regex_replace but allows matching a subgroup.
 * match_num: which match to replace, 0 for whole match.
 * literal: FALSE to interpret escape sequences in @a replace.
 * returns: number of replacements.
 * bug: replaced text can affect matching of ^ or \b */
guint utils_string_regex_replace_all(GString *haystack, GRegex *regex,
		guint match_num, const gchar *replace, gboolean literal)
{
	GMatchInfo *minfo;
	guint ret = 0;
	gint start = 0;

	g_assert(literal); /* escapes not implemented yet */
	g_return_val_if_fail(replace, 0);

	/* ensure haystack->str is not null */
	if (haystack->len == 0)
		return 0;

	/* passing a start position makes G_REGEX_MATCH_NOTBOL automatic */
	while (g_regex_match_full(regex, haystack->str, -1, start, 0, &minfo, NULL))
	{
		gint end, len;

		g_match_info_fetch_pos(minfo, match_num, &start, &end);
		len = end - start;
		utils_string_replace(haystack, start, len, replace);
		ret++;

		/* skip past whole match */
		g_match_info_fetch_pos(minfo, 0, NULL, &end);
		start = end - len + strlen(replace);
		g_match_info_free(minfo);
	}
	g_match_info_free(minfo);
	return ret;
}


/* Get project or default startup directory (if set), or NULL. */
const gchar *utils_get_default_dir_utf8(void)
{
	if (app->project && !EMPTY(app->project->base_path))
	{
		return app->project->base_path;
	}

	if (!EMPTY(prefs.default_open_path))
	{
		return prefs.default_open_path;
	}
	return NULL;
}


/**
 *  Wraps g_spawn_sync() and internally calls this function on Unix-like
 *  systems. On Win32 platforms, it uses the Windows API.
 *
 *  @param dir The child's current working directory, or @a NULL to inherit parent's.
 *  @param argv The child's argument vector.
 *  @param env The child's environment, or @a NULL to inherit parent's.
 *  @param flags Flags from GSpawnFlags.
 *  @param child_setup A function to run in the child just before exec().
 *  @param user_data The user data for child_setup.
 *  @param std_out The return location for child output.
 *  @param std_err The return location for child error messages.
 *  @param exit_status The child exit status, as returned by waitpid().
 *  @param error The return location for error or @a NULL.
 *
 *  @return @c TRUE on success, @c FALSE if an error was set.
 **/
gboolean utils_spawn_sync(const gchar *dir, gchar **argv, gchar **env, GSpawnFlags flags,
						  GSpawnChildSetupFunc child_setup, gpointer user_data, gchar **std_out,
						  gchar **std_err, gint *exit_status, GError **error)
{
	gboolean result;

	if (argv == NULL)
	{
		g_set_error(error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED, "argv must not be NULL");
		return FALSE;
	}

	if (std_out)
		*std_out = NULL;

	if (std_err)
		*std_err = NULL;

#ifdef G_OS_WIN32
	result = win32_spawn(dir, argv, env, flags, std_out, std_err, exit_status, error);
#else
	// calling g_spawn_sync after previously calling g_spawn_async messes up the SIGCHLD signal and it will never return the correct exit status
	result = g_spawn_sync(dir, argv, env, flags, NULL, NULL, std_out, std_err, exit_status, error);
	
	/*
	// but this workaround freezes the app
	GPid child_pid = 0;
	gint stdout, stderr;
	result = g_spawn_async_with_pipes(dir, argv, env, flags | G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &child_pid, NULL, &stdout, &stderr, error);
	if ( !result ) 
	{
		return FALSE;
	}
	else
	{
		if ( waitpid( child_pid, exit_status, 0 ) < 0 )
		{
			if ( errno == ECHILD )
			{
				g_set_error(error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED, "failed to get exit status");
				g_spawn_close_pid(child_pid);
				return FALSE;
			}
			else
			{
				g_set_error(error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED, "failed to wait for child process");
				g_spawn_close_pid(child_pid);
				return FALSE;
			}
		}
		else
		{
			int flags = fcntl( stdout, F_GETFL, 0 );
			fcntl( stdout, F_SETFL, flags | O_NONBLOCK );

			*std_out = g_strdup("");

			gchar buffer[ 256 ];
			int result = read( stdout, buffer, 256 );
			while( result > 0 )
			{
				SETPTR( *std_out, g_strconcat(*std_out,buffer) );
			}

			if ( result < 0 )
			{
				g_set_error(error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED, "failed to read stdout from child process");
				g_spawn_close_pid(child_pid);
				return FALSE;
			}
		}

		g_spawn_close_pid(child_pid);
	}
	result = TRUE;
	*/

#endif

	return result;
}


/**
 *  Wraps g_spawn_async() and internally calls this function on Unix-like
 *  systems. On Win32 platforms, it uses the Windows API.
 *
 *  @param dir The child's current working directory, or @a NULL to inherit parent's.
 *  @param argv The child's argument vector.
 *  @param env The child's environment, or @a NULL to inherit parent's.
 *  @param flags Flags from GSpawnFlags.
 *  @param child_setup A function to run in the child just before exec().
 *  @param user_data The user data for child_setup.
 *  @param child_pid The return location for child process ID, or NULL.
 *  @param error The return location for error or @a NULL.
 *
 *  @return @c TRUE on success, @c FALSE if an error was set.
 **/
gboolean utils_spawn_async(const gchar *dir, gchar **argv, gchar **env, GSpawnFlags flags,
						   GSpawnChildSetupFunc child_setup, gpointer user_data, GPid *child_pid,
						   GError **error)
{
	gboolean result;

	if (argv == NULL)
	{
		g_set_error(error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED, "argv must not be NULL");
		return FALSE;
	}

#ifdef G_OS_WIN32
	result = win32_spawn_async(dir, argv, env, flags, NULL, NULL, NULL, error);
#else
	result = g_spawn_async(dir, argv, env, flags, NULL, NULL, child_pid, error);
#endif
	return result;
}


/* Retrieves the path for the given URI.
 * It returns:
 * - the path which was determined by g_filename_from_uri() or GIO
 * - NULL if the URI is non-local and gvfs-fuse is not installed
 * - a new copy of 'uri' if it is not an URI. */
gchar *utils_get_path_from_uri(const gchar *uri)
{
	gchar *locale_filename;

	g_return_val_if_fail(uri != NULL, NULL);

	if (! utils_is_uri(uri))
		return g_strdup(uri);

	/* this will work only for 'file://' URIs */
	locale_filename = g_filename_from_uri(uri, NULL, NULL);
	/* g_filename_from_uri() failed, so we probably have a non-local URI */
	if (locale_filename == NULL)
	{
		GFile *file = g_file_new_for_uri(uri);
		locale_filename = g_file_get_path(file);
		g_object_unref(file);
		if (locale_filename == NULL)
		{
			geany_debug("The URI '%s' could not be resolved to a local path. This means "
				"that the URI is invalid or that you don't have gvfs-fuse installed.", uri);
		}
	}

	return locale_filename;
}


gboolean utils_is_uri(const gchar *uri)
{
	g_return_val_if_fail(uri != NULL, FALSE);

	return (strstr(uri, "://") != NULL);
}


/* path should be in locale encoding */
gboolean utils_is_remote_path(const gchar *path)
{
	g_return_val_if_fail(path != NULL, FALSE);

	/* if path is an URI and it doesn't start "file://", we take it as remote */
	if (utils_is_uri(path) && strncmp(path, "file:", 5) != 0)
		return TRUE;

#ifndef G_OS_WIN32
	{
		static gchar *fuse_path = NULL;
		static gsize len = 0;

		if (G_UNLIKELY(fuse_path == NULL))
		{
			fuse_path = g_build_filename(g_get_home_dir(), ".gvfs", NULL);
			len = strlen(fuse_path);
		}
		/* Comparing the file path against a hardcoded path is not the most elegant solution
		 * but for now it is better than nothing. Ideally, g_file_new_for_path() should create
		 * proper GFile objects for Fuse paths, but it only does in future GVFS
		 * versions (gvfs 1.1.1). */
		return (strncmp(path, fuse_path, len) == 0);
	}
#endif

	return FALSE;
}


/* Remove all relative and untidy elements from the path of @a filename.
 * @param filename must be a valid absolute path.
 * @see tm_get_real_path() - also resolves links. */
void utils_tidy_path(gchar *filename)
{
	GString *str;
	const gchar *needle;
	gboolean preserve_double_backslash = FALSE;

	g_return_if_fail(g_path_is_absolute(filename));

	str = g_string_new(filename);

	if (str->len >= 2 && strncmp(str->str, "\\\\", 2) == 0)
		preserve_double_backslash = TRUE;

#ifdef G_OS_WIN32
	/* using MSYS we can get Unix-style separators */
	utils_string_replace_all(str, "/", G_DIR_SEPARATOR_S);
#endif
	/* replace "/./" and "//" */
	utils_string_replace_all(str, G_DIR_SEPARATOR_S "." G_DIR_SEPARATOR_S, G_DIR_SEPARATOR_S);
	utils_string_replace_all(str, G_DIR_SEPARATOR_S G_DIR_SEPARATOR_S, G_DIR_SEPARATOR_S);

	if (preserve_double_backslash)
		g_string_prepend(str, "\\");

	/* replace "/../" */
	needle = G_DIR_SEPARATOR_S ".." G_DIR_SEPARATOR_S;
	while (1)
	{
		const gchar *c = strstr(str->str, needle);
		if (c == NULL)
			break;
		else
		{
			gssize pos, sub_len;

			pos = c - str->str;
			if (pos <= 3)
				break;	/* bad path */

			/* replace "/../" */
			g_string_erase(str, pos, strlen(needle));
			g_string_insert_c(str, pos, G_DIR_SEPARATOR);
	
			/* search for last "/" before found "/../" */
			c = g_strrstr_len(str->str, pos, G_DIR_SEPARATOR_S);
			sub_len = pos - (c - str->str);
			if (! c)
				break;	/* bad path */

			pos = c - str->str;	/* position of previous "/" */
			g_string_erase(str, pos, sub_len);
		}
	}
	if (str->len <= strlen(filename))
		memcpy(filename, str->str, str->len + 1);
	else
		g_warn_if_reached();
	g_string_free(str, TRUE);
}


/**
 *  Removes characters from a string, in place.
 *
 *  @param string String to search.
 *  @param chars Characters to remove.
 *
 *  @return @a string - return value is only useful when nesting function calls, e.g.:
 *  @code str = utils_str_remove_chars(g_strdup("f_o_o"), "_"); @endcode
 *
 *  @see @c g_strdelimit.
 **/
gchar *utils_str_remove_chars(gchar *string, const gchar *chars)
{
	const gchar *r;
	gchar *w = string;

	g_return_val_if_fail(string, NULL);
	if (G_UNLIKELY(EMPTY(chars)))
		return string;

	foreach_str(r, string)
	{
		if (!strchr(chars, *r))
			*w++ = *r;
	}
	*w = 0x0;
	return string;
}


/* Gets list of sorted filenames with no path and no duplicates from user and system config */
GSList *utils_get_config_files(const gchar *subdir)
{
	gchar *path = g_build_path(G_DIR_SEPARATOR_S, app->configdir, subdir, NULL);
	GSList *list = utils_get_file_list_full(path, FALSE, FALSE, NULL);
	GSList *syslist, *node;

	if (!list)
	{
		utils_mkdir(path, FALSE);
	}
	SETPTR(path, g_build_path(G_DIR_SEPARATOR_S, app->datadir, subdir, NULL));
	syslist = utils_get_file_list_full(path, FALSE, FALSE, NULL);
	/* merge lists */
	list = g_slist_concat(list, syslist);

	list = g_slist_sort(list, (GCompareFunc) utils_str_casecmp);
	/* remove duplicates (next to each other after sorting) */
	foreach_slist(node, list)
	{
		if (node->next && utils_str_equal(node->next->data, node->data))
		{
			GSList *old = node->next;

			g_free(old->data);
			node->next = old->next;
			g_slist_free1(old);
		}
	}
	g_free(path);
	return list;
}


/* Suffix can be NULL or a string which should be appended to the Help URL like
 * an anchor link, e.g. "#some_anchor". */
gchar *utils_get_help_url(const gchar *suffix)
{
	gint skip;
	gchar *uri;

#ifdef G_OS_WIN32
	skip = 8;
	uri = g_strconcat("file:///", win32_get_installation_dir(), "/../Help/home.html", NULL);
	g_strdelimit(uri, "\\", '/'); /* replace '\\' by '/' */
#elif __APLLE__
	skip = 7;
	uri = g_strconcat("file://", app->datadir, "/../Help/home.html", NULL);
#else
	skip = 7;
	uri = g_strconcat("file://", app->datadir, "/../../../Help/home.html", NULL);
#endif

	//geany_debug( "%s", uri );

	if (! g_file_test(uri + skip, G_FILE_TEST_IS_REGULAR))
	{	/* fall back to online documentation if it is not found on the hard disk */
		g_free(uri);
		uri = g_strconcat(GEANY_HOMEPAGE, "documentation/home.html", NULL);
	}

	if (suffix != NULL)
	{
		SETPTR(uri, g_strconcat(uri, suffix, NULL));
	}

	return uri;
}


static gboolean str_in_array(const gchar **haystack, const gchar *needle)
{
	const gchar **p;

	for (p = haystack; *p != NULL; ++p)
	{
		if (utils_str_equal(*p, needle))
			return TRUE;
	}
	return FALSE;
}


/**
 * Copies the current environment into a new array.
 * @a exclude_vars is a @c NULL-terminated array of variable names which should be not copied.
 * All further arguments are key, value pairs of variables which should be added to
 * the environment.
 *
 * The argument list must be @c NULL-terminated.
 *
 * @param exclude_vars @c NULL-terminated array of variable names to exclude.
 * @param first_varname Name of the first variable to copy into the new array.
 * @param ... Key-value pairs of variable names and values, @c NULL-terminated.
 *
 * @return The new environment array.
 **/
gchar **utils_copy_environment(const gchar **exclude_vars, const gchar *first_varname, ...)
{
	gchar **result;
	gchar **p;
	gchar **env;
	va_list args;
	const gchar *key, *value;
	guint n, o;

	/* count the additional variables */
	va_start(args, first_varname);
	for (o = 1; va_arg(args, gchar*) != NULL; o++);
	va_end(args);
	/* the passed arguments should be even (key, value pairs) */
	g_return_val_if_fail(o % 2 == 0, NULL);

	o /= 2;

	/* get all the environ variables */
	env = g_listenv();

	/* create an array large enough to hold the new environment */
	n = g_strv_length(env);
	/* 'n + o + 1' could leak a little bit when exclude_vars is set */
	result = g_new(gchar *, n + o + 1);

	/* copy the environment */
	for (n = 0, p = env; *p != NULL; ++p)
	{
		/* copy the variable */
		value = g_getenv(*p);
		if (G_LIKELY(value != NULL))
		{
			/* skip excluded variables */
			if (exclude_vars != NULL && str_in_array(exclude_vars, *p))
				continue;

			result[n++] = g_strconcat(*p, "=", value, NULL);
		}
	}
	g_strfreev(env);

	/* now add additional variables */
	va_start(args, first_varname);
	key = first_varname;
	value = va_arg(args, gchar*);
	while (key != NULL)
	{
		result[n++] = g_strconcat(key, "=", value, NULL);

		key = va_arg(args, gchar*);
		if (key == NULL)
			break;
		value = va_arg(args, gchar*);
	}
	va_end(args);

	result[n] = NULL;

	return result;
}


/* Joins @a first and @a second into a new string vector, freeing the originals.
 * The original contents are reused. */
gchar **utils_strv_join(gchar **first, gchar **second)
{
	gchar **strv;
	gchar **rptr, **wptr;

	if (!first)
		return second;
	if (!second)
		return first;

	strv = g_new0(gchar*, g_strv_length(first) + g_strv_length(second) + 1);
	wptr = strv;

	foreach_strv(rptr, first)
		*wptr++ = *rptr;
	foreach_strv(rptr, second)
		*wptr++ = *rptr;

	g_free(first);
	g_free(second);
	return strv;
}


/* Try to parse a date using g_date_set_parse(). It doesn't take any format hint,
 * obviously g_date_set_parse() uses some magic.
 * The returned GDate object must be freed. */
GDate *utils_parse_date(const gchar *input)
{
	GDate *date = g_date_new();

	g_date_set_parse(date, input);

	if (g_date_valid(date))
		return date;

	g_date_free(date);
	return NULL;
}


gchar *utils_parse_and_format_build_date(const gchar *input)
{
	gchar date_buf[255];
	GDate *date = utils_parse_date(input);

	if (date != NULL)
	{
		g_date_strftime(date_buf, sizeof(date_buf), GEANY_TEMPLATES_FORMAT_DATE, date);
		g_date_free(date);
		return g_strdup(date_buf);
	}

	return g_strdup(input);
}

gboolean utils_copy_file (const gchar *src, const gchar *dst, gboolean overwrite, volatile gchar* progress)
{
    g_return_val_if_fail (src != NULL, FALSE);
    g_return_val_if_fail (dst != NULL, FALSE);

    /* check source file */
    if (!g_file_test (src, G_FILE_TEST_EXISTS)) {
        if ( progress ) sprintf( (gchar*)progress, "File '%s' not found", src );
        else g_critical (G_STRLOC ": File '%s' not found.", src);
        return FALSE;
    }
    else if (!g_file_test (src, G_FILE_TEST_IS_REGULAR)) {
        if ( progress ) sprintf( (gchar*)progress, "'%s' not a regular file.", src );
        else g_critical (G_STRLOC ": '%s' not a regular file.", src);
        return FALSE;
    }
    
#ifndef G_OS_WIN32
    if (g_file_test (src, G_FILE_TEST_IS_SYMLINK)) {
        gchar* link = g_file_read_link( src, NULL );
        if ( link )
        {
            symlink( link, dst );
            g_free(link);
        }
        return TRUE;
    }
#endif


    /* check the destination */
    if (g_file_test (dst, G_FILE_TEST_EXISTS)) {

        if (!overwrite) {
            if ( progress ) sprintf( (gchar*)progress, "Destination file '%s' exists and overwrite is disabled.", dst );
            else g_critical (G_STRLOC ": Destination file '%s' exists and overwrite is disabled.", dst);
            return FALSE;
        }

        /* remove dest fileif overwrite is enabled */
        if (g_unlink (dst) != 0) {
            if ( progress ) sprintf( (gchar*)progress, "%s", g_strerror (errno) );
            else g_critical (G_STRLOC ": %s.", g_strerror (errno));
            return FALSE;
        }
    }

	gchar *contents = 0;
	gsize length = 0;

	if ( !g_file_get_contents( src, &contents, &length, NULL ) )
	{
        if ( progress ) sprintf( (gchar*)progress, "Unable to open '%s' for reading. %s.", src, g_strerror (errno) );
        else g_critical (G_STRLOC ": Unable to open '%s' for reading. %s.", src, g_strerror (errno));
        return FALSE;
	}

	if ( !g_file_set_contents( dst, contents, length, NULL ) )
	{
		g_free(contents);
        if ( progress ) sprintf( (gchar*)progress, "Unable to open '%s' for writing. %s.", dst, g_strerror (errno) );
        else g_critical (G_STRLOC ": Unable to open '%s' for writing. %s.", dst, g_strerror (errno));
        return FALSE;
	}
    
	g_free(contents);
    
    return TRUE;
}

gboolean utils_copy_folder ( const gchar* src, const gchar* dst, gboolean recursive, volatile gchar* progress )
{
	g_return_val_if_fail (src != NULL, FALSE);
    g_return_val_if_fail (dst != NULL, FALSE);

	if (!g_file_test (src, G_FILE_TEST_EXISTS)) {
        if ( progress ) sprintf( (gchar*)progress, "Location '%s' not found.", src );
        else g_critical (G_STRLOC ": Location '%s' not found.", src);
        return FALSE;
    }

	if (!g_file_test (src, G_FILE_TEST_IS_DIR)) {
        if ( progress ) sprintf( (gchar*)progress, "Location '%s' is not a directory.", src );
        else g_critical (G_STRLOC ": Location '%s' is not a directory.", src);
        return FALSE;
    }
    
#ifndef G_OS_WIN32
    if (g_file_test (src, G_FILE_TEST_IS_SYMLINK)) {
        gchar* link = g_file_read_link( src, NULL );
        if ( link )
        {
            symlink( link, dst );
            g_free( link );
        }
        return TRUE;
    }
#endif


	if (!g_file_test (dst, G_FILE_TEST_EXISTS)) {
        if ( g_mkdir_with_parents( dst, 0755 ) < 0 )
		{
            if ( progress ) sprintf( (gchar*)progress, "Failed to make destination directory '%s'", dst );
            else g_critical (G_STRLOC ": Failed to make destination directory '%s'", dst);
			return FALSE;
		}
    }

	// scan the directory
	const gchar *filename;
	GDir *dir = g_dir_open(src, 0, NULL);
	if (dir == NULL)
	{
        if ( progress ) sprintf( (gchar*)progress, "Failed to open directory '%s'", src );
        else g_critical (G_STRLOC ": Failed to open directory '%s'", src);
		return FALSE;
	}

	foreach_dir(filename, dir)
	{
		gchar* fullsrcpath = g_build_filename( src, filename, NULL );
		gchar* fulldstpath = g_build_filename( dst, filename, NULL );

		if ( g_file_test( fullsrcpath, G_FILE_TEST_IS_DIR ) )
		{
			if ( recursive )
			{
				if ( !utils_copy_folder( fullsrcpath, fulldstpath, recursive, progress ) ) 
				{
					g_dir_close(dir);
					g_free(fullsrcpath);
					g_free(fulldstpath);
					return FALSE;
				}
			}
		}
		else if ( g_file_test( fullsrcpath, G_FILE_TEST_IS_REGULAR ) )
		{
			if ( progress ) strcpy( progress, fulldstpath );
			if ( !utils_copy_file( fullsrcpath, fulldstpath, TRUE, progress ) )
			{
				g_dir_close(dir);
				g_free(fullsrcpath);
				g_free(fulldstpath);
				return FALSE;
			}
		}

		g_free(fullsrcpath);
		g_free(fulldstpath);
	}

	g_dir_close(dir);
	
	return TRUE;
}

gboolean utils_remove_folder_recursive ( const gchar* src )
{
	g_return_val_if_fail (src != NULL, FALSE);
	if ( strcmp(src,"/") == 0 ) { g_critical (G_STRLOC ": Delete attempt on '%s' ignored.", src); return FALSE; } // don't delete root
	if ( strcmp(src,"C:") == 0 ) { g_critical (G_STRLOC ": Delete attempt on '%s' ignored.", src); return FALSE; } // don't delete root
	if ( strcmp(src,"C:\\") == 0 ) { g_critical (G_STRLOC ": Delete attempt on '%s' ignored.", src); return FALSE; } // don't delete root

	if (!g_file_test (src, G_FILE_TEST_EXISTS)) {
        return TRUE;
    }

	if (!g_file_test (src, G_FILE_TEST_IS_DIR)) {
        g_critical (G_STRLOC ": Location '%s' is not a directory.", src);
        return FALSE;
    }

	// scan the directory
	const gchar *filename;
	GDir *dir = g_dir_open(src, 0, NULL);
	if (dir == NULL)
	{
		g_critical (G_STRLOC ": Failed to open directory '%s'", src);
		return FALSE;
	}

	foreach_dir(filename, dir)
	{
		gchar* fullsrcpath = g_build_filename( src, filename, NULL );

		if ( g_file_test( fullsrcpath, G_FILE_TEST_IS_DIR ) )
		{
			if ( !utils_remove_folder_recursive( fullsrcpath ) ) 
			{
				g_dir_close(dir);
				g_free(fullsrcpath);
				return FALSE;
			}
		}
		else if ( g_file_test( fullsrcpath, G_FILE_TEST_IS_REGULAR ) )
		{
			if ( g_unlink( fullsrcpath ) != 0 ) 
			{
				g_dir_close(dir);
				g_free(fullsrcpath);
				return FALSE;
			}
		}

		g_free(fullsrcpath);
	}

	g_dir_close(dir);

	g_rmdir( src );
	
	return TRUE;
}

gboolean utils_add_folder_to_zip ( mz_zip_archive *pZip, const gchar* src, const gchar* dst, gboolean recursive, gboolean selective_compress )
{
	g_return_val_if_fail (pZip != NULL, FALSE);
	g_return_val_if_fail (src != NULL, FALSE);
    g_return_val_if_fail (dst != NULL, FALSE);

	if (!g_file_test (src, G_FILE_TEST_EXISTS)) {
        g_critical (G_STRLOC ": Location '%s' not found.", src);
        return FALSE;
    }

	if (!g_file_test (src, G_FILE_TEST_IS_DIR)) {
        g_critical (G_STRLOC ": Location '%s' is not a directory.", src);
        return FALSE;
    }

	// scan the directory
	const gchar *filename;
	GDir *dir = g_dir_open(src, 0, NULL);
	if (dir == NULL)
	{
		g_critical (G_STRLOC ": Failed to open directory '%s'", src);
		return FALSE;
	}

	foreach_dir(filename, dir)
	{
		gchar* fullsrcpath = g_build_path( "/", src, filename, NULL );
		gchar* fulldstpath = g_build_path( "/", dst, filename, NULL );
		
		//dialogs_show_msgbox ( GTK_MESSAGE_ERROR, fullsrcpath );
		//dialogs_show_msgbox ( GTK_MESSAGE_ERROR, fulldstpath );
		//dialogs_show_msgbox ( GTK_MESSAGE_ERROR, filename );

		if ( g_file_test( fullsrcpath, G_FILE_TEST_IS_DIR ) )
		{
			if ( recursive )
			{
				if ( !utils_add_folder_to_zip( pZip, fullsrcpath, fulldstpath, recursive, selective_compress ) ) 
				{
					g_dir_close(dir);
					g_free(fullsrcpath);
					g_free(fulldstpath);
					//dialogs_show_msgbox ( GTK_MESSAGE_ERROR, "fail a" );
					return FALSE;
				}
			}
		}
		else if ( g_file_test( fullsrcpath, G_FILE_TEST_IS_REGULAR ) )
		{
			int level = 9;
			gchar *ext = strrchr( filename, '.' );
			if ( ext && selective_compress )
			{
				if ( utils_str_casecmp( ext, ".mp3" ) == 0 ) level = 0;
				if ( utils_str_casecmp( ext, ".m4a" ) == 0 ) level = 0;
				if ( utils_str_casecmp( ext, ".jpg" ) == 0 ) level = 0;
				if ( utils_str_casecmp( ext, ".png" ) == 0 ) level = 0;
				if ( utils_str_casecmp( ext, ".gif" ) == 0 ) level = 0;
				if ( utils_str_casecmp( ext, ".wav" ) == 0 ) level = 0;
				if ( utils_str_casecmp( ext, ".ogg" ) == 0 ) level = 0;
				if ( utils_str_casecmp( ext, ".mpg" ) == 0 ) level = 0;
				if ( utils_str_casecmp( ext, ".mpeg" ) == 0 ) level = 0;
				if ( utils_str_casecmp( ext, ".mp4" ) == 0 ) level = 0;
				if ( utils_str_casecmp( ext, ".m4v" ) == 0 ) level = 0;
				if ( utils_str_casecmp( ext, ".dat" ) == 0 ) level = 0;
				if ( utils_str_casecmp( ext, ".zip" ) == 0 ) level = 0;
			}

			if ( !mz_zip_writer_add_file( pZip, fulldstpath, fullsrcpath, NULL, 0, level ) )
			{
				g_dir_close(dir);
				g_free(fullsrcpath);
				g_free(fulldstpath);
				//dialogs_show_msgbox ( GTK_MESSAGE_ERROR, "fail b" );
				return FALSE;
			}
		}

		g_free(fullsrcpath);
		g_free(fulldstpath);
	}

	g_dir_close(dir);
	
	return TRUE;
}

gboolean utils_add_folder_to_html5_data_file( FILE *pHTML5Data, const gchar* srcfull, const gchar* src, gchar* load_package_string, gchar* additional_folders_string, int* currpos )
{
	g_return_val_if_fail (pHTML5Data != NULL, FALSE);
	g_return_val_if_fail (srcfull != NULL, FALSE);
	g_return_val_if_fail (src != NULL, FALSE);
    g_return_val_if_fail (load_package_string != NULL, FALSE);
	g_return_val_if_fail (additional_folders_string != NULL, FALSE);
	g_return_val_if_fail (currpos != NULL, FALSE);

	if (!g_file_test (srcfull, G_FILE_TEST_EXISTS)) {
        g_critical (G_STRLOC ": Location '%s' not found.", srcfull);
        return FALSE;
    }

	if (!g_file_test (srcfull, G_FILE_TEST_IS_DIR)) {
        g_critical (G_STRLOC ": Location '%s' is not a directory.", srcfull);
        return FALSE;
    }

	// scan the directory
	const gchar *filename;
	GDir *dir = g_dir_open(srcfull, 0, NULL);
	if (dir == NULL)
	{
		g_critical (G_STRLOC ": Failed to open directory '%s'", srcfull);
		return FALSE;
	}

	foreach_dir(filename, dir)
	{
		gchar* filepath = g_build_path( "/", srcfull, filename, NULL );
		gchar* shortfilepath = g_build_path( "/", src, filename, NULL );
			
		if ( g_file_test( filepath, G_FILE_TEST_IS_DIR ) )
		{
			strcat( additional_folders_string, "Module[\"FS_createPath\"](\"" ); 
			strcat( additional_folders_string, src ); 
			strcat( additional_folders_string, "\", \"" );
			strcat( additional_folders_string, filename );
			strcat( additional_folders_string, "\", true, true);" );

			if ( !utils_add_folder_to_html5_data_file( pHTML5Data, filepath, shortfilepath, load_package_string, additional_folders_string, currpos ) ) 
			{
				g_dir_close(dir);
				g_free(filepath);
				g_free(shortfilepath);
				return FALSE;
			}
		}
		else if ( g_file_test( filepath, G_FILE_TEST_IS_REGULAR ) )
		{
			gchar *contents = 0;
			gsize length = 0;

			if ( !g_file_get_contents( filepath, &contents, &length, NULL ) )
			{
				g_dir_close(dir);
				g_free(filepath);
				g_free(shortfilepath);
				if ( contents ) g_free(contents);

				g_critical (G_STRLOC ": Failed to open file '%s'", filepath);
				return FALSE;
			}
			
			if ( length > 0 && fwrite( contents, 1, length, pHTML5Data ) != length )
			{
				g_dir_close(dir);
				g_free(filepath);
				g_free(shortfilepath);
				g_free(contents);

				g_critical (G_STRLOC ": Failed to convert file '%s' to HTML5 data", filepath);
				return FALSE;
			}

			g_free(contents);

			int audio = 0;
			gchar *ext = strrchr( filename, '.' );
			if ( ext )
			{
				if ( utils_str_casecmp( ext, ".mp3" ) == 0 ) audio = 1;
				if ( utils_str_casecmp( ext, ".m4a" ) == 0 ) audio = 1;
				if ( utils_str_casecmp( ext, ".wav" ) == 0 ) audio = 1;
				if ( utils_str_casecmp( ext, ".ogg" ) == 0 ) audio = 1;
			}

			// append file data to load packing string
			char str[ 30 ];
			strcat( load_package_string, "{\"audio\":" ); strcat( load_package_string, audio ? "1" : "0" );
			strcat( load_package_string, ",\"start\":" ); sprintf( str, "%d", *currpos ); strcat( load_package_string, str );
			strcat( load_package_string, ",\"crunched\":0" ); 
			strcat( load_package_string, ",\"end\":" ); sprintf( str, "%d", *currpos+length ); strcat( load_package_string, str );
			strcat( load_package_string, ",\"filename\":\"" ); strcat( load_package_string, shortfilepath ); strcat( load_package_string, "\"}," );

			*currpos += length;
		}

		g_free(filepath);
		g_free(shortfilepath);
	}

	g_dir_close(dir);
	
	return TRUE;
}

gchar* utils_create_relative_path( const gchar* base_path, const gchar* path )
{
	if ( !base_path || !path ) return g_strdup("");
	if ( !*base_path || !*path) return g_strdup("");

	// paths must start the same to make a realtive path
	if ( *base_path != *path ) return g_strdup(path);

	// advance to the point where the paths differ
	int counter = 0;
	int lastSlash = 0;
	while( *(base_path+counter) && *(path+counter) && *(base_path+counter) == *(path+counter) )
	{
		if ( *(base_path+counter) == '/' || *(base_path+counter) == '\\' ) lastSlash = counter+1;
		counter++;
	}

	// if base path stops here then the relative path is just the rest of the main path
	if ( !*(base_path+counter) ) 
	{
		if ( *(path+counter) == '/' || *(path+counter) == '\\' ) return g_strdup( path+counter+1 );
		else if ( *(path+counter-1) == '/' || *(path+counter-1) == '\\' ) return g_strdup( path+counter );
		else return g_strconcat( "../", path+lastSlash, NULL );
	}

	gchar temp[1024];
	temp[0] = 0;

	while( *(base_path+counter) )
	{
		if ( (*(base_path+counter) == '/' || *(base_path+counter) == '\\') && *(base_path+counter+1) )
		{
			strcat( temp, "../" );
		}
		counter++;
	}

	strcat( temp, "../" );

	strcat( temp, path+lastSlash );
	return g_strdup( temp );
}
