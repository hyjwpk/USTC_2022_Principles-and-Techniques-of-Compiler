%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "syntax_tree.h"

// external functions from lex
extern int yylex();
extern int yyparse();
extern int yyrestart();
extern FILE * yyin;

// external variables from lexical_analyzer module
extern int lines;
extern char * yytext;
extern int pos_end;
extern int pos_start;

// Global syntax tree
syntax_tree *gt;

// Error reporting
void yyerror(const char *s);
syntax_tree_node *node(const char *node_name, int children_num, ...);
%}

%union {
     struct _syntax_tree_node * node;
     char * name;
}

%token <node> ADD
%token <node> SUB
%token <node> MUL
%token <node> DIV
%token <node> NUM
%token <node> LPARENTHESE
%token <node> RPARENTHESE
%type <node> input expression addop term mulop factor num

%start input

%%
input : expression {$$ = node( "input", 1, $1); gt->root = $$;}
    ;
expression : expression addop term  {$$ = node( "expression", 3, $1, $2, $3);}
    |   term {$$ = node( "expression", 1, $1);}
    ;

addop : ADD {$$ = node( "addop", 1, $1);}
    |  SUB {$$ = node( "addop", 1, $1);}
    ;

term : term mulop factor {$$ = node( "term", 3, $1, $2, $3);}
    |   factor {$$ = node( "term", 1, $1);}
    ;

mulop : MUL {$$ = node( "mulop", 1, $1);}
    |  DIV {$$ = node( "mulop", 1, $1);}
    ;

factor : LPARENTHESE expression RPARENTHESE {$$ = node( "factor", 3, $1, $2, $3);}
    |  num {$$ = node( "factor", 1, $1);}
    ;

num : NUM {$$ = node( "num", 1, $1);}
%%

void yyerror(const char * s) {
    fprintf(stderr, "error at line %d column %d: %s\n", lines, pos_start, s);
}

syntax_tree *parse(const char *input_path)
{
    if (input_path != NULL) {
        if (!(yyin = fopen(input_path, "r"))) {
            fprintf(stderr, "[ERR] Open input file %s failed.\n", input_path);
            exit(1);
        }
    } else {
        yyin = stdin;
    }

    lines = pos_start = pos_end = 1;
    gt = new_syntax_tree();
    yyrestart(yyin);
    yyparse();
    return gt;
}

syntax_tree_node *node(const char *name, int children_num, ...)
{
    syntax_tree_node *p = new_syntax_tree_node(name);
    syntax_tree_node *child;
    if (children_num == 0) {
        child = new_syntax_tree_node("epsilon");
        syntax_tree_add_child(p, child);
    } else {
        va_list ap;
        va_start(ap, children_num);
        for (int i = 0; i < children_num; ++i) {
            child = va_arg(ap, syntax_tree_node *);
            syntax_tree_add_child(p, child);
        }
        va_end(ap);
    }
    return p;
}
