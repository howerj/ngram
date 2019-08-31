#include "ngram.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define UNUSED(X) ((void)(X))

#ifdef _WIN32 /* Used to unfuck file mode for "Win"dows. Text mode is for losers. */
#include <windows.h>
#include <io.h>
#include <fcntl.h>
static void binary(FILE *f) { _setmode(_fileno(f), _O_BINARY); }
#else
static inline void binary(FILE *f) { UNUSED(f); }
#endif

typedef struct {
	char *arg;   /* parsed argument */
	int error,   /* turn error reporting on/off */
	    index,   /* index into argument list */
	    option,  /* parsed option */
	    reset;   /* set to reset */
	char *place; /* internal use: scanner position */
	int  init;   /* internal use: initialized or not */
} ngram_getopt_t;   /* getopt clone; with a few modifications */

static int hexCharToNibble(int c) {
	c = tolower(c);
	if ('a' <= c && c <= 'f')
		return 0xa + c - 'a';
	return c - '0';
}

static int file_get(void *in) {
	assert(in);
	return fgetc((FILE*)in);
}

static int file_put(int ch, void *out) {
	assert(out);
	return fputc(ch, (FILE*)out);
}

/* converts up to two characters and returns number of characters converted */
static int hexStr2ToInt(const char *str, int *const val) {
	assert(str);
	assert(val);
	*val = 0;
	if (!isxdigit(*str))
		return 0;
	*val = hexCharToNibble(*str++);
	if (!isxdigit(*str))
		return 1;
	*val = (*val << 4) + hexCharToNibble(*str);
	return 2;
}

static int unescape(char *r, size_t length) {
	assert(r);
	size_t k = 0;
	for (size_t j = 0, ch = 0; (ch = r[j]) && k < length; j++, k++) {
		if (ch == '\\') {
			j++;
			switch (r[j]) {
			case  '0': r[k] = '\0'; break;
			case '\\': r[k] = '\\'; break;
			case  '"': r[k] = '"';  break;
			case  'a': r[k] = '\a'; break;
			case  'b': r[k] = '\b'; break;
			case  'e': r[k] = 27;   break;
			case  'f': r[k] = '\f'; break;
			case  'n': r[k] = '\n'; break;
			case  'r': r[k] = '\r'; break;
			case  't': r[k] = '\t'; break;
			case  'v': r[k] = '\v'; break;
			case  'x': {
				int val = 0;
				const int pos = hexStr2ToInt(&r[j + 1], &val);
				if (pos < 1)
					return -2;
				j += pos;
				r[k] = val;
				break;
			}
			default:
				r[k] = r[j]; break;
			}
		} else {
			r[k] = ch;
		}
	}
	return k;
}


/* Adapted from: <https://stackoverflow.com/questions/10404448>,
 * and then from <https://github.com/howerj/pickle> in 'main.c'. */
static int ngram_getopt(ngram_getopt_t *opt, const int argc, char *const argv[], const char *fmt) {
	assert(opt);
	assert(fmt);
	assert(argv);
	enum { RETURN_E = -1, BADARG_E = ':', BADCH_E = '?' };

	if (!(opt->init)) {
		opt->place = ""; /* option letter processing */
		opt->init  = 1;
		opt->index = 1;
	}

	if (opt->reset || !*opt->place) { /* update scanning pointer */
		opt->reset = 0;
		if (opt->index >= argc || *(opt->place = argv[opt->index]) != '-') {
			opt->place = "";
			return RETURN_E;
		}
		if (opt->place[1] && *++opt->place == '-') { /* found "--" */
			opt->index++;
			opt->place = "";
			return RETURN_E;
		}
	}

	const char *oli = NULL; /* option letter list index */
	if ((opt->option = *opt->place++) == ':' || !(oli = strchr(fmt, opt->option))) { /* option letter okay? */
		 /* if the user didn't specify '-' as an option, assume it means -1.  */
		if (opt->option == '-')
			return RETURN_E;
		if (!*opt->place)
			opt->index++;
		/*if (opt->error && *fmt != ':')
			(void)fprintf(stderr, "illegal option -- %c\n", opt->option);*/
		return BADCH_E;
	}

	if (*++oli != ':') { /* don't need argument */
		opt->arg = NULL;
		if (!*opt->place)
			opt->index++;
	} else {  /* need an argument */
		if (*opt->place) { /* no white space */
			opt->arg = opt->place;
		} else if (argc <= ++opt->index) { /* no arg */
			opt->place = "";
			if (*fmt == ':')
				return BADARG_E;
			/*if (opt->error)
				(void)fprintf(stderr, "option requires an argument -- %c\n", opt->option);*/
			return BADCH_E;
		} else	{ /* white space */
			opt->arg = argv[opt->index];
		}
		opt->place = "";
		opt->index++;
	}
	return opt->option; /* dump back option letter */
}

