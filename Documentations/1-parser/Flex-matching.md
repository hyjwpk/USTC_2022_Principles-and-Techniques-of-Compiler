# Flex matching -- How the Input Is Matched

This document is from `The flex Manual`. For better experience, we recommend you to read the original documents.  `The flex Manual` can be read with `info flex` in terminal.

```shell
$ info flex
# choose Matching node
```

However, the following is enough for you to finish labs.

Note: if there is any discrepancy, please refer to `The flex Manual`.

**************************



When the generated scanner is run, it analyzes its input looking for strings which match any of its patterns.  If it finds more than one match, it takes the one matching the most text (for trailing context rules, this includes the length of the trailing part, even though it will then be returned to the input).  If it finds two or more matches of the same length, the rule listed first in the `flex` input file is chosen.



Once the match is determined, the text corresponding to the match (called the "token") is made available in the global character pointer `yytext`, and its length in the global integer `yyleng`.  The "action" corresponding to the matched pattern is then executed, and then the remaining input is scanned for another match.



If no match is found, then the "default rule" is executed: the next character in the input is considered matched and copied to the standard output.  Thus, the simplest valid `flex` input is:

```c
     %%
```

which generates a scanner that simply copies its input (one character at a time) to its output.



Note that `yytext` can be defined in two different ways: either as a character _pointer_ or as a character _array_.  You can control which definition `flex` uses by including one of the special directives `%pointer` or `%array` in the first (definitions) section of your flex input.  The default is `%pointer`, unless you use the `-l` lex compatibility option, in which case `yytext` will be an array.  The advantage of using `%pointer` is substantially faster scanning and no buffer overflow when matching very large tokens (unless you run out of dynamic memory).  The disadvantage is that you are restricted in how your actions can modify `yytext`, and calls to the `unput()` function destroys the present contents of `yytext`, which can be a considerable porting headache when moving between different `lex` versions.



The advantage of `%array` is that you can then modify `yytext` to your heart‘s content, and calls to `unput()` do not destroy `yytext`. Furthermore, existing `lex` programs sometimes access `yytext` externally using declarations of the form:

```c
     extern char yytext[];
```

This definition is erroneous when used with `%pointer`, but correct for `%array`.



The `%array` declaration defines `yytext` to be an array of `YYLMAX` characters, which defaults to a fairly large value.  You can change the size by simply #define'ing `YYLMAX` to a different value in the first section of your `flex` input.  As mentioned above, with `%pointer` yytext grows dynamically to accommodate large tokens.  While this means your `%pointer` scanner can accommodate very large tokens (such as matching entire blocks of comments), bear in mind that each time the scanner must resize `yytext` it also must rescan the entire token from the beginning, so matching such tokens can prove slow.  `yytext` presently does _not_ dynamically grow if a call to `unput()` results in too much text being pushed back; instead, a run-time error results.



Also note that you cannot use `%array` with C++ scanner classes
