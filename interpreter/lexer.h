/*
 * The lexer turns Lisp-0 source into tokens.
 *
 * It allocates nothing: the source is a read-only buffer, and tokens point into it. It does not
 * exit on an error but returns an error token, because the table of errors (their messages and exit
 * statuses) is still to be written, and will map these.
 *
 * The syntax:
 *
 * - Anywhere in the source, comments and strings included, a byte is printable ASCII (32 to 126),
 *   a tab, a line feed or a carriage return. Any other control character, NUL and DEL included, is
 *   an error, and so is a byte above 127.
 * - The blanks are space, tab, line feed and carriage return. A comment runs from `;` to the next
 *   line feed, so a carriage return alone does not end it.
 * - `(`, `)` and `'` are tokens of their own.
 * - A symbol is a run of letters, digits and `- ? ! * + / < > = .`. It ends at the first character
 *   outside this alphabet, so `(`, `)`, `'`, `"`, `;` and blanks all end one: `a'b` is the symbol
 *   `a`, a quote and the symbol `b`.
 * - Any other punctuation is an error outside strings and comments. That keeps `[ ] { } : # @ ^ ~
 *   & % $ | \ _`, the backquote and the comma free for later languages to give a meaning.
 * - An integer is an optional sign (`+` or `-`), then decimal digits, or `0x` and hexadecimal
 *   digits (`0x` in lower case, the digits in either case). A word that starts like one, with an
 *   optional sign and then a digit, must be one: `1+`, `12ab` and `1.5` are errors, not symbols.
 *   One that does not fit a fixnum (61 bits, signed: -2^60 to 2^60 - 1) is an error too.
 * - A string is between double quotes. It holds any allowed byte (a raw line feed or tab included),
 *   but for `"` and `\`, which only appear as the escapes `\"` and `\\`. The third escape is `\n`.
 *
 * It assumes an ASCII execution character set.
 */

#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <stdlib.h> /* size_t */

enum token_kind {
    TOKEN_END, /* The end of the source. */
    TOKEN_OPEN, /* ( */
    TOKEN_CLOSE, /* ) */
    TOKEN_QUOTE, /* ' */
    TOKEN_SYMBOL,
    TOKEN_INTEGER,
    TOKEN_STRING,
    TOKEN_ERROR
};

enum lex_error {
    LEX_OK, /* Not an error token. */
    LEX_NON_ASCII, /* A byte above 127. */
    LEX_BAD_CHARACTER, /* A control character that is not allowed, or, outside strings and comments,
                          punctuation that is not part of Lisp-0. */
    LEX_BAD_ESCAPE, /* A backslash in a string followed by anything but `n`, `\` or `"`. */
    LEX_UNTERMINATED_STRING,
    LEX_BAD_INTEGER, /* A word that starts like an integer but is not one. */
    LEX_INTEGER_RANGE /* An integer that does not fit a fixnum. */
};

struct token {
    enum token_kind kind;
    enum lex_error error; /* TOKEN_ERROR: which. */
    const unsigned char *text; /* TOKEN_SYMBOL: its name. TOKEN_INTEGER: its characters, sign and
                                  prefix included. TOKEN_STRING: what is between the quotes,
                                  escapes included (see token_decode_string). */
    size_t length; /* The length of text. */
    size_t decoded_length; /* TOKEN_STRING: the length of the string, with its escapes decoded. */
    int64_t integer; /* TOKEN_INTEGER: its value, within the fixnum range. */
};

struct lexer {
    const unsigned char *cursor;
    const unsigned char *end;
};

/* The source may be NULL if its length is 0, as with an empty file. */
void lexer_init(struct lexer *lexer, const unsigned char *source, size_t length);

/*
 * Returns the next token. After an error token, the lexer is at the end of the source: the error
 * ends the lexing, and the next token is TOKEN_END.
 */
struct token lexer_next(struct lexer *lexer);

/* Writes the token's string to out, escapes decoded: decoded_length bytes, no terminator. */
void token_decode_string(const struct token *token, unsigned char *out);

#endif
