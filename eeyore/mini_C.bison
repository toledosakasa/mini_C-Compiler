/****************************************************/
/* File: tiny.y                                     */
/* The TINY Yacc/Bison specification file           */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/
%{
#define YYPARSER /* distinguishes Yacc output from other code files */
#define YYERROR_VERBOSE
#define YYDEBUG 1
// int yydebug = 1;
#include "globals_.h"
#include "util.h"
#include "scan.h"
#include "geneeyore.h"
// #include "parse.h"

#define YYSTYPE TreeNode *
static int savedLineNo;  /* ditto */
static TreeNode * savedTree; /* stores syntax tree for later return */
// static TreeNode * helpnode = NULL;
int lineno = 0;
int Error = FALSE;
FILE* source; /* source code text file */
FILE* listing ;/* send listing to screen */
static int yylex(void);
%}

%token IF ELSE WHILE RETURN INT MAIN
%token ID NUM 
%token EQ ASSIGN PLUS MINUS TIMES OVER MOD LT GT AND OR NE NOT FUNCALL
%token LPAREN RPAREN LBRACK RBRACK LBRACE RBRACE SEMI COMMA
%token ERROR 

%% /* Grammar for mini_C */


// program     : exp
//                  { savedTree = $1;} 
//             ;


goal        : defs main_func 
                {
                    $$ = newStmtNode(RootK);
                    $$->child[0] = $1;
                    $$->child[1] = $2;
                    savedTree = $$;
                }
            ;

main_func   : INT MAIN LPAREN RPAREN LBRACE st_fd_seq RBRACE 
                {                    
                    $$ = newStmtNode(FieldK);
                    $$->child[0] = $6;
                }
            ;

defs        : %empty {$$ = NULL;}
            | defs three_def 
                {
                    YYSTYPE t = $1;
                    if (t != NULL)
                    { 
                        while (t->sibling != NULL)
                        t = t->sibling;
                        t->sibling = $2;
                        $$ = $1; 
                    }
                    else $$ = $2;
                } 
            ;
three_def   : var_defn {$$ = $1;}
            | func_defn {$$ = $1;}
            | func_decl {$$ = $1;}

var_defn    : INT ID SEMI
                {
                    $$ = newStmtNode(Var_DefnK);
                    $$->attr.name = copyString(IDname);
                }
            | INT ID LBRACK NUM RBRACK SEMI
                {
                    $$ = newStmtNode(Var_DefnK);
                    $$->attr.name = copyString(IDname);
                    $$->num_for_array = numvalue;//numvalue是NUM先前被lex解析时存的
                }
            ;

var_decl    : INT ID
                {
                    $$ = newStmtNode(Var_DeclK);
                    $$->attr.name = copyString(IDname);
                }
            | INT ID LBRACK NUM RBRACK
                {
                    $$ = newStmtNode(Var_DeclK);
                    $$->attr.name = copyString(IDname);
                    $$->num_for_array=-2;//代表decl,声明的NUM无意义
                }
            | INT ID LBRACK RBRACK
                {
                    $$ = newStmtNode(Var_DeclK);
                    $$->attr.name = copyString(IDname);
                    $$->num_for_array=-2;//代表decl
                }
            ;

conflict_assist : INT ID 
                {
                    pushname(IDname);
                    savedLineNo = lineno; /*词法解析完ID后先把名字存起来*/
                }

func_defn   : conflict_assist
                LPAREN var_in_fun RPAREN LBRACE st_fd_seq RBRACE
                {
                    $$ = newStmtNode(Func_DefnK);
                    $$->attr.name = popname();
                    $$->lineno = savedLineNo;
                    $$->child[0] = $3;
                    $$->child[1] = newStmtNode(FieldK);
                    $$->child[1]->child[0] = $6;
                    $$->child[1]->child[1] = $3; // 为了代码生成那里不做大修改而写的
                }

func_decl   : conflict_assist
                LPAREN var_in_fun RPAREN SEMI
                {
                    $$ = newStmtNode(Func_DeclK);
                    $$->attr.name = popname();
                    $$->lineno = savedLineNo;
                    $$->child[0] = $3;
                }

var_in_fun  : %empty {$$ = NULL;}
            | var_decl {$$ = $1;}
            | var_in_fun COMMA var_decl
                {
                    YYSTYPE t = $1;
                    if (t != NULL)
                    { 
                        while (t->sibling != NULL)
                        t = t->sibling;
                        t->sibling = $3;
                        $$ = $1; 
                    }
                    else $$ = $3;
                } 
            ;

