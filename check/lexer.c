/*
 * Checks the lexer (interpreter/lexer.c). Each case lexes a source up to its end and writes its
 * tokens as text, which it compares with the expected text:
 *
 *   (  )  '        punctuation
 *   sym:NAME       a symbol
 *   int:VALUE      an integer, in decimal
 *   str:"CONTENT"  a string, decoded
 *   error:NAME     an error token (after it, the lexer must be at the end)
 *
 * In the names and the contents, `"`, `\` and line feed are written as the escapes `\"`, `\\` and
 * `\n`, and any other control character, DEL or byte above 127 as `\xNN`.
 *
 * Usage: check-lexer, built and run by `make test` with every C compiler in COMPILERS.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../interpreter/lexer.h"

static int failures;

struct text {
    char data[2048];
    size_t length;
};

static void put(struct text *text, const char *string)
{
    size_t length = strlen(string);

    if (text->length + length >= sizeof text->data) {
        fprintf(stderr, "check-lexer: the text of a case is too long\n");
        exit(2);
    }
    memcpy(text->data + text->length, string, length + 1);
    text->length += length;
}

static void put_character(struct text *text, unsigned char c)
{
    char buffer[8];

    if (c == '"' || c == '\\') {
        snprintf(buffer, sizeof buffer, "\\%c", c);
    } else if (c == '\n') {
        snprintf(buffer, sizeof buffer, "\\n");
    } else if (c < 32 || c >= 127) {
        snprintf(buffer, sizeof buffer, "\\x%02x", (unsigned)c);
    } else {
        snprintf(buffer, sizeof buffer, "%c", c);
    }
    put(text, buffer);
}

static const char *error_name(enum lex_error error)
{
    switch (error) {
    case LEX_OK:
        return "LEX_OK";
    case LEX_NON_ASCII:
        return "LEX_NON_ASCII";
    case LEX_BAD_CHARACTER:
        return "LEX_BAD_CHARACTER";
    case LEX_BAD_ESCAPE:
        return "LEX_BAD_ESCAPE";
    case LEX_UNTERMINATED_STRING:
        return "LEX_UNTERMINATED_STRING";
    case LEX_BAD_INTEGER:
        return "LEX_BAD_INTEGER";
    case LEX_INTEGER_RANGE:
        return "LEX_INTEGER_RANGE";
    }
    return "?";
}

static void put_characters(struct text *text, const unsigned char *characters, size_t length)
{
    size_t i;

    for (i = 0; i < length; i++) {
        put_character(text, characters[i]);
    }
}

static void put_string_token(struct text *text, const struct token *token)
{
    /* One byte more than the decoded length, all filled with a byte that is not ASCII: a length
       that is too small overwrites the last byte, and one that is too large leaves some fill in
       the string. */
    unsigned char *buffer = malloc(token->decoded_length + 1);

    if (buffer == NULL) {
        exit(2);
    }
    memset(buffer, 0xAA, token->decoded_length + 1);
    token_decode_string(token, buffer);
    if (buffer[token->decoded_length] != 0xAA) {
        put(text, "str:OVERRUN");
    } else {
        put(text, "str:\"");
        put_characters(text, buffer, token->decoded_length);
        put(text, "\"");
    }
    free(buffer);
}

static void put_token(struct text *text, const struct token *token)
{
    char buffer[64];

    switch (token->kind) {
    case TOKEN_END:
        break;
    case TOKEN_OPEN:
        put(text, "(");
        break;
    case TOKEN_CLOSE:
        put(text, ")");
        break;
    case TOKEN_QUOTE:
        put(text, "'");
        break;
    case TOKEN_SYMBOL:
        put(text, "sym:");
        put_characters(text, token->text, token->length);
        break;
    case TOKEN_INTEGER:
        snprintf(buffer, sizeof buffer, "int:%lld", (long long)token->integer);
        put(text, buffer);
        break;
    case TOKEN_STRING:
        put_string_token(text, token);
        break;
    case TOKEN_ERROR:
        put(text, "error:");
        put(text, error_name(token->error));
        break;
    }
}

static void render(const char *source, size_t length, struct text *text)
{
    struct lexer lexer;

    text->length = 0;
    text->data[0] = '\0';
    lexer_init(&lexer, (const unsigned char *)source, length);
    for (;;) {
        struct token token = lexer_next(&lexer);

        if (token.kind == TOKEN_END) {
            return;
        }
        if (text->length > 0) {
            put(text, " ");
        }
        put_token(text, &token);
    }
}

