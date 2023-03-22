# lab1 实验报告

## 实验要求

本次实验需要从无到有完成一个完整的 `Cminus-f `解析器，包括基于 `flex` 的词法分析器和基于 `bison` 的语法分析器

## 实验难点

### 词法分析器

对`Cminus-f`词法中的内容写出正则匹配式，其中困难的部分在于正则匹配中转义字符的处理，多行注释的匹配。

### 语法分析器

对`Cminus-f`语法写出对应的解析规则，其中困难的部分在于理解输出语法树中`epsilon`和`Cminus-f`中`empty`的对应关系，将对应节点设置为空可以输出`epsilon`

## 实验设计

实验通过基于 `flex` 的词法分析器和基于 `bison` 的语法分析器完成一个`Cminus-f `解析器，其中通过 `flex` 用正则表达式完成词法的识别，返回一个`token`，同时构建一个语法树上的节点， `flex` 会生成一个`yylex`函数完成这一过程。`yylex`执行的词法分析结果会被`bison` 所使用，根据定义的语法规则，组织语法树上的节点，最终形成语法树。

实验要完成的部分是 `flex` 中词法分析返回`token`，构建语法树节点，以及`bison` 中补全解析规则，构建语法树。以及根据自己的理解，定义`union`中的成员，以便构建语法树。

在**lexical_analyzer.l**中我根据词法实现了关键字、专用符号、标识符、整数、浮点数、空白符、换行符的正则匹配，以及对于多行注释的匹配。多行注释我使用的正则表达式为`\/\*([^*]|\*+[^/])*\*+\/`，其中左右两侧为`\/\*`、`\*+\/`用于匹配注释的起始和结束。在中间为`([^*]|\*+[^/])*`用于匹配注释的内容，为了解决注释中出现`/*\`字符出现的问题，注释内容匹配除`*`以外的其他字符，或者若干个`*`以及不是`/`的字符连接成的串。

在**syntax_analyzer.y**根据观察`node()`和`pass_node()`可知，union中的成员应为`struct _syntax_tree_node * node`，之后根据`Cminus-f`语法写出对应的解析规则，在规约时，利用`node()`构建节点并连接它的孩子，生成语法树。

## 实验结果验证

通过了本地easy normal hard的测试以及平台测试

自行设计的测试程序如下

```c
/* prime */
void a;
int f(void) {if (1) { if (1) {return 1;} else {return 2;} } else if (2) {return 3;} else {return 4;}}
int prime(int num)
{
    int i;
    int result;
    int sqrt;
    int temp[1];
    f();
    i = sqrt = 1;
    result = 2;
    while (result > 0)
    {
        sqrt = sqrt + 2 * i - 1;/* i^2=(i-1)^2+2i-1
        */
        if (num - sqrt < 0){
            return result;
        }
        temp = num;
        while (temp[0] > 0)
        {
            temp[0] = temp[0] - i;
        } ;
        if (temp[0] == 0){
            result = result - 1;
        }
        i = i + 1;
    } ;
    return result;
}
```

测试程序中包含

- 多函数定义
- 全局变量定义
- 局部变量定义
- if语句
- while语句
- 连续赋值语句
- 函数调用
- 数组使用
- 嵌套if语句
- 单行注释/多行注释
- 运算语句

[lexer程序生成的结果](./lexer_output)

[parser程序生成的结果](./parser_output)

## 实验反馈

希望补充实验文件中include内文件的说明
