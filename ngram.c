/* Project : ngram.c - Generate n-grams
 * Author  : Richard James Howe
 * License : Public Domain
 * Email   : howe.r.j.89@gmail.com|
 * Website : <https://github.com/howerj/ngram>
 * To-Do List
 * - [x] implement n-gram collection
 * - [x] program must work on binary data
 * - [x] allow n-gram to be split on delimiter
 * - [x] turn into a library that can read from an arbitrary stream,
 *       remove the need for <stdio.h>.
 * - [x] minimal and maximal printing, print in sorted order with
 *       count first, better printing in general, allow merged
 *       n-gram printing for character n-grams.
 * - [x] implement binary search to speed up search
 * - [ ] implement binary search insertion to speed up n-gram generation
 * - [x] allow binary values to be escaped in delimiter string
 * - [x] optional tree printing
 * - [ ] add unit tests
 * - [ ] allow regex as a delimiter (will require a binary regex engine),
 *   adapt the regex engine from <https://github.com/howerj/pickle>.
 * - [x] add options; n-gram min/max, ignore case?, specify delimiters, ...
 * - [ ] remove this To-Do list once all items have been completed */

#include "ngram.h"
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef NDEBUG
#define DEBUGGING (0)
#else
#define DEBUGGING (1)
#endif

#define SEPERATOR  ","
#define BINARY_SEARCH (1)
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

typedef struct {
	size_t l;
	uint8_t m[];
} v_t;

static int get(ngram_io_t *io) {
	assert(io);
	const int r = io->get(io->in);
	io->read += r >= 0;
	assert((r >= 0 && r <= 255) || r == -1);
	return r;
}

static int put(const int ch, ngram_io_t *io) {
	assert(io);
	const int r = io->put(ch, io->out);
	io->wrote += r >= 0;
	assert((r >= 0 && r <= 255) || r == -1);
	return r;
}

static int sput(const char *s, ngram_io_t *io) {
	assert(io);
	assert(s);
	size_t i = 0;
	for (i = 0; s[i]; i++)
		if (put(s[i], io) < 0)
			return -1;
	return i;
}

static int output(unsigned count, int docount, int merge, uint8_t *m, size_t l, ngram_io_t *io) {
	assert(m);
	assert(io);
	size_t r = 0;
	if (!merge) {
		if (sput("\"", io) < 0)
			return -1;
		r++;
	}

	for (size_t i = 0; i < l; i++) {
		char s[5 /* \xHH + '\0' */] = { m[i] };
		char *p = s;
		switch (m[i]) {
		case '\0': p = "\\0";  break;
		case '\\': p = "\\\\"; break;
		case '"':  p = "\"";   break;
		case '\a': p = "\\a";  break;
		case '\b': p = "\\b";  break;
		case   27: p = "\\e";  break;
		case '\f': p = "\\f";  break;
		case '\n': p = "\\n";  break;
		case '\r': p = "\\r";  break;
		case '\t': p = "\\t";  break;
		case '\v': p = "\\v";  break;
		default:
			if (!isprint(m[i]))
				snprintf(s, sizeof s, "\\x%X", m[i]);
		}
		r += strlen(p); /* strnlen(p, sizeof s) */
		if (sput(p, io) < 0)
			return -1;
	}
	if (docount) {
		char buf[32] = { 0 };
		int spf = snprintf(buf, sizeof buf, "%s" SEPERATOR "%u", merge ? "" : "\"", (unsigned)count);
		if (spf < 0)
			return -1;
		int p = sput(buf, io);
		r += p;
	} else if (!merge) {
		if (sput("\"" SEPERATOR, io) < 0)
			return -1;
		r += 2;
	}
	return r;
}

static ngram_t *mk(v_t *v) {
	assert(v);
	ngram_t *n = calloc(1, v->l + sizeof *n);
	if (!n)
		return NULL;
	memcpy(n->m, v->m, v->l);
	n->ml = v->l;
	return n;
}

static int unmk(ngram_t *n) {
	if (!n)
		return 0;
	const size_t l = n->nl;
	for (size_t i = 0; i < l; i++) {
		unmk(n->ns[i]);
		n->ns[i] = NULL;
	}
	free(n->ns);
	n->ns = NULL;
	free(n);
	return 0;
}