static void check(int line, const char *source, size_t length, const char *expected)
{
    struct text actual;

    render(source, length, &actual);
    if (strcmp(actual.data, expected) != 0) {
        fprintf(stderr, "check/lexer.c:%d: for [%.*s]\n  expected: %s\n  got:      %s\n", line,
                (int)length, source, expected, actual.data);
        failures++;
    }
}

#define CHECK(source, expected) check(__LINE__, (source), sizeof(source) - 1, (expected))

static void expect_kind(int line, struct lexer *lexer, enum token_kind kind)
{
    struct token token = lexer_next(lexer);

    if (token.kind != kind) {
        fprintf(stderr, "check/lexer.c:%d: expected token kind %d, got %d\n", line, (int)kind,
                (int)token.kind);
        failures++;
    }
}

static void check_blanks_and_comments(void)
{
    CHECK("", "");
    CHECK(" \t\r\n ", "");
    CHECK("; only a comment", "");
    CHECK("; a comment\n(a)", "( sym:a )");
    CHECK("a ; a comment ( ) ' \" \\\nb", "sym:a sym:b");
    CHECK(";;; several semicolons ;;;\n;\n", "");
    /* A comment runs to the next line feed: not to a carriage return, which is a blank outside. */
    CHECK("; a\tb\rc\nd", "sym:d");
    CHECK("a ;c\r\nb", "sym:a sym:b");
    CHECK("a ;c\rb", "sym:a");
    CHECK("a\rb", "sym:a sym:b");
}

static void check_punctuation(void)
{
    CHECK("()", "( )");
    CHECK("'a", "' sym:a");
    CHECK("''a", "' ' sym:a");
    CHECK("'(a b)", "' ( sym:a sym:b )");
    CHECK("((a) (b (c)))", "( ( sym:a ) ( sym:b ( sym:c ) ) )");
}

static void check_symbols(void)
{
    CHECK("a", "sym:a");
    CHECK("foo-bar? set! *x* a/b <= x.y A1",
          "sym:foo-bar? sym:set! sym:*x* sym:a/b sym:<= sym:x.y sym:A1");
    CHECK("+ - ... = < > /", "sym:+ sym:- sym:... sym:= sym:< sym:> sym:/");
    CHECK("-x +x", "sym:-x sym:+x");
    /* A sign is followed by a digit to make an integer, and by anything else to make a symbol. */
    CHECK("+-1 --1 -+1 ++1", "sym:+-1 sym:--1 sym:-+1 sym:++1");
    CHECK("pair? identical? bit-and shift-left bytes->string",
          "sym:pair? sym:identical? sym:bit-and sym:shift-left sym:bytes->string");
    /* A symbol ends at the first character outside the alphabet. */
    CHECK("a(b)c", "sym:a ( sym:b ) sym:c");
    CHECK("a'b", "sym:a ' sym:b");
    CHECK("a\"b\"", "sym:a str:\"b\"");
    CHECK("a;b\nc", "sym:a sym:c");
    CHECK("a\nb\tc\rd", "sym:a sym:b sym:c sym:d");
}

static void check_integers(void)
{
    CHECK("0 42 007 -1 +5 -0", "int:0 int:42 int:7 int:-1 int:5 int:0");
    CHECK("0x1F 0x1f 0xaB -0x10 +0x10 0x0", "int:31 int:31 int:171 int:-16 int:16 int:0");
    CHECK("(+ 1 2)", "( sym:+ int:1 int:2 )");
    /* An integer ends at each delimiter, as a symbol does. */
    CHECK("12(", "int:12 (");
    CHECK("12'", "int:12 '");
    CHECK("12\"s\"", "int:12 str:\"s\"");
    CHECK("12;c\n0x1F;c", "int:12 int:31");
    CHECK("12\t13\r\n14", "int:12 int:13 int:14");
    CHECK("(- 1)", "( sym:- int:1 )");

    /* The fixnum range is -2^60 to 2^60 - 1. */
    CHECK("1152921504606846975", "int:1152921504606846975");
    CHECK("-1152921504606846976", "int:-1152921504606846976");
    CHECK("0xFFFFFFFFFFFFFFF", "int:1152921504606846975");
    CHECK("-0x1000000000000000", "int:-1152921504606846976");
    CHECK("1152921504606846976", "error:LEX_INTEGER_RANGE");
    CHECK("-1152921504606846977", "error:LEX_INTEGER_RANGE");
    CHECK("0x1000000000000000", "error:LEX_INTEGER_RANGE");
    CHECK("-0x1000000000000001", "error:LEX_INTEGER_RANGE");
    /* Out of range by far, including values that would wrap around in 64 bits. */
    CHECK("18446744073709551616", "error:LEX_INTEGER_RANGE");
    CHECK("0x10000000000000000", "error:LEX_INTEGER_RANGE");
    CHECK("99999999999999999999999999999999", "error:LEX_INTEGER_RANGE");
    CHECK("000000000000000000000000000000001", "int:1");

    /* A word that starts like an integer is one or an error, never a symbol. */
    CHECK("1+", "error:LEX_BAD_INTEGER");
    CHECK("12abc", "error:LEX_BAD_INTEGER");
    CHECK("1.5", "error:LEX_BAD_INTEGER");
    CHECK("-5x", "error:LEX_BAD_INTEGER");
    CHECK("0x", "error:LEX_BAD_INTEGER");
    CHECK("+0x", "error:LEX_BAD_INTEGER");
    CHECK("0xg", "error:LEX_BAD_INTEGER");
    CHECK("0X1F", "error:LEX_BAD_INTEGER");
    CHECK("0x1-", "error:LEX_BAD_INTEGER");
    CHECK("99999999999999999999x", "error:LEX_BAD_INTEGER");
    CHECK("a 1+ b", "sym:a error:LEX_BAD_INTEGER");
}