static void *duplicate(const void *d, size_t l) {
	void *r = malloc(l);
	return r ? memcpy(r, d, l) : r;
}


static int usage(const char *arg0) {
	static const char *fmt ="\
usage: %s [-h] [-d delimiters] [-w] [-l integer] [-H integer] [-n length]\n\n\
Project : ngram - generate n-grams from arbitrary data\n\
Author  : Richard James Howe\n\
License : Public Domain\n\
Email   : howe.r.j.89@gmail.com\n\
Website : <https://github.com/howerj/ngram>\n\n\
Input is taken from standard input and written to standard output,\n\
the program works on textual or binary data. Output is in the form\n\
of escaped strings. Non-zero is returned on error.\n\n\
Options:\n\n\
  -h        print this help message and exit successfully\n\
  -d string use a list of delimiters, binary values are in hex, '\\xHH'\n\
  -w        use white space as a set of delimiters\n\
  -l #      minimum n-gram count to print, maximum if -H not used\n\
  -H #      maximum n-gram count to generate\n\
  -n #      instead of using a delimiter, read # in bytes at a time\n\n";

	return fprintf(stderr, fmt, arg0);
}

int main(int argc, char **argv) {
	uint8_t *delims = NULL, whitespace[] = { ' ', '\t', '\n', '\r', '\v', '\f' };
	char *odelim = NULL;
	size_t dl = 0;
	int min = -1, max = -1, bcount = 1;
	ngram_getopt_t opt = { .init = 0 };
	for (int ch = 0; (ch = ngram_getopt(&opt, argc, argv, "hl:H:d:wn:")) != -1;) {
		switch (ch) {
		case 'h': usage(argv[0]); return 0;
		case 'l': min = atoi(opt.arg); break;
		case 'H': max = atoi(opt.arg); break;
		case 'd': delims = duplicate(opt.arg, strlen(opt.arg)); odelim = opt.arg; break;
		case 'w': delims = whitespace; dl = sizeof whitespace; break;
		case 'n': bcount = atoi(opt.arg); break;
		default:
			fprintf(stderr, "bad arg -- %c\n", ch);
			usage(argv[0]);
			return 1;
		}
	}
	binary(stdin);
	binary(stdout);

	if (min < 0)
		min = 1;
	if (max < 0 || min >= max)
		max = min;
	if (bcount <= 0) {
		fprintf(stderr, "bad bcount -- %d", bcount);
		return -1;
	}

	if (delims && delims != whitespace) {
		const int r = unescape((char*)delims, strlen(odelim));
		if (r < 0) {
			fprintf(stderr, "invalid escape list -- %s\n", odelim);
			return 1;
		}
		dl = r;
	}

	ngram_io_t io = { .get = file_get, .put = file_put, .in = stdin, .out = stdout, };
	ngram_t *root = ngram(&io, max, delims, delims ? dl : (unsigned)bcount);
	if (!root) {
		fprintf(stderr, "ngram generation failed\n");
		return 1;
	}
	if (ngram_print(root, &io, min, max) < 0) {
		fprintf(stderr, "ngram print failed\n");
		return 1;
	}
	ngram_free(root);
	return 0;
}