st_fd_seq   : %empty {$$ = NULL;}
            | st_fd_seq stmt
                {
                    YYSTYPE t = $1;
                    if (t != NULL)
                    { 
                        while (t->sibling != NULL)
                        t = t->sibling;
                        t->sibling = $2;
                        $$ = $1; 
                    }
                    else $$ = $2;
                } 
            | st_fd_seq func_decl
                {
                    YYSTYPE t = $1;
                    if (t != NULL)
                    { 
                        while (t->sibling != NULL)
                        t = t->sibling;
                        t->sibling = $2;
                        $$ = $1; 
                    }
                    else $$ = $2;
                } 
            ;

stmt        : LBRACE stmts RBRACE 
                {
                    $$ = newStmtNode(FieldK);
                    $$->child[0] = $2;
                }
            | if_stmt {$$ = $1;}
            | while_stmt {$$ = $1;}
            | assign_stmt {$$ = $1;}
            | var_defn {$$ = $1;}
            | return_stmt {$$ = $1;}
            | fun_call SEMI{$$ = $1;}
            ;

stmts       : stmt {$$ = $1;}
            | stmts stmt 
                {
                    YYSTYPE t = $1;
                    if (t != NULL)
                    { 
                        while (t->sibling != NULL)
                        t = t->sibling;
                        t->sibling = $2;
                        $$ = $1; 
                    }
                    else $$ = $2;
                 
                } 
            ;

if_stmt     : IF LPAREN exp RPAREN stmt
                { 
                    $$ = newStmtNode(IfK);
                    $$->child[0] = $3;
                    $$->child[1] = $5;
                }
            | IF LPAREN exp RPAREN stmt ELSE stmt //会有一个移进/规约冲突, if if else 的话，会像c一样让else和近的if匹配
                { 
                    $$ = newStmtNode(IfK);
                    $$->child[0] = $3;
                    $$->child[1] = $5;
                    $$->child[2] = $7;
                }
            ;

while_stmt  : WHILE LPAREN exp RPAREN stmt
                {
                    $$ = newStmtNode(WhileK);
                    $$->child[0] = $3;
                    $$->child[1] = $5;
                }
            ;

assign_stmt : ID 
                { 
                    pushname(IDname);
                    savedLineNo = lineno; /*词法解析完ID后先把名字存起来*/
                }
              ASSIGN exp SEMI
                { 
                    $$ = newStmtNode(AssignK);
                    $$->child[0] = $4;
                    $$->attr.name = popname();
                    $$->lineno = savedLineNo;
                }
            | ID
                { 
                    pushname(IDname);
                    savedLineNo = lineno; /*词法解析完ID后先把名字存起来*/
                }
              LBRACK exp RBRACK ASSIGN exp SEMI
                { 
                    $$ = newStmtNode(AssignK);
                    $$->child[0] = $4;
                    $$->child[1] = $7;
                    $$->attr.name = popname();
                    $$->lineno = savedLineNo;
                }
            ;

return_stmt : RETURN exp SEMI
                {
                    $$ = newStmtNode(ReturnK);
                    $$->child[0] = $2;
                }
            ;

exp_in_fun  : %empty {$$ = NULL;}
            | exp {$$ = $1;}
            | exp_in_fun COMMA exp
                { 
                    YYSTYPE t = $1;
                    if (t != NULL)
                    { 
                        while (t->sibling != NULL)
                            t = t->sibling;
                        t->sibling = $3;
                        $$ = $1;
                    }
                    else $$ = $3;
                }
            ;

exp         : exp OR exp1
                { 
                    $$ = newExpNode(OpK);
                    $$->child[0] = $1;
                    $$->child[1] = $3;
                    $$->attr.op = OR;
                }
            | exp1 {$$ = $1;}
            ;

exp1        : exp1 AND exp2
                { 
                    $$ = newExpNode(OpK);
                    $$->child[0] = $1;
                    $$->child[1] = $3;
                    $$->attr.op = AND;
                }
            | exp2 {$$ = $1;}
            ;

exp2        : exp2 EQ exp3
                { 
                    $$ = newExpNode(OpK);
                    $$->child[0] = $1;
                    $$->child[1] = $3;
                    $$->attr.op = EQ;
                }
            | exp2 NE exp3
                { 
                    $$ = newExpNode(OpK);
                    $$->child[0] = $1;
                    $$->child[1] = $3;
                    $$->attr.op = NE;
                }
            | exp3 {$$ = $1;}
            ;