static void check_strings(void)
{
    CHECK("\"\"", "str:\"\"");
    CHECK("\"abc\"", "str:\"abc\"");
    CHECK("\"a b ( ) ' ; c\"", "str:\"a b ( ) ' ; c\"");
    CHECK("\"a\" \"b\"", "str:\"a\" str:\"b\"");
    CHECK("(\"a\"\"b\")", "( str:\"a\" str:\"b\" )");
    /* Escapes. */
    CHECK("\"a\\nb\"", "str:\"a\\nb\"");
    CHECK("\"a\\\\b\"", "str:\"a\\\\b\"");
    CHECK("\"a\\\"b\"", "str:\"a\\\"b\"");
    CHECK("\"\\\\\\\"\\n\"", "str:\"\\\\\\\"\\n\"");
    CHECK("\"\\\\\"", "str:\"\\\\\"");
    /* An escaped backslash is not the start of another escape: `\\n` is a backslash and an n. */
    CHECK("\"\\\\n\"", "str:\"\\\\n\"");
    CHECK("\"\\\\\\n\"", "str:\"\\\\\\n\"");
    /* Any other allowed byte stands for itself: a raw line feed, tab or carriage return too. */
    CHECK("\"a\nb\"", "str:\"a\\nb\"");
    CHECK("\"a\tb\"", "str:\"a\\x09b\"");
    CHECK("\"a\rb\"", "str:\"a\\x0db\"");

    CHECK("\"abc", "error:LEX_UNTERMINATED_STRING");
    CHECK("\"", "error:LEX_UNTERMINATED_STRING");
    CHECK("\"abc\\", "error:LEX_UNTERMINATED_STRING");
    CHECK("\"abc\\\"", "error:LEX_UNTERMINATED_STRING");
    CHECK("\"\\t\"", "error:LEX_BAD_ESCAPE");
    CHECK("\"\\x\"", "error:LEX_BAD_ESCAPE");
    CHECK("\"\\0\"", "error:LEX_BAD_ESCAPE");
    CHECK("\"\\ \"", "error:LEX_BAD_ESCAPE");
}

static void check_rejected_characters(void)
{
    /* The punctuation that the later languages may use, and so Lisp-0 reserves. */
    const char *reserved = "[]{}:#@^~&%$|\\_`,";
    const char *c;

    for (c = reserved; *c != '\0'; c++) {
        check(__LINE__, c, 1, "error:LEX_BAD_CHARACTER");
    }
    CHECK("a#b", "sym:a error:LEX_BAD_CHARACTER");
    CHECK("(a [b])", "( sym:a error:LEX_BAD_CHARACTER");
    /* Control characters other than tab, line feed and carriage return, and DEL. */
    CHECK("\x01", "error:LEX_BAD_CHARACTER");
    CHECK("\f", "error:LEX_BAD_CHARACTER");
    CHECK("\v", "error:LEX_BAD_CHARACTER");
    CHECK("\x7f", "error:LEX_BAD_CHARACTER");
    CHECK("\0", "error:LEX_BAD_CHARACTER");
    CHECK("a\0", "sym:a error:LEX_BAD_CHARACTER");
}

