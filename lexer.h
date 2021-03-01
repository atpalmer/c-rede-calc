#ifndef LEXER_H
#define LEXER_H

enum token_type {
    TOKT_END = 0,
    TOKT_PRINT = ';',
    TOKT_ADD = '+',
    TOKT_SUB = '-',
    TOKT_MULT = '*',
    TOKT_DIV = '/',
    TOKT_EQ = '=',
    TOKT_LPAREN = '(',
    TOKT_RPAREN = ')',
    TOKT_NUMBER = -1,
    TOKT_VARIABLE = -2,
};

typedef struct {
    enum token_type type;
    int bytes_read;
} Token;

typedef struct {
    Token base;
    char value[];
} VarNameToken;

typedef struct {
    Token base;
    double value;
} NumericToken;

typedef struct {
    const char *data;
    int len;
    int pos;
    Token *curr;
} Lexer;


Lexer *lexer_new(const char *data);
void lexer_free(Lexer *this);
Token *lexer_next(Lexer *this);
Token *lexer_peek(Lexer *this);

static inline int lexer_has_next(Lexer *this) {
    return this->pos < this->len;
}

static inline Token *lexer_curr(Lexer *this) {
    return this->curr;
}

void token_free(Token **this);

static inline enum token_type token_type(Token *this) {
    return this->type;
}

static inline double token_number(Token *this) {
    return ((NumericToken *)this)->value;
}

static inline const char *token_varname(Token *this) {
    return ((VarNameToken *)this)->value;
}

#endif
