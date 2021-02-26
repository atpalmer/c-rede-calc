#include <stdio.h>
#include "parser.h"
#include "lexer.h"

void test_lexer() {
    Lexer *lexer = lexer_new("123");
    lexer_free(lexer);
}

int main(void) {
    test_lexer();
    printf("Hello, world.\n");
}