static int is_allowed_byte(int byte)
{
    return (byte >= 32 && byte <= 126) || byte == '\t' || byte == '\n' || byte == '\r';
}

/* The rule on bytes holds everywhere: in the code, in a comment and in a string. */
static void check_control_characters(void)
{
    int byte;

    for (byte = 0; byte < 128; byte++) {
        char code[1];
        char comment[2];
        char string[3];
        char escape[4];

        if (is_allowed_byte(byte)) {
            continue;
        }
        code[0] = (char)byte;
        comment[0] = ';';
        comment[1] = (char)byte;
        string[0] = '"';
        string[1] = (char)byte;
        string[2] = '"';
        escape[0] = '"';
        escape[1] = '\\';
        escape[2] = (char)byte;
        escape[3] = '"';
        check(__LINE__, code, sizeof code, "error:LEX_BAD_CHARACTER");
        check(__LINE__, comment, sizeof comment, "error:LEX_BAD_CHARACTER");
        check(__LINE__, string, sizeof string, "error:LEX_BAD_CHARACTER");
        /* After a backslash, the byte is reported as what it is, not as a bad escape. */
        check(__LINE__, escape, sizeof escape, "error:LEX_BAD_CHARACTER");
    }
    CHECK("; \001", "error:LEX_BAD_CHARACTER");
    CHECK("a ; \033[0m\nb", "sym:a error:LEX_BAD_CHARACTER");
    CHECK("; \177", "error:LEX_BAD_CHARACTER");
    CHECK("; a\0b", "error:LEX_BAD_CHARACTER");
    CHECK("\"a\0b\"", "error:LEX_BAD_CHARACTER");
    CHECK("\"\033\"", "error:LEX_BAD_CHARACTER");
}

/* Every allowed byte is accepted in a comment, and in a string, where it stands for itself. */
static void check_allowed_bytes(void)
{
    int byte;

    for (byte = 0; byte < 128; byte++) {
        char comment[2];
        char string[3];
        struct text expected;

        if (!is_allowed_byte(byte)) {
            continue;
        }
        comment[0] = ';';
        comment[1] = (char)byte;
        check(__LINE__, comment, sizeof comment, "");
        if (byte == '"' || byte == '\\') {
            continue;
        }
        string[0] = '"';
        string[1] = (char)byte;
        string[2] = '"';
        expected.length = 0;
        expected.data[0] = '\0';
        put(&expected, "str:\"");
        put_character(&expected, (unsigned char)byte);
        put(&expected, "\"");
        check(__LINE__, string, sizeof string, expected.data);
    }
}

static void check_non_ascii(void)
{
    CHECK("\xC3\xA9", "error:LEX_NON_ASCII");
    CHECK("a\x80", "sym:a error:LEX_NON_ASCII");
    CHECK("\xFF", "error:LEX_NON_ASCII");
    CHECK("; \xC3\xA9", "error:LEX_NON_ASCII");
    CHECK("a ; \xC3\xA9\n b", "sym:a error:LEX_NON_ASCII");
    CHECK("\"\xC3\xA9\"", "error:LEX_NON_ASCII");
    CHECK("\"a\\\xC3\"", "error:LEX_NON_ASCII");
    CHECK("\"abc\xC3", "error:LEX_NON_ASCII");
}

static void check_end(void)
{
    struct lexer lexer;
    const unsigned char source[] = "# a";

    /* The end of the source repeats. */
    lexer_init(&lexer, source, 0);
    expect_kind(__LINE__, &lexer, TOKEN_END);
    expect_kind(__LINE__, &lexer, TOKEN_END);

    /* An empty source may be a null pointer, as when allocating an empty file gives one. */
    lexer_init(&lexer, NULL, 0);
    expect_kind(__LINE__, &lexer, TOKEN_END);

    /* An error ends the lexing. */
    lexer_init(&lexer, source, sizeof source - 1);
    expect_kind(__LINE__, &lexer, TOKEN_ERROR);
    expect_kind(__LINE__, &lexer, TOKEN_END);
    expect_kind(__LINE__, &lexer, TOKEN_END);
}

int main(void)
{
    check_blanks_and_comments();
    check_punctuation();
    check_symbols();
    check_integers();
    check_strings();
    check_rejected_characters();
    check_control_characters();
    check_allowed_bytes();
    check_non_ascii();
    check_end();
    if (failures > 0) {
        fprintf(stderr, "check-lexer: %d failed\n", failures);
        return 1;
    }
    return 0;
}
