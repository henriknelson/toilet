/*
 *  TOIlet        The Other Implementation’s letters
 *  Copyright (c) 2006 Sam Hocevar <sam@zoy.org>
 *                All Rights Reserved
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the Do What The Fuck You Want To
 *  Public License, Version 2, as published by Sam Hocevar. See
 *  http://sam.zoy.org/wtfpl/COPYING for more details.
 */

/*
 * This is the main program entry point.
 */

#include "config.h"

#if defined(HAVE_INTTYPES_H)
#   include <inttypes.h>
#endif
#if defined(HAVE_GETOPT_H)
#   include <getopt.h>
#endif
#if defined(HAVE_SYS_IOCTL_H) && defined(HAVE_TIOCGWINSZ)
#   include <sys/ioctl.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cucul.h>

#include "toilet.h"
#include "render.h"
#include "figlet.h"
#include "filter.h"

static void version(void);
#if defined(HAVE_GETOPT_H)
static void usage(void);
#endif

int main(int argc, char *argv[])
{
    context_t struct_cx;
    context_t *cx = &struct_cx;

    cucul_buffer_t *buffer;

    int i, j, ret;

    int infocode = -1;

    cx->export = "utf8";
    cx->font = "mono9";
    cx->dir = "/usr/share/figlet/";

    cx->term_width = 80;

    cx->filters = NULL;
    cx->nfilters = 0;

#if defined(HAVE_GETOPT_H)
    for(;;)
    {
#   ifdef HAVE_GETOPT_LONG
#       define MOREINFO "Try `%s --help' for more information.\n"
        int option_index = 0;
        static struct option long_options[] =
        {
            /* Long option, needs arg, flag, short option */
            { "font", 1, NULL, 'f' },
            { "directory", 1, NULL, 'd' },
            { "width", 1, NULL, 'w' },
            { "termwidth", 0, NULL, 't' },
            { "filter", 1, NULL, 'F' },
            { "gay", 0, NULL, 'g' },
            { "metal", 0, NULL, 'm' },
            { "irc", 0, NULL, 'i' },
            { "help", 0, NULL, 'h' },
            { "infocode", 1, NULL, 'I' },
            { "version", 0, NULL, 'v' },
            { NULL, 0, NULL, 0 }
        };

        int c = getopt_long(argc, argv, "f:d:w:tF:gmihI:v",
                            long_options, &option_index);
#   else
#       define MOREINFO "Try `%s -h' for more information.\n"
        int c = getopt(argc, argv, "f:d:w:tF:gmihI:v");
#   endif
        if(c == -1)
            break;

        switch(c)
        {
        case 'h': /* --help */
            usage();
            return 0;
        case 'I': /* --infocode */
            infocode = atoi(optarg);
            break;
        case 'v': /* --version */
            version();
            return 0;
        case 'f': /* --font */
            cx->font = optarg;
            break;
        case 'd': /* --directory */
            cx->dir = optarg;
            break;
        case 'F': /* --filter */
            if(filter_add(cx, optarg))
                return -1;
            break;
        case 'g': /* --gay */
            filter_add(cx, "gay");
            break;
        case 'm': /* --metal */
            filter_add(cx, "metal");
            break;
        case 'w': /* --width */
            cx->term_width = atoi(optarg);
            break;
        case 't': /* --termwidth */
        {
#if defined(HAVE_SYS_IOCTL_H) && defined(HAVE_TIOCGWINSZ)
            struct winsize ws;

            if((ioctl(1, TIOCGWINSZ, &ws) != -1 ||
                ioctl(2, TIOCGWINSZ, &ws) != -1 ||
                ioctl(0, TIOCGWINSZ, &ws) != -1) && ws.ws_col != 0)
                cx->term_width = ws.ws_col;
#endif
            break;
        }
        case 'i': /* --irc */
            cx->export = "irc";
            break;
        case '?':
            printf(MOREINFO, argv[0]);
            return 1;
        default:
            printf("%s: invalid option -- %i\n", argv[0], c);
            printf(MOREINFO, argv[0]);
            return 1;
        }
    }
#else
#   define MOREINFO "Usage: %s message...\n"
    int optind = 1;
#endif

    switch(infocode)
    {
        case -1:
            break;
        case 0:
            version();
            return 0;
        case 1:
            printf("20201\n");
            return 0;
        case 2:
            printf("%s\n", cx->dir);
            return 0;
        case 3:
            printf("%s\n", cx->font);
            return 0;
        case 4:
            printf("%u\n", cx->term_width);
            return 0;
        default:
            return 0;
    }

    if(!strcasecmp(cx->font, "mono9"))
        ret = init_big(cx);
    else if(!strcasecmp(cx->font, "term"))
        ret = init_tiny(cx);
    else
        ret = init_figlet(cx);

    if(ret)
        return -1;

    if(optind >= argc)
    {
        char buf[10];
        unsigned int len;
        uint32_t ch;

        i = 0;

        /* Read from stdin */
        while(!feof(stdin))
        {
            buf[i++] = getchar();
            buf[i] = '\0';

            ch = cucul_utf8_to_utf32(buf, &len);

            if(!len)
                continue;

            cx->feed(cx, ch);
            i = 0;
        }
    }
    else for(i = optind; i < argc; i++)
    {
        /* Read from commandline */
        unsigned int len;

        if(i != optind)
            cx->feed(cx, ' ');

        for(j = 0; argv[i][j];)
        {
            cx->feed(cx, cucul_utf8_to_utf32(argv[i] + j, &len));
            j += len;
        }
    }

    cx->end(cx);

    /* Apply optional effects to our string */
    filter_do(cx);

    /* Output char */
    buffer = cucul_export_canvas(cx->cv, cx->export);
    fwrite(cucul_get_buffer_data(buffer),
           cucul_get_buffer_size(buffer), 1, stdout);
    cucul_free_buffer(buffer);

    cucul_free_canvas(cx->cv);

    return 0;
}

