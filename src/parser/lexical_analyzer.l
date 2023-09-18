%option noyywrap
%{
/*****************声明和选项设置  begin*****************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "syntax_tree.h"
#include "syntax_analyzer.h"

int lines = 1;
int pos_start = 1;
int pos_end = 1;

void pass_node(char *text){
     yylval.node = new_syntax_tree_node(text);
}

/*****************声明和选项设置  end*****************/

%}


%%
 /* FIXME: */
 /* two cases for you, pass_node will send flex's token to bison */
 /* Trim spaces */
[ \t] {pos_start = pos_end; pos_end += 1;}
 /* OPERATORS */
\+ 	{pos_start = pos_end; pos_end++; pass_node(yytext); return ADD;}
\- 	{pos_start = pos_end; pos_end++; pass_node(yytext); return SUB;}
\* 	{pos_start = pos_end; pos_end++; pass_node(yytext); return MUL;}
\/ 	{pos_start = pos_end; pos_end++; pass_node(yytext); return DIV;}
\> 	{pos_start = pos_end; pos_end++; pass_node(yytext); return GT;}
\>= 	{pos_start = pos_end; pos_end += 2; pass_node(yytext); return GEQ;}
\< 	{pos_start = pos_end; pos_end++; pass_node(yytext); return LT;}
\<= 	{pos_start = pos_end; pos_end += 2; pass_node(yytext); return LEQ;}
\== 	{pos_start = pos_end; pos_end += 2; pass_node(yytext); return EQ;}
\!= 	{pos_start = pos_end; pos_end += 2; pass_node(yytext); return NEQ;}
\= 	{pos_start = pos_end; pos_end++; pass_node(yytext); return ASSIGN;}
 /* SYMBOLS */
;              {pos_start = pos_end; pos_end++; pass_node(yytext); return SEMICOLON;}
,              {pos_start = pos_end; pos_end++; pass_node(yytext); return COMMA;}
\(             {pos_start = pos_end; pos_end++; pass_node(yytext); return LPAREN;}
\)             {pos_start = pos_end; pos_end++; pass_node(yytext); return RPAREN;}
\[             {pos_start = pos_end; pos_end++; pass_node(yytext); return LBRACKET;}
\]             {pos_start = pos_end; pos_end++; pass_node(yytext); return RBRACKET;}
\{             {pos_start = pos_end; pos_end++; pass_node(yytext); return LBRACE;}
\}             {pos_start = pos_end; pos_end++; pass_node(yytext); return RBRACE;}
 /* KEYWORDS */
if             {pos_start = pos_end; pos_end += 2; pass_node(yytext); return IF;}
else           {pos_start = pos_end; pos_end += 4; pass_node(yytext); return ELSE;}
int            {pos_start = pos_end; pos_end += 3; pass_node(yytext); return INT;}
return         {pos_start = pos_end; pos_end += 6; pass_node(yytext); return RETURN;}
void           {pos_start = pos_end; pos_end += 4; pass_node(yytext); return VOID;}
while          {pos_start = pos_end; pos_end += 5; pass_node(yytext); return WHILE;}
float          {pos_start = pos_end; pos_end += 5; pass_node(yytext); return FLOAT;}
 /* ID */
[a-zA-Z]+      {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return ID;}
 /* LITERAL */
[0-9]+ 	          {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return INTEGER;}
[0-9]+\.|[0-9]*\.[0-9]+ {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return FLOATPOINT;}
\/\*.*\*\/     {pos_start = pos_end; pos_end += strlen(yytext);}
 /* NEWLINE */
\n             {pos_start = pos_end = 1; lines++;}
 /* COMMENT */
"/*"([^\*]|(\*)*[^\*/])*(\*)*"*/" {
     pos_start = pos_end;
     // Considering multi-line comment
     for (int i = 0; i < strlen(yytext); i++) {
          if (yytext[i] == '\n') {
               lines++;
               pos_end = 0;
          } else {
               pos_end++;
          }
     }
}
 /* NO MATCH */
. { pos_start = pos_end; pos_end++; return ERROR; }

 /****请在此补全所有flex的模式与动作  end******/
%%