static inline int compare(void *m, void *n, size_t cnt, int nocase) {
	assert(m);
	assert(n);
	char *mc = m, *nc = n;
	if (nocase) {
		for (size_t i = 0; i < cnt; i++) {
			int diff = tolower(mc[i]) - tolower(nc[i]);
			if (diff)
				return diff;
		}
		return 0;
	}
	return memcmp(m, n, cnt);
}

static int grow(ngram_t *tree, ngram_t *n) {
	assert(tree);
	ngram_t **old = tree->ns;
	tree->ns = realloc(tree->ns, (tree->nl + 1) * sizeof *tree->ns);
	if (!(tree->ns)) {
		for (size_t i = 0; i < tree->nl; i++)
			unmk(old[i]);
		free(tree->ns);
		tree->ns = NULL;
		tree->nl = 0;
		return -1;
	}
	if (BINARY_SEARCH) { /* should do binary search to find insert position...*/
		tree->ns[tree->nl] = NULL;
		for (size_t i = 0; i < tree->nl + 1; i++) {
			ngram_t *chld = tree->ns[i];
			if (!chld) {
				tree->ns[i] = n;
				break;
			}
			const int m = compare(chld->m, n->m, MIN(chld->ml, n->ml), 0);
			//assert(m || chld->ml != n->ml); /* should not be inserting already existing nodes */
			if (m > 0) {
				memmove(&tree->ns[i + 1], &tree->ns[i], (tree->nl - i) * sizeof *tree->ns);
				tree->ns[i] = n;
				break;
			}
		}
		tree->nl++;
	} else {
		tree->ns[tree->nl++] = n;
	}
	n->parent = tree;
	return 0;
}

static int is(ngram_t *n, v_t *v) {
	assert(n);
	assert(v);
	return n->ml == v->l && !compare(n->m, v->m, v->l, 0);
}

static ngram_t *find(ngram_t *n, v_t *v) {
	assert(n);
	assert(v);
	if (BINARY_SEARCH) {
		if (!(n->ns))
			return NULL;
		long l = 0, r = n->nl - 1;
		while (r >= l) {
			long m = l + (r - l) / 2;
			ngram_t *chld = n->ns[m];
			/* if v->l != chld->ml optimize by checking last character */
			const int k = compare(chld->m, v->m, MIN(chld->ml, v->l), 0);
			if (!k && chld->ml == v->l)
				return chld;
			if (k > 0)
				r = m - 1;
			else
				l = m + 1;
		}
		return NULL;
	} else {
		const size_t l = n->nl;
		for (size_t i = 0; i < l; i++)
			if (is(n->ns[i], v))
				return n->ns[i];
	}
	return NULL;
}

static int add(ngram_t *n, v_t **vs, int vl) {
	assert(vs);
	if (!n || vl <= 0)
		return 0;
	ngram_t *f = find(n, vs[0]);
	if (!f) {
		f = mk(vs[0]);
		if (!f)
			return -1;
		if (grow(n, f) < 0)
			return -1;
	}
	f->cnt++;
	return add(f, vs + 1, vl - 1);
}

static int repeat(ngram_io_t *io, int ch, int cnt) {
	assert(io);
	assert(cnt >= 0);
	for (int i = 0; i < cnt; i++)
		if (put(ch, io) < 0)
			return -1;
	return cnt;
}

static int print_tree(ngram_t *n, ngram_io_t *io, int min, int max, int depth) {
	assert(io);
	if (!n)
		return 0;
	int r = 0;
	const int root = n->ml == 0;
	if (!root) {
		const int k = repeat(io, ' ', depth);
		if (k < 0)
			return -1;
		r += k;
		const int j = output(n->cnt, depth >= (min - 1), 0, n->m, n->ml, io);
		if (j < 0)
			return -1;
		r += j;
		if (put('\n', io) < 0)
			return -1;
	}
	r += 1;
	const size_t l = n->nl;
	for (size_t i = 0; i < l; i++) {
		const int k = print_tree(n->ns[i], io, min, max, depth + !root);
		if (k < 0)
			return -1;
		r += k;
	}
	return r;
}

