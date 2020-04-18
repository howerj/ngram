/* Test driver for n-gram generation library
 * <https://github.com/howerj/ngram */
#include "ngram.h"
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define UNUSED(X) ((void)(X))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y) ((X) < (Y) ? (Y) : (X))

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
} ngram_getopt_t;    /* getopt clone; with a few modifications */

typedef struct {
	double avg_len;
	size_t min_len, max_len, ngrams;
} ngram_stats_t;

static int ignore_case = 0;

static int hexCharToNibble(int c) {
	c = tolower(c);
	if ('a' <= c && c <= 'f')
		return 0xa + c - 'a';
	return c - '0';
}

static int file_get(void *in) {
	assert(in);
	const int r = fgetc((FILE*)in);
	if (ignore_case && r != EOF)
		return tolower(r);
	return r;
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

static int usage(FILE *out, const char *arg0) {
	assert(out);
	unsigned long version = 0;
	ngram_version(&version);
	const int o = (version >> 24) & 0xFF;
	const int x = (version >> 16) & 0xFF;
	const int y = (version >>  8) & 0xFF;
	const int z = (version >>  0) & 0xFF;
	static const char *fmt ="\
usage: %s [-hibtwWvt] [-d delimiters] [-lH integer] [-n length] [-s separator]\n\n\
Project : ngram - generate n-grams from arbitrary data\n\
Author  : Richard James Howe\n\
License : The Unlicense\n\
Email   : howe.r.j.89@gmail.com\n\
Version : %d.%d.%d\n\
Options : %x\n\
Website : <https://github.com/howerj/ngram>\n\n\
Input is taken from standard input and written to standard output,\n\
the program works on textual or binary data. Output is in the form\n\
of escaped strings. Non-zero is returned on error.\n\n\
Options:\n\n\
  -h        print this help message and exit successfully\n\
  -i        ignore case by converting upper to lower case\n\
  -b        run built in self tests\n\
  -t        tree print instead of on n-gram per line\n\
  -v        increase verbosity\n\
  -d string use a list of delimiters, binary values are in hex, '\\xHH'\n\
  -s char   set the output separator for printing results\n\
  -w        use white space as a set of delimiters\n\
  -W        use any character that is not alphanumeric as a delimiter\n\
  -l #      minimum n-gram count to print, maximum if -H not used\n\
  -H #      maximum n-gram count to generate\n\
  -n #      instead of using a delimiter, read # in bytes at a time\n\n";
	return fprintf(out, fmt, arg0, x, y, z, o);
}

static int prepare_set(uint8_t set[static 256], int (*comp)(int ch), int invert) {
	size_t j = 0;
	for (size_t i = 0; i < 256; i++)
		if (invert ^ !!comp(i))
			set[j++] = i;
	return j;
}

static size_t count(ngram_t *n) {
	if (!n)
		return 0;
	size_t r = !!(n->ml);
	for (size_t i = 0; i < n->nl; i++)
		r += count(n->ns[i]);
	return r;
}

static size_t total_bytes(ngram_t *n, ngram_stats_t *s) {
	if (!n)
		return 0;
	size_t r = n->ml;
	if (n->ml) /* do not count root node */
		s->min_len = MIN(r, s->min_len);
	s->max_len = MAX(r, s->max_len);
	for (size_t i = 0; i < n->nl; i++)
		r += total_bytes(n->ns[i], s);
	return r;
}

static int stats(ngram_t *n, ngram_stats_t *s) {
	assert(s);
	s->ngrams = count(n);
	s->min_len = n->nl ? SIZE_MAX : 0;
	if (s->ngrams)
		s->avg_len = ((double)total_bytes(n, s)) / (double)(s->ngrams);
	return 0;
}

int main(int argc, char **argv) {
	uint8_t *delims = NULL;
	uint8_t set[256] = { 0 };
	char *odelim = NULL;
	size_t dl = 0;
	int bcount = 1, verbose = 0;
	ngram_getopt_t opt = { .init = 0 };
	ngram_print_t p = { .min = -1, .max = -1, .tree = 0, .merge = 0, .sep = ',', };
	for (int ch = 0; (ch = ngram_getopt(&opt, argc, argv, "hibtvl:H:d:wWn:s:")) != -1;) {
		switch (ch) {
		case 'h': usage(stdout, argv[0]); return 0;
		case 'i': ignore_case = 1; break;
		case 'b': return -!!ngram_tests();
		case 't': p.tree = 1; break;
		case 'v': verbose++; break;
		case 'l': p.min = atoi(opt.arg); break;
		case 'H': p.max = atoi(opt.arg); break;
		case 's': p.sep = opt.arg[0]; break;
		case 'd': delims = duplicate(opt.arg, strlen(opt.arg)); odelim = opt.arg; break;
		case 'w': delims = set; dl = prepare_set(set, isspace, 0); break;
		case 'W': delims = set; dl = prepare_set(set, isalnum, 1); break;
		case 'n': bcount = atoi(opt.arg); break;
		default:
			fprintf(stderr, "bad arg -- %c\n", ch);
			usage(stderr, argv[0]);
			return 1;
		}
	}
	binary(stdin);
	binary(stdout);

	if (p.min < 0)
		p.min = 1;
	if (p.max < 0 || p.min >= p.max)
		p.max = p.min;
	if (bcount <= 0) {
		fprintf(stderr, "bad bcount -- %d", bcount);
		return -1;
	}

	if (delims && delims != set) {
		const int r = unescape((char*)delims, strlen(odelim));
		if (r < 0) {
			fprintf(stderr, "invalid escape list -- %s\n", odelim);
			return 1;
		}
		dl = r;
	}

	if (verbose) {
		if (fprintf(stderr, "ngram generator: min = %d, max = %d, tree = %d\n", p.min, p.max, p.tree) < 0)
			return 1;
	}

	ngram_io_t io = { .get = file_get, .put = file_put, .in = stdin, .out = stdout, };
	clock_t begin = clock();
	ngram_t *root = ngram(&io, p.max, delims, delims ? dl : (unsigned)bcount);
	clock_t end = clock();
	const double time = (double)(end - begin) / CLOCKS_PER_SEC;
	if (!root) {
		fprintf(stderr, "ngram generation failed\n");
		return 1;
	}
	p.merge = !p.tree && delims == NULL;
	if (ngram_print(root, &io, &p) < 0) {
		fprintf(stderr, "ngram print failed\n");
		return 1;
	}
	if (verbose) {
		ngram_stats_t st = { .avg_len = 0 };
		if (stats(root, &st) < 0)
			return 2;
		if (fprintf(stderr, "time:   %.3fs\n", time) < 0)
			return 1;
		if (fprintf(stderr, "ngrams: %u\n", (unsigned)st.ngrams) < 0)
			return 1;
		if (fprintf(stderr, "avg ln: %g\n", st.avg_len) < 0)
			return 1;
		if (fprintf(stderr, "min ln: %u\n", (unsigned)st.min_len) < 0)
			return 1;
		if (fprintf(stderr, "max ln: %u\n", (unsigned)st.max_len) < 0)
			return 1;
	}
	ngram_free(root);
	return 0;
}

