#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "syswrap.h"
#include "lexer.h"

static int _is_whitespace(char c) {
    static const char VALID[] = " \t";
    return !!memchr(VALID, c, strlen(VALID));
}

static int _is_alpha(char c) {
    static const char VALID[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_";
    return !!memchr(VALID, c, strlen(VALID));
}

static int _is_eof(char c) {
    return c == '\0';
}

static int _read_double(const char *data, double *result) {
    const char *p = data;
    *result = strtod(data, (void *)&p);
    return p - data;
}

static int _read_identifier(const char *data, char *buff, int bufflen) {
    char *valp = buff;
    while(_is_alpha(*data)) {
        *(valp++) = *data++;
        if(valp - buff >= bufflen - 1)
            break;
    }
    *valp = '\0';
    return valp - buff;
}

struct symmap_entry {
    const char *match;
    TokenType type;
};

static const struct symmap_entry symmap[] = {
    {"\n",  TOKT_NEWLINE},
    {"+",   TOKT_PLUS},
    {"-",   TOKT_MINUS},
    {"**",  TOKT_DUBSTAR},
    {"*",   TOKT_STAR},
    {"//",  TOKT_DUBSLASH},
    {"/",   TOKT_SLASH},
    {"%",   TOKT_PERCENT},
    {"==",  TOKT_DUBEQ},
    {"=",   TOKT_EQ},
    {"<",   TOKT_LT},
    {">",   TOKT_GT},
    {"!=",  TOKT_NE},
    {"(",   TOKT_LPAREN},
    {")",   TOKT_RPAREN},
    {0},
};

static int _read_symbol(const char *data, TokenType *type) {
    const struct symmap_entry *entry = symmap;
    while(entry->match) {
        size_t len = strlen(entry->match);
        if(strncmp(data, entry->match, len) == 0) {
            *type = entry->type;
            return len;
        }
        ++entry;
    }
    *type = TOKT_NULL;
    return 0;
}

static void token_base_init(Token *base, TokenType type, int bytes_read) {
    base->type = type;
    base->bytes_read = bytes_read;
}

static Token *token_try_new_symbol(const char *data) {
    TokenType type = TOKT_NULL;
    int bytes_read = _read_symbol(data, &type);
    if(!bytes_read)
        return NULL;
    Token *new = malloc_or_die(sizeof *new);
    token_base_init(new, type, bytes_read);
    return new;
}

static Token *token_try_new_numeric(const char *data) {
    double value;
    int bytes_read = _read_double(data, &value);
    if(!bytes_read)
        return NULL;
    NumericToken *new = malloc_or_die(sizeof *new);
    new->value = value;
    token_base_init(&new->base, TOKT_NUMBER, bytes_read);
    return (Token *)new;
}

static Token *token_try_new_identifier(const char *data) {
    if(!_is_alpha(*data))
        return NULL;
    IdentifierToken *new = malloc_or_die(sizeof *new + 32 * sizeof(*new->value));
    int bytes_read = _read_identifier(data, new->value, 32);
    token_base_init(&new->base, TOKT_IDENTIFIER, bytes_read);
    return (Token *)new;
}

void token_free(Token *this) {
    free(this);
}

TokenType token_type(Token *this) {
    return this ? this->type : TOKT_NULL;
}

Token *token_ensure_type(Token *this, TokenType expect) {
    if(token_type(this) == expect)
        return this;
    fprintf(stderr, "TokenTypeError: Expected: %d. Received: %d\n",
        expect, token_type(this));
    exit(-1);
}

double token_number(Token *this) {
    NumericToken *numtok = (NumericToken *)token_ensure_type(this, TOKT_NUMBER);
    return numtok->value;
}

const char *token_varname(Token *this) {
    IdentifierToken *idtok = (IdentifierToken *)token_ensure_type(this, TOKT_IDENTIFIER);
    return idtok->value;
}

Lexer *lexer_new(const char *data) {
    Lexer *new = malloc_or_die(sizeof *new);
    new->data = data;
    new->pos = 0;
    return new;
}

void lexer_free(Lexer *this) {
    free(this);
}

static Token *_peek(const char *data) {
    Token *result = NULL;

    result = token_try_new_symbol(data);
    if(result)
        return result;
    result = token_try_new_numeric(data);
    if(result)
        return result;
    result = token_try_new_identifier(data);
    if(result)
        return result;

    fprintf(stderr, "Lex error. Unhandled char: '%c'\n", *data);
    exit(-1);
}

Token *lexer_next(Lexer *this) {
    while(_is_whitespace(this->data[this->pos]))
        ++this->pos;
    if(_is_eof(this->data[this->pos]))
        return NULL;
    Token *peek = _peek(&this->data[this->pos]);
    if(!peek)
        return NULL;
    this->pos += peek->bytes_read;
    return peek;
}