#if defined(HAVE_GETOPT_H)
#   define USAGE \
    "Usage: toilet [ -ghimtvF ] [ -d fontdirectory ]\n" \
    "              [ -f fontfile ] [ -w outputwidth ]\n" \
    "              [ -I infocode ] [ message ]\n"
#else
#   define USAGE ""
#endif

static void version(void)
{
    printf(
    "TOIlet Copyright 2006 Sam Hocevar\n"
    "Internet: <sam@zoy.org> Version: %s, date: %s\n"
    "\n"
    "TOIlet, along with the various TOIlet fonts and documentation, may be\n"
    "freely copied and distributed.\n"
    "\n"
    "If you use TOIlet, please send an e-mail message to <sam@zoy.org>.\n"
    "\n"
    "The latest version of TOIlet is available from the web site,\n"
    "        http://libcaca.zoy.org/toilet.html\n"
    "\n"
    USAGE,
    VERSION, DATE);
}

#if defined(HAVE_GETOPT_H)
static void usage(void)
{
    printf(USAGE);
#   ifdef HAVE_GETOPT_LONG
    printf("  -f, --font <fontfile>    select the font\n");
    printf("  -d, --directory <dir>    specify font directory\n");
    printf("  -w, --width <width>      set output width\n");
    printf("  -t, --termwidth          adapt to terminal's width\n");
    printf("  -F, --filter             apply one or several filters to the text\n");
    printf("  -g, --gay                add a rainbow effect to the text\n");
    printf("  -m, --metal              add a metal effect to the text\n");
    printf("  -i, --irc                output IRC colour codes\n");
    printf("  -h, --help               display this help and exit\n");
    printf("  -I, --infocode           print FIGlet-compatible infocode\n");
    printf("  -v, --version            output version information and exit\n");
#   else
    printf("  -f <fontfile>    select the font\n");
    printf("  -d <dir>         specify font directory\n");
    printf("  -w <width>       set output width\n");
    printf("  -t               adapt to terminal's width\n");
    printf("  -F               apply one or several filters to the text\n");
    printf("  -g               add a rainbow effect to the text\n");
    printf("  -m               add a metal effect to the text\n");
    printf("  -i               output IRC colour codes\n");
    printf("  -h               display this help and exit\n");
    printf("  -I               print FIGlet-compatible infocode\n");
    printf("  -v               output version information and exit\n");
#   endif
}
#endif