static int print_up(ngram_t *n, ngram_io_t *io, int merge) {
	assert(io);
	if (!n)
		return 0;
	int r = 0;
	const int j = print_up(n->parent, io, merge);
	if (j < 0)
		return -1;
	r++;
	if (n->ml) {
		const int p = output(0, 0, merge, n->m, n->ml, io);
		if (p < 0)
			return -1;
		r += p;
	}
	return 0;
}

static int print_line(ngram_t *n, ngram_io_t *io, int merge, int min, int max, int depth)  {
	assert(io);
	if (!n)
		return 0;
	int r = 0;
	for (size_t i = 0; i < n->nl; i++) {
		const int j = print_line(n->ns[i], io, merge, min, max, depth + 1);
		if (j < 0)
			return -1;
		r += j;
	}
	if (depth >= min && n->cnt) {
		char buf[32] = { 0 };
		if (snprintf(buf, sizeof buf, "%u,", (unsigned)n->cnt) < 0)
			return -1;
		const int q = sput(buf, io);
		if (q < 0)
			return -1;
		r += q;
		if (merge) {
			const int k = put('"', io);
			if (k < 0)
				return -1;
			r++;
		}
		const int j = print_up(n, io, merge);
		if (j < 0)
			return -1;
		if (merge) {
			const int k = put('"', io);
			if (k < 0)
				return -1;
			r++;
		}
		if (put('\n', io) < 0)
			return -1;
		r += j + 1;
	}
	return r;
}

static int token(ngram_io_t *io, v_t **out, const int lmode, const uint8_t *delim, const size_t dlen) {
	assert(io);
	assert(out);
	*out = NULL;
	size_t i = 0;
	v_t *n = NULL, *o = NULL;
	if (lmode) {
		if (!(n = malloc(sizeof (*n) + dlen)))
			return -1;
		for (i = 0; i < dlen; i++) {
			const int ch = get(io);
			if (ch == -1)
				break;
			n->m[i] = ch;
		}
		if (i == 0) {
			free(n);
			return 0;
		}
	} else {
		int ch = 0;
		assert(delim);
again:
		for (ch = 0; (ch = get(io)) != -1; i++) {
			if (memchr(delim, ch, dlen))
				break;
			if (!(o = realloc(n, sizeof (*n) + i + 1))) {
				free(n);
				return -1;
			}
			n = o;
			n->m[i] = ch;
		}
		if (ch == -1) {
			free(n);
			return 0;
		}
		if (i == 0)
			goto again;
	}
	n->l = i;
	*out = n;
	return 0;
}

static int delist(v_t **ls, int max) {
	assert(max >= 0);
	for (int i = 0; i < max; i++) {
		free(ls[i]);
		ls[i] = NULL;
	}
	free(ls);
	return 0;
}

ngram_t *ngram(ngram_io_t *io, const int max, const uint8_t *delimiters, const size_t length) {
	assert(io);
	int use_delimiters = !!delimiters;
	ngram_t *root = calloc(1, sizeof *root);
	v_t **ls = calloc(1, max * sizeof *ls);
	if (!ls || !root) {
		free(root);
		free(ls);
		return NULL;
	}
	/* we could also add a set of characters to ignore, but we could just
	* preprocess the text instead instead of adding complexity in here */
	for (int j = 0;;j += j < max) {
		v_t *v = NULL;
		if (token(io, &v, !use_delimiters, delimiters, length) < 0)
			goto fail;
		if (!v)
			break;
		free(ls[0]);
		memmove(ls, ls + 1, (max - 1) * sizeof *ls);
		ls[max - 1] = v;
		if (j >= max)
			if (add(root, ls, max) < 0)
				goto fail;
	}
	delist(ls, max);
	return root;
fail:
	delist(ls, max);
	free(root);
	return NULL;

}

int ngram_print(ngram_t *n, ngram_io_t *io, const int merge, const int tree, const int min, const int max) {
	assert(io);
	return tree ? print_tree(n, io, min, max, 0) : print_line(n, io, merge, min, max, 0);
}

int ngram_free(ngram_t *n) {
	return unmk(n);
}

int ngram_tests(void) {
	if (!DEBUGGING)
		return 0;
	return 0;
}

