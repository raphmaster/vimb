/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2013 Daniel Carl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include <stdio.h>
#include "ctype.h"
#include "util.h"

char *util_get_config_dir(void)
{
    char *path = g_build_filename(g_get_user_config_dir(), "vimb", NULL);
    util_create_dir_if_not_exists(path);

    return path;
}

char *util_get_cache_dir(void)
{
    char *path = g_build_filename(g_get_user_cache_dir(), "vimb", NULL);
    util_create_dir_if_not_exists(path);

    return path;
}

const char *util_get_home_dir(void)
{
    const char *dir = g_getenv("HOME");

    if (!dir) {
        dir = g_get_home_dir();
    }

    return dir;
}

void util_create_dir_if_not_exists(const char *dirpath)
{
    if (!g_file_test(dirpath, G_FILE_TEST_IS_DIR)) {
        g_mkdir_with_parents(dirpath, 0755);
    }
}

void util_create_file_if_not_exists(const char *filename)
{
    if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
        FILE *f = fopen(filename, "a");
        fclose(f);
    }
}

/**
 * Retrieves the length bytes from given file.
 *
 * The memory of returned string have to be freed!
 */
char *util_get_file_contents(const char *filename, gsize *length)
{
    GError *error  = NULL;
    char *content = NULL;
    if (!(g_file_test(filename, G_FILE_TEST_IS_REGULAR)
        && g_file_get_contents(filename, &content, length, &error))
    ) {
        fprintf(stderr, "Cannot open %s: %s\n", filename, error ? error->message : "file not found");
        g_clear_error(&error);
    }
    return content;
}

/**
 * Retrieves the file content as lines.
 *
 * The result have to be freed by g_strfreev().
 */
char **util_get_lines(const char *filename)
{
    char *content = util_get_file_contents(filename, NULL);
    char **lines  = NULL;
    if (content) {
        /* split the file content into lines */
        lines = g_strsplit(content, "\n", -1);
        g_free(content);
    }
    return lines;
}

char *util_strcasestr(const char *haystack, const char *needle)
{
    unsigned char c1, c2;
    int i, j;
    int nlen = strlen(needle);
    int hlen = strlen(haystack) - nlen + 1;

    for (i = 0; i < hlen; i++) {
        for (j = 0; j < nlen; j++) {
            c1 = haystack[i + j];
            c2 = needle[j];
            if (toupper(c1) != toupper(c2)) {
                goto next;
            }
        }
        return (char*)haystack + i;
next:
        ;
    }
    return NULL;
}


/**
 * Checks if the given source array of pointer are prefixes to all those
 * entries given as array of search strings.
 */
gboolean util_array_contains_all_tags(char **src, unsigned int s, char **query, unsigned int q)
{
    unsigned int i, n;

    if (!s || !q) {
        return true;
    }

    /* iterate over all query parts */
    for (i = 0; i < q; i++) {
        gboolean found = false;
        for (n = 0; n < s; n++) {
            if (g_str_has_prefix(src[n], query[i])) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    return true;
}

/**
 * Checks if the given array of tags are all found in source string.
 */
gboolean util_string_contains_all_tags(char *src, char **query, unsigned int q)
{
    unsigned int i;
    if (!q) {
        return true;
    }

    /* iterate over all query parts */
    for (i = 0; i < q; i++) {
        if (!util_strcasestr(src, query[i])) {
            return false;
        }
    }

    return true;
}

/**
 * Replaces appearances of search in string by given replace.
 * Returne a new allocated string of search was found.
 */
char *util_str_replace(const char* search, const char* replace, const char* string)
{
    if (!string) {
        return NULL;
    }

    char **buf = g_strsplit(string, search, -1);
    char *ret  = g_strjoinv(replace, buf);
    g_strfreev(buf);

    return ret;
}

/**
 * Creates a temporary file with given content.
 *
 * Upon success, and if file is non-NULL, the actual file path used is
 * returned in file. This string should be freed with g_free() when not
 * needed any longer.
 */
gboolean util_create_tmp_file(const char *content, char **file)
{
    int fp;
    ssize_t bytes, len;

    fp = g_file_open_tmp(PROJECT "-XXXXXX", file, NULL);
    if (fp == -1) {
        fprintf(stderr, "Could not create temporary file %s", *file);
        g_free(*file);
        return false;
    }

    len = strlen(content);

    /* write content into temporary file */
    bytes = write(fp, content, len);
    if (bytes < len) {
        close(fp);
        unlink(*file);
        fprintf(stderr, "Could not write temporary file %s", *file);
        g_free(*file);

        return false;
    }
    close(fp);

    return true;
}

/**
 * Build the absolute file path of given path and possible given directory.
 * If the path is already absolute or uses ~/ for the home directory, the
 * directory is ignored.
 *
 * Returned path must be freed.
 */
char *util_buil_path(const char *path, const char *dir)
{
    char *fullPath, *p;

    /* creating directory */
    if (path[0] == '/') {
        fullPath = g_strdup(path);
    } else if (path[0] == '~') {
        if (path[1] == '/') {
            fullPath = g_strconcat(g_get_home_dir(), &path[1], NULL);
        } else {
            fullPath = g_strconcat(g_get_home_dir(), "/", &path[1], NULL);
        }
    } else if (dir) {
        fullPath = g_strconcat(dir, "/", path, NULL);
    } else {
        fullPath = g_strconcat(g_get_current_dir(), "/", path, NULL);
    }

    if ((p = strrchr(fullPath, '/'))) {
        *p = '\0';
        g_mkdir_with_parents(fullPath, 0700);
        *p = '/';
    }

    return fullPath;
}
