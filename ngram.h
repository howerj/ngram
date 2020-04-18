#ifndef NGRAM_H
#define NGRAM_H

#include <stddef.h>
#include <stdint.h>

struct ngram {
	struct ngram *parent; /* parent of this n-gram */
	struct ngram **ns;    /* sorted list of n-gram children */
	size_t ml,            /* length of this item, if zero, this is the root node and not an n-gram */
	       nl,            /* number of children */
	       cnt;           /* count of this n-gram occurrence */
	uint8_t m[];          /* flexible array member; this n-grams value */
}; /* tree of n-grams, each node is an element in that n-gram */

typedef struct ngram ngram_t;

typedef struct {
	int (*get)(void *in);          /* return negative on error, a byte (0-255) otherwise */
	int (*put)(int ch, void *out); /* return ch on no error */
	void *in, *out;                /* passed to 'get' and 'put' respectively */
	size_t read, wrote;            /* read only, bytes 'get' and 'put' respectively */
} ngram_io_t; /**< I/O abstraction, use to redirect to wherever you want... */

typedef struct {
	int min, max, sep;
	unsigned merge: 1, tree :1;
} ngram_print_t;

/* if delimiters == NULL, then split on n-grams of length */
ngram_t *ngram(ngram_io_t *io, int max, const uint8_t *delimiters, size_t length);
int ngram_print(const ngram_t *n, ngram_io_t *io, const ngram_print_t *p);
int ngram_free(ngram_t *n);
int ngram_tests(void); /* 0  = success or NDEBUG defined, negative on fail */
int ngram_version(unsigned long *version);

#endif
