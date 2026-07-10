/*****************************************************************************
 *   rules.l - Flex Rules for TIPS Language Subset
 *   MSU CSE 4714/6714 Capstone Project (Fall 2025)
 *   Author: Derek Willis
*****************************************************************************/
%option noyywrap nodefault nounput
%option yylineno

%{
#include "lexer.h"
%}
    
%%
 /* DEFINE YOUR RULES STARTING HERE */

PROGRAM           { return PROGRAM; }
BEGIN           { return TOK_BEGIN; }
END                 { return END; }
WRITE               { return WRITE; }
SPAWN               { return CUSTOM; }

VAR             { return VAR; }
[0-9]+\.[0-9]+   { return FLOATLIT; }
[0-9]+          { return INTLIT; }
":="            { return ASSIGN; }
INTEGER         { return INTEGER; }
REAL            { return REAL; }
":"             { return COLON; }
READ            { return READ; }

IF              { return IF; }
THEN            { return THEN; }
ELSE              { return ELSE; }
WHILE            { return WHILE; }

"<"            { return LESSTHAN; }
">"            { return GREATERTHAN; }
"="            { return EQUALTO; }
"!="            { return NOTEQUALTO; }
"<>"            { return NOTEQUALTO; }

AND            { return TOK_AND; }
OR            { return TOK_OR; }
NOT            { return TOK_NOT; }

";"             { return SEMICOLON; }
"("             { return OPENPAREN; }
")"             { return CLOSEPAREN; }

"+="            { return SELF_PLUS; }
"-="            { return SELF_MINUS; }
"*="            { return SELF_MULTIPLY; }
"/="            { return SELF_DIVIDE; }

"+"            { return PLUS; }
"-"            { return MINUS; }
"*"            { return MULTIPLY; }
"/"            { return DIVIDE; }
MOD            { return MOD; }

"##".*                { /*line skip*/ }
"/*"([^*]|\*+[^*/])*\*+"/"   { /*block skip*/ }

[ \t\r\n]+                { }

[A-Z][A-Z0-9]{0,7}           { return IDENT; }
[A-Z][A-Z0-9]{8,}           { return UNKNOWN;}


'[^'\n]{0,78}'              { return STRINGLIT; }
'[^\n']{79,}'                { return UNKNOWN; }

.                   { return UNKNOWN; }
<<EOF>>             { return TOK_EOF; }

%%
