%{
    char Input[100];
    int SizeInput = 0;
%}
%option noyywrap
INT [0-9]+
PLUS \+
STAR \*
LP \(
RP \)

%%
{INT} {Input[SizeInput++] = 'i';}
{PLUS} {Input[SizeInput++] = '+';}
{STAR} {Input[SizeInput++] = '*';}
{LP} {Input[SizeInput++] = '(';}
{RP} {Input[SizeInput++] = ')';}

[\f\n\r\t\v\x20] {}
. {printf("TYPE A ERROR: Undefined key '%s'\n", yytext);}
%%