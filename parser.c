#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "error.h"
#include "syswrap.h"
#include "lexer.h"
#include "parser.h"

void parser_setlinevar(Parser *this, double value) {
    varmap_setval(&this->varmap, "_", value);
}

Parser *parser_new(const char *program) {
    Parser *new = malloc_or_die(sizeof *new);
    new->lexer = lexer_new(program);
    new->curr = NULL;
    new->varmap = NULL;
    return new;
}

void parser_set_buff(Parser *this, const char *program) {
    lexer_free(this->lexer);
    this->lexer = lexer_new(program);
}

void parser_free(Parser *this) {
    lexer_free(this->lexer);
    token_free(this->curr);
    varmap_free(this->varmap);
    free(this);
}

Token *parser_curr(Parser *this) {
    if(!this->curr)
        this->curr = lexer_next(this->lexer);
    return this->curr;
}

void parser_next(Parser *this) {
    if(this->curr)
        token_free(this->curr);
    this->curr = lexer_next(this->lexer);
}

void parser_accept(Parser *this, TokenType token_type) {
    Token *curr = parser_curr(this);
    token_ensure_type(curr, token_type);
    parser_next(this);
}

double parser_expr(Parser *this);

double parser_handle_assignment(Parser *this, const char *key) {
    parser_accept(this, TOKT_EQ);
    double result = parser_expr(this);
    varmap_setval(&this->varmap, key, result);
    return result;
}

double parser_handle_variable(Parser *this) {
    Token *var = parser_curr(this);
    char *key = strdup_or_die(token_varname(var));
    parser_accept(this, TOKT_IDENTIFIER);

    Token *eq = parser_curr(this);

    double result = token_type(eq) == TOKT_EQ
        ? parser_handle_assignment(this, key)
        : varmap_getval(this->varmap, key);

    free(key);
    return result;
}

double parser_paren_expr(Parser *this) {
    parser_accept(this, TOKT_LPAREN);
    double result = parser_expr(this);
    parser_accept(this, TOKT_RPAREN);
    return result;
}

double parser_number(Parser *this) {
    Token *curr = parser_curr(this);
    double result = token_number(curr);
    parser_accept(this, TOKT_NUMBER);
    return result;
}

double parser_atom(Parser *this) {
    Token *curr = parser_curr(this);

    switch(token_type(curr)) {
    case TOKT_NUMBER:
        return parser_number(this);
    case TOKT_SUB:
        parser_accept(this, TOKT_SUB);
        return -parser_atom(this);
    case TOKT_LPAREN:
        return parser_paren_expr(this);
    case TOKT_IDENTIFIER:
        return parser_handle_variable(this);
    default:
        break;
    }

    fprintf(stderr, "TokenType Error: Cannot parse atom. Position: %d. Found: '%c' (%d).\n",
        this->lexer->pos, token_type(curr), token_type(curr));
    exit(-1);
}

double parser_term(Parser *this) {
    double result = parser_atom(this);

    for(;;) {
        Token *curr = parser_curr(this);

        switch(token_type(curr)) {
        case TOKT_MULT:
            parser_accept(this, TOKT_MULT);
            result *= parser_atom(this);
            break;
        case TOKT_DIV:
            parser_accept(this, TOKT_DIV);
            result /= parser_atom(this);
            break;
        case TOKT_FLOORDIV:
            parser_accept(this, TOKT_FLOORDIV);
            result /= parser_atom(this);
            result = floor(result);
            break;
        case TOKT_MOD:
            parser_accept(this, TOKT_MOD);
            result = fmod(result, parser_atom(this));
            break;
        default:
            goto done;
        }
    }

done:
    return result;
}

double parser_expr(Parser *this) {
    double result = parser_term(this);

    for(;;) {
        Token *curr = parser_curr(this);

        switch(token_type(curr)) {
        case TOKT_ADD:
            parser_accept(this, TOKT_ADD);
            result += parser_term(this);
            break;
        case TOKT_SUB:
            parser_accept(this, TOKT_SUB);
            result -= parser_term(this);
            break;
        default:
            goto done;
        }
    }

done:
    return result;
}

double parser_line(Parser *this) {
    double result = parser_expr(this);
    parser_accept(this, TOKT_NEWLINE);
    return result;
}
