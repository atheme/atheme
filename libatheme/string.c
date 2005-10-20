/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * String functions.
 *
 * $Id: string.c 3053 2005-10-20 18:04:13Z nenolod $
 */

#include <org.atheme.claro.base>

#ifndef HAVE_STRLCAT
/* These functions are taken from Linux. */
size_t strlcat(char *dest, const char *src, size_t count)
{
        size_t dsize = strlen(dest);
        size_t len = strlen(src);
        size_t res = dsize + len;

        dest += dsize;
        count -= dsize;
        if (len >= count)
                len = count - 1;
        memcpy(dest, src, len);
        dest[len] = 0;
        return res;
}
#endif

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dest, const char *src, size_t size)
{
        size_t ret = strlen(src);

        if (size)
        {
                size_t len = (ret >= size) ? size - 1 : ret;
                memcpy(dest, src, len);
                dest[len] = '\0';
        }
        return ret;
}
#endif

/* copy at most len-1 characters from a string to a buffer, NULL terminate */
char *strscpy(char *d, const char *s, size_t len)
{
        char *d_orig = d;

        if (!len)
                return d;
        while (--len && (*d++ = *s++));
        *d = 0;
        return d_orig;
}

/* removes unwanted chars from a line */
void strip(char *line)
{
        char *c;

        if (line)
        {
                if ((c = strchr(line, '\n')))
                        *c = '\0';
                if ((c = strchr(line, '\r')))
                        *c = '\0';
                if ((c = strchr(line, '\1')))
                        *c = '\0';
        }
}


