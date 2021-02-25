% ngram(1) | N-Gram Generator

# NAME

NGRAM - An interface to the ngram library

# SYNOPSES

ngram -h

# DESCRIPTION

	Project: ngram - Generate ngrams            
	Author:  Richard James Howe                
	License: Public Domain                     
	Email:   howe.r.j.89@gmail.com             
	Website: <https://github.com/howerj/ngram> 

Generate an [n-gram][] list from an input. The program acts a [Unix][]
[filter][]. The program is built around a library that can read from an
arbitrary source (and print to one), allowing it to be embedded in other
applications.

Input is taken from standard input and written to standard output,
the program works on textual or binary data. Output is in the form
of escaped strings. Non-zero is returned on error.

# OPTIONS

	-h        print this help message and exit successfully
	-i        ignore case by converting upper to lower case
	-b        run built in self tests
	-t        tree print instead of on n-gram per line
	-v        increase verbosity
	-d string use a list of delimiters, binary values are in hex, '\xHH'
	-s char   set the output separator for printing results
	-w        use white space as a set of delimiters
	-W        use any character that is not alphanumeric as a delimiter
	-l #      minimum n-gram count to print, maximum if -H not used
	-H #      maximum n-gram count to generate
	-n #      instead of using a delimiter, read # in bytes at a time


# RETURN CODE

This program returns zero on success and non-zero on failure.

# LICENSE

Placed in the public domain, effectively, with the Unlicense, do what thou wilt.

# BUILDING

Build requirements are a [C][] compiler and [Make][]. To build:

	make

To run the tests:

	make test

# RUNNING

For a full list of command line options, run "./ngram -h" (On Unixen) or
"ngram.exe -h" on Windows.

To save [n-grams][] to a file:

	./ngram < file.ext > file.ngrams

By default [n-grams][] of length 1 are constructed with their counts prefixed,
and elements are split into single characters. This produces a frequency count
of all the symbols that occur in the input text. Ranges of [n-grams][] can be
printed by specifying them with the "-l" and "-H" command. Instead of splitting
the input character by character it is possible to split it on a set of
delimiters (one of the built in sets, like white-space specified by "-w", or
with a custom set, specified with the "-d" options, which excepts a set of
characters to delimit the [n-grams][] on).

To print out all [n-grams][] of length 2:

	./ngram -l 2 < file.ext > file.ngrams

Or

	./ngram -l 2 -H 2 < file.ext > file.ngrams

To print out all [n-grams][] of length 2-4:

	./ngram -l 2 -H 4 < file.ext > file.ngrams

To split on white-space instead of character by character:

	./ngram -l 3 -w < file.ext > file.ngrams

Alternatively:

	./ngrams -l 3 -d " \x09\n\r\v" < file.ext > file.ngrams

Output can be given the form of a tree as well with the "-t" option.

# PREPROCESSING TEXT

This tool does not handle ignoring a set of characters when constructing 
its [n-grams][]. Instead of adding these features to this tool the
program [tr][] can be used to delete characters (ignoring them).

[n-gram]: https://en.wikipedia.org/wiki/N-gram
[n-grams]: https://en.wikipedia.org/wiki/N-gram
[C]: https://en.wikipedia.org/wiki/C_%28programming_language%29
[Make]: https://en.wikipedia.org/wiki/Make_(software)
[filter]: https://en.wikipedia.org/wiki/Filter_(software)
[Unix]: https://en.wikipedia.org/wiki/Unix
[tr]: https://en.wikipedia.org/wiki/Tr_(Unix)
