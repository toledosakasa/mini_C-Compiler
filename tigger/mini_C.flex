/****************************************************/
/* File: mini_C.flex                                */
/* Lex specification for mini_C                     */
/****************************************************/

%{
#include "globals.h"
#include "util.h"
#include "scan.h"
// /* lexeme of identifier or reserved word */
char tokenString[MAXTOKENLEN+1];
char IDname[MAXTOKENLEN+1];
int numvalue;

%}

digit       [0-9]
number      {digit}+
letter      [a-zA-Z_]
identifier  {letter}+({letter}|{number})*
newline     \n
return      \r\n
whitespace  [ \t]+

%%

"if"            {return IF;}
"else"          {return ELSE;}
"while"         {return WHILE;}
"return"        {return RETURN;}
"int"           {return INT;}
"main"          {return MAIN;}
"=="            {return EQ;}
"="             {return ASSIGN;}
"+"             {return PLUS;}
"-"             {return MINUS;}
"*"             {return TIMES;}
"/"             {return OVER;}
"%"             {return MOD;}
"<"             {return LT;}
">"             {return GT;}
"&&"            {return AND;}
"||"            {return OR;}
"!="            {return NE;}
"!"             {return NOT;}
"("             {return LPAREN; /* left parenthesis */}
")"             {return RPAREN;}
"["             {return LBRACK; /* left bracket */}
"]"             {return RBRACK;}
"{"             {return LBRACE; /* left brace */}
"}"             {return RBRACE;}
";"             {return SEMI; /* semi-colon */}
","             {return COMMA;}
{number}        {return NUM;}
{identifier}    {return ID;}
{newline}       {lineno++;}
{return}       {lineno++;}
{whitespace}    {/* skip whitespace */}
\/\/.*          {/* skip annotation */}
\/"*".*"*"\/    {/* skip annotation */}
.               {return ERROR;}

%%

// int main()
// {
//     TokenType a;
//     while(a = yylex())
//     {
//         printf("%d ",a);
//     } 
//     return 0;
// }

TokenType getToken(void)
{
    static int firstTime = TRUE;
    TokenType currentToken;
    if (firstTime)
    {
        firstTime = FALSE;
        lineno++;
        yyin = source;
        yyout = listing;
    }
    currentToken = yylex();
    // printf("gettoken %d\n",currentToken);
    strncpy(tokenString, yytext, MAXTOKENLEN);
    if(currentToken==ID)
        strncpy(IDname, yytext, MAXTOKENLEN);
    if(currentToken==NUM)
        numvalue=atoi(tokenString);
    // printf("token is %s\n",tokenString);
    // for (int i=0;i<MAXTOKENLEN; i++)
    //     printf("%d ", tokenString[i]);
    // printf("\n");
    // if (TraceScan)
    // {
    //     fprintf(listing, "\t%d: ", lineno);
    //     printToken(currentToken, tokenString);
    // }
    return currentToken;
}

