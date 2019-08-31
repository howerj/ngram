# ngram

	| Project | ngram: Generate ngrams            |
	| ------- | --------------------------------- |
	| Author  | Richard James Howe                |
	| License | Public Domain                     |
	| Email   | howe.r.j.89@gmail.com             |
	| Website | <https://github.com/howerj/ngram> |

Generate an [n-gram][] list from an input. The program acts a [Unix][] 
[filter][]. The program is built around a library that can read from an
arbitrary source (and print to one), allowing it to be embedded in other
applications.

## Building

Build requirements are a [C][] compiler and [Make][]. To build:

	make

To run the tests:

	make test

## Running

For a full list of command line options, run "./ngram -h" (On Unixen) or
"ngram.exe -h" on Windows.


[n-gram]: https://en.wikipedia.org/wiki/N-gram
[C]: https://en.wikipedia.org/wiki/C_%28programming_language%29
[Make]: https://en.wikipedia.org/wiki/Make_(software)
[filter]: https://en.wikipedia.org/wiki/Filter_(software)
[Unix]: https://en.wikipedia.org/wiki/Unix
