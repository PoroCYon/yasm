/* $IdPath$
 * Generic Options Support Header File
 *
 * Copyright (c) 2001  Stanislav Karchebny <berk@madfire.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of other contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "util.h"

#include <stdio.h>

#ifdef STDC_HEADERS
# include <string.h>
#endif

#include <libintl.h>
#define _(String)       gettext(String)
#ifdef gettext_noop
#define N_(String)      gettext_noop(String)
#else
#define N_(String)      (String)
#endif

#include "options.h"
#include "errwarn.h"


#ifdef __DEBUG__
#define DEBUG(x) fprintf ## x ;
#else
#define DEBUG(x)
#endif


/* Options Parser */
int
parse_cmdline(int argc, char **argv, opt_option *options, int nopts)
{
    int errors = 0;
    int i;
    int got_it;

    DEBUG((stderr, "parse_cmdline: entered\n"));

  fail:
    while (--argc) {
	argv++;

	if (argv[0][0] == '-') {	/* opt */
	    got_it = 0;
	    if (argv[0][1] == '-') {	/* lopt */
		for (i = 0; i < nopts; i++) {
		    if (options[i].lopt &&
			strncmp(&argv[0][2], options[i].lopt,
				strlen(options[i].lopt)) == 0) {
			char *param;

			if (options[i].takes_param) {
			    param = strchr(&argv[0][2], '=');
			    if (!param) {
				ErrorNow(_
					 ("option '--%s' needs an argument!"),
					 options[i].lopt);
				errors++;
				goto fail;
			    } else {
				*param = '\0';
				param++;
			    }
			} else
			    param = NULL;

			if (!options[i].
			    handler(&argv[0][2], param, options[i].extra))
			    got_it = 1;
			break;
		    }
		}
		if (!got_it) {
		    ErrorNow(_("unrecognized option '%s'"), argv[0]);
		    errors++;
		}
	    } else {		/* sopt */

		for (i = 0; i < nopts; i++) {
		    if (argv[0][1] == options[i].sopt) {
			char *cmd = &argv[0][1];
			char *param;

			if (options[i].takes_param) {
			    param = argv[1];
			    if (*param == '-' || param == NULL) {
				ErrorNow(_("option '-%c' needs an argument!"),
					 options[i].sopt);
				errors++;
				goto fail;
			    } else {
				argc--;
				argv++;
			    }
			} else
			    param = NULL;

			if (!options[i].handler(cmd, param, options[i].extra))
			    got_it = 1;
			break;
		    }
		}
		if (!got_it) {
		    ErrorNow(_("unrecognized option '%s'"), argv[0]);
		    errors++;
		}
	    }
	} else {    /* not an option, then it should be a file or something */

	    if (not_an_option_handler(argv[0]))
		errors++;
	}
    }

    DEBUG((stderr, "parse_cmdline: finished\n"));
    return errors;
}

void
help_msg(char *msg, char *tail, opt_option *options, int nopts)
{
    char optbuf[100], optopt[100];
    int i;

    fprintf(stdout, msg);

    for (i = 0; i < nopts; i++) {
	optbuf[0] = 0;
	optopt[0] = 0;

	if (options[i].takes_param) {
	    if (options[i].sopt) {
		sprintf(optbuf, "-%c <%s>", options[i].sopt,
			options[i].param_desc ? options[i].
			param_desc : "param");
	    }
	    if (options[i].sopt && options[i].lopt)
		strcat(optbuf, ", ");
	    if (options[i].lopt) {
		sprintf(optopt, "--%s <%s>", options[i].lopt,
			options[i].param_desc ? options[i].
			param_desc : "param");
		strcat(optbuf, optopt);
	    }
	} else {
	    if (options[i].sopt) {
		sprintf(optbuf, "-%c", options[i].sopt);
	    }
	    if (options[i].sopt && options[i].lopt)
		strcat(optbuf, ", ");
	    if (options[i].lopt) {
		sprintf(optopt, "--%s", options[i].lopt);
		strcat(optbuf, optopt);
	    }
	}

	fprintf(stdout, "    %-24s  %s\n", optbuf, options[i].description);
    }

    fprintf(stdout, tail);
}