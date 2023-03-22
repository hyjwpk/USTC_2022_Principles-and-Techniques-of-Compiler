#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<syntax_analyzer.h>

///
extern int lines;
extern int pos_start;
extern int pos_end;

///
extern FILE *yyin;
extern char *yytext;
extern int yylex();

// Mac-only hack.
YYSTYPE yylval;

///
int main(int argc, const char **argv) {
     if (argc != 2) {
          printf("usage: lexer input_file\n");
          return 0;
     }

     const char *input_file = argv[1];
     yyin = fopen(input_file, "r");
     if (!yyin) {
          fprintf(stderr, "cannot open file: %s\n", input_file);
          return 1;
     }

     int token;
     printf("%5s\t%10s\t%s\t%s\n", "Token", "Text", "Line", "Column (Start,End)");
     while ((token = yylex())) {
          printf("%-5d\t%10s\t%d\t(%d,%d)\n",
                 token, yytext,
                 lines, pos_start, pos_end);
     }
     return 0;
}