exp3        : exp3 LT exp4
                { 
                    $$ = newExpNode(OpK);
                    $$->child[0] = $1;
                    $$->child[1] = $3;
                    $$->attr.op = LT;
                }
            | exp3 GT exp4
                { 
                    $$ = newExpNode(OpK);
                    $$->child[0] = $1;
                    $$->child[1] = $3;
                    $$->attr.op = GT;
                }
            | exp4 {$$ = $1;}
            ;

exp4        : exp4 PLUS exp5
                { 
                    $$ = newExpNode(OpK);
                    $$->child[0] = $1;
                    $$->child[1] = $3;
                    $$->attr.op = PLUS;
                }
            | exp4 MINUS exp5
                {   
                    $$ = newExpNode(OpK);
                    $$->child[0] = $1;
                    $$->child[1] = $3;
                    $$->attr.op = MINUS;
                } 
            | exp5 {$$ = $1;}
            ;

exp5        : exp5 TIMES exp6
                { 
                    $$ = newExpNode(OpK);
                    $$->child[0] = $1;
                    $$->child[1] = $3;
                    $$->attr.op = TIMES;
                }
            | exp5 OVER exp6
                { 
                    $$ = newExpNode(OpK);
                    $$->child[0] = $1;
                    $$->child[1] = $3;
                    $$->attr.op = OVER;
                }
            | exp5 MOD exp6
                { 
                    $$ = newExpNode(OpK);
                    $$->child[0] = $1;
                    $$->child[1] = $3;
                    $$->attr.op = MOD;
                }
            | exp6 {$$ = $1;}
            ;

exp6        : MINUS exp7
                { 
                    $$ = newExpNode(OpK);
                    $$->child[0] = $2;
                    $$->attr.op = MINUS;
                }
            | NOT exp7
                { 
                    $$ = newExpNode(OpK);
                    $$->child[0] = $2;
                    $$->attr.op = NOT;
                }
            | exp7 {$$ = $1;}
            ;

exp7        : LPAREN exp RPAREN {$$ = $2;}
            | NUM
                { 
                    $$ = newExpNode(ConstK);
                    $$->attr.val = numvalue;
                }
            | ID
                { 
                    $$ = newExpNode(IdK);
                    $$->attr.name = copyString(IDname);
                }
            | ID 
                { 
                    pushname(IDname);
                    savedLineNo = lineno; /*词法解析完ID后先把名字存起来*/
                }
                LBRACK exp RBRACK
                {
                    $$ = newExpNode(OpK);
                    $$->child[0] = newExpNode(IdK);
                    $$->child[0]->attr.name = popname();
                    $$->child[1] = $4 ;
                    $$->attr.op = LBRACK;/*用'['作为访问数组的运算符*/
                }        
            | fun_call {$$ = $1;}
            ;

fun_call    : ID
                { 
                    pushname(IDname);
                    savedLineNo = lineno; /*词法解析完ID后先把名字存起来*/
                }
                LPAREN exp_in_fun RPAREN
                {
                    $$ = newExpNode(OpK);
                    $$->child[0] = newExpNode(IdK);
                    $$->child[0]->attr.name = popname();
                    $$->child[1] = $4 ;
                    $$->attr.op = FUNCALL;/*用FUNCALL作为访问数组的运算符*/
                }      


%%

int yyerror(char *message)
{
    fprintf(listing, "Syntax error at line %d: %s\n", lineno, message);
    fprintf(listing, "Current token: ");
    printToken(yychar, tokenString);
    Error = TRUE;
    return 0;
}

/* yylex calls getToken to make Yacc/Bison output
 * compatible with ealier versions of the TINY scanner
 */
static int yylex(void)
{
    int a = getToken();
    //printf("inyylex %d\n",a);
    return a;
}

TreeNode *parse(void)
{
    yyparse();
    // printf("%s\n",helpnode->attr.name);
    return savedTree;
}

int main(int argc, char** argv)
{
    if (argc > 1)
    { /*open the file*/
        source = fopen(argv[1], "r");
        if (!source)
        {
            fprintf(stderr, "could not open %s\n", argv[1]);
            exit(1);
        }
    }
    else
        source = stdin;
    if (argc > 2)
    { /*open the file*/
        listing = fopen(argv[2], "w");
        if (!listing)
        {
            fprintf(stderr, "could not open %s\n", argv[1]);
            exit(1);
        }
    }
    else
        listing = stdout;

    TreeNode *syntaxTree;
    syntaxTree = parse();
    // PrintAll(syntaxTree);
    geneeyore(syntaxTree);
    return 0;
}
