#ifndef NGRAM_H
#define NGRAM_H

#include <stddef.h>
#include <stdint.h>

struct ngram {
	size_t ml,         /* length of this item, if zero, this is the root node and not an n-gram */
	       nl,         /* number of children */
	       cnt;        /* count of this n-gram occurrence */
	struct ngram **ns; /* sorted list of n-gram children */
	uint8_t m[];       /* flexible array member; this n-grams value */
}; /* tree of n-grams, each node is an element in that n-gram */

typedef struct ngram ngram_t;

typedef struct {
	int (*get)(void *in);          /* return negative on error, a byte (0-255) otherwise */
	int (*put)(int ch, void *out); /* return ch on no error */
	void *in, *out;                /* passed to 'get' and 'put' respectively */
	size_t read, wrote;            /* read only, bytes 'get' and 'put' respectively */
} ngram_io_t; /**< I/O abstraction, use to redirect to wherever you want... */

/* if delimiters == NULL, then split on n-grams of length */
ngram_t *ngram(ngram_io_t *io, int max, const uint8_t *delimiters, size_t length);
int ngram_print(ngram_t *n, ngram_io_t *io, int min, int max);
int ngram_free(ngram_t *n);

#endif
