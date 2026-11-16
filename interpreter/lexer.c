/*
 * The lexer (see lexer.h for the syntax).
 *
 * Strict C99, standard library only, no compiler extensions.
 */

#include "lexer.h"

/* Fixnums are 61-bit signed integers: from -2^60 to 2^60 - 1. */
#define FIXNUM_MAX ((((uint64_t)1) << 60) - 1)

static int is_blank(unsigned char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int is_digit(unsigned char c)
{
    return c >= '0' && c <= '9';
}

static int is_non_ascii(unsigned char c)
{
    return c > 127;
}

/*
 * Checks a byte of the source, wherever it is, comments and strings included: it is printable
 * ASCII, a tab, a line feed or a carriage return. Any other control character is an error, a NUL
 * and DEL included.
 */
static enum lex_error check_byte(unsigned char c)
{
    if (is_non_ascii(c)) {
        return LEX_NON_ASCII;
    }
    if ((c >= ' ' && c <= '~') || c == '\t' || c == '\n' || c == '\r') {
        return LEX_OK;
    }
    return LEX_BAD_CHARACTER;
}

/* The value of a hexadecimal digit, or -1 if it is not one. */
static int digit_value(unsigned char c)
{
    if (is_digit(c)) {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

/* The character that the escape `\c` stands for, or -1 if there is no such escape. */
static int escape_value(unsigned char c)
{
    switch (c) {
    case 'n':
        return '\n';
    case '\\':
    case '"':
        return c;
    default:
        return -1;
    }
}

/* The alphabet of symbols: letters, digits and - ? ! * + / < > = . */
static int is_symbol_char(unsigned char c)
{
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || is_digit(c)) {
        return 1;
    }
    switch (c) {
    case '-':
    case '?':
    case '!':
    case '*':
    case '+':
    case '/':
    case '<':
    case '>':
    case '=':
    case '.':
        return 1;
    default:
        return 0;
    }
}

static struct token make_token(enum token_kind kind)
{
    struct token token;

    token.kind = kind;
    token.error = LEX_OK;
    token.text = NULL;
    token.length = 0;
    token.decoded_length = 0;
    token.integer = 0;
    return token;
}

/* An error ends the lexing: the lexer moves to the end of the source. */
static struct token failure(struct lexer *lexer, enum lex_error error)
{
    struct token token = make_token(TOKEN_ERROR);

    token.error = error;
    lexer->cursor = lexer->end;
    return token;
}

/* A comment ends at a line feed. Fails only on a comment that holds a byte that is not allowed. */
static enum lex_error skip_blanks_and_comments(struct lexer *lexer)
{
    const unsigned char *p = lexer->cursor;

    while (p != lexer->end) {
        if (is_blank(*p)) {
            p++;
        } else if (*p == ';') {
            while (p != lexer->end && *p != '\n') {
                enum lex_error error = check_byte(*p);

                if (error != LEX_OK) {
                    return error;
                }
                p++;
            }
        } else {
            break;
        }
    }
    lexer->cursor = p;
    return LEX_OK;
}

/* Does the word start like an integer: an optional sign, then a digit? */
static int looks_like_integer(const unsigned char *p, const unsigned char *end)
{
    if (p != end && (*p == '+' || *p == '-')) {
        p++;
    }
    return p != end && is_digit(*p);
}

/* Parses a word that looks like an integer. Malformed wins over out of range. */
static enum lex_error parse_integer(const unsigned char *p, const unsigned char *end,
                                    int64_t *value)
{
    int negative = 0;
    int overflow = 0;
    uint64_t base = 10;
    uint64_t limit;
    uint64_t magnitude = 0;

    if (*p == '+' || *p == '-') {
        negative = *p == '-';
        p++;
    }
    if (end - p > 1 && p[0] == '0' && p[1] == 'x') {
        base = 16;
        p += 2;
    }
    if (p == end) {
        return LEX_BAD_INTEGER;
    }
    /* The magnitude of the most negative fixnum is one more than the largest positive one. */
    limit = FIXNUM_MAX + (negative ? 1 : 0);
    for (; p != end; p++) {
        int digit = digit_value(*p);

        if (digit < 0 || (uint64_t)digit >= base) {
            return LEX_BAD_INTEGER;
        }
        /* A digit that would take the magnitude past the limit is not accumulated, so nothing
           wraps. The loop still goes on to the end, to tell a malformed integer from one that is
           out of range. */
        if (magnitude > (limit - (uint64_t)digit) / base) {
            overflow = 1;
        } else {
            magnitude = magnitude * base + (uint64_t)digit;
        }
    }
    if (overflow) {
        return LEX_INTEGER_RANGE;
    }
    *value = negative ? -(int64_t)magnitude : (int64_t)magnitude;
    return LEX_OK;
}

/* A symbol or an integer: the longest run of symbol characters. */
static struct token scan_word(struct lexer *lexer)
{
    const unsigned char *start = lexer->cursor;
    const unsigned char *p = start;
    struct token token;

    while (p != lexer->end && is_symbol_char(*p)) {
        p++;
    }
    lexer->cursor = p;
    if (looks_like_integer(start, p)) {
        int64_t value = 0;
        enum lex_error error = parse_integer(start, p, &value);

        if (error != LEX_OK) {
            return failure(lexer, error);
        }
        token = make_token(TOKEN_INTEGER);
        token.integer = value;
    } else {
        token = make_token(TOKEN_SYMBOL);
    }
    token.text = start;
    token.length = (size_t)(p - start);
    return token;
}

/* A string: from the opening quote, which the cursor is at, to the closing one. */
static struct token scan_string(struct lexer *lexer)
{
    const unsigned char *start = lexer->cursor + 1;
    const unsigned char *p = start;
    size_t decoded_length = 0;
    struct token token;

    for (;;) {
        enum lex_error error;

        if (p == lexer->end) {
            return failure(lexer, LEX_UNTERMINATED_STRING);
        }
        error = check_byte(*p);
        if (error != LEX_OK) {
            return failure(lexer, error);
        }
        if (*p == '"') {
            break;
        }
        if (*p == '\\') {
            p++;
            if (p == lexer->end) {
                return failure(lexer, LEX_UNTERMINATED_STRING);
            }
            error = check_byte(*p);
            if (error == LEX_OK && escape_value(*p) < 0) {
                error = LEX_BAD_ESCAPE;
            }
            if (error != LEX_OK) {
                return failure(lexer, error);
            }
        }
        p++;
        decoded_length++;
    }
    token = make_token(TOKEN_STRING);
    token.text = start;
    token.length = (size_t)(p - start);
    token.decoded_length = decoded_length;
    lexer->cursor = p + 1;
    return token;
}

void lexer_init(struct lexer *lexer, const unsigned char *source, size_t length)
{
    lexer->cursor = source;
    /* Adding to a null pointer is undefined, even when adding nothing. */
    lexer->end = length > 0 ? source + length : source;
}

struct token lexer_next(struct lexer *lexer)
{
    enum lex_error error = skip_blanks_and_comments(lexer);
    unsigned char c;

    if (error != LEX_OK) {
        return failure(lexer, error);
    }
    if (lexer->cursor == lexer->end) {
        return make_token(TOKEN_END);
    }
    c = *lexer->cursor;
    if (is_symbol_char(c)) {
        return scan_word(lexer);
    }
    switch (c) {
    case '(':
        lexer->cursor++;
        return make_token(TOKEN_OPEN);
    case ')':
        lexer->cursor++;
        return make_token(TOKEN_CLOSE);
    case '\'':
        lexer->cursor++;
        return make_token(TOKEN_QUOTE);
    case '"':
        return scan_string(lexer);
    default:
        return failure(lexer, is_non_ascii(c) ? LEX_NON_ASCII : LEX_BAD_CHARACTER);
    }
}

void token_decode_string(const struct token *token, unsigned char *out)
{
    const unsigned char *p = token->text;
    const unsigned char *end = p + token->length;

    while (p != end) {
        unsigned char c = *p++;

        if (c == '\\') {
            c = (unsigned char)escape_value(*p++);
        }
        *out++ = c;
    }
}
