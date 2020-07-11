#include "globals.h"
#include "geneeyore.h"
#include "util.h"
// #include "util.h"

int Tid = 2;
int tid = 0;
int lid = 0;
int pid = 0;
int savevnum;
// int thisfuntstart;

typedef enum
{
    Int,
    IntArray,
    Fun,
    lbrace, // lbrace 是创建域时压栈的区分符号
    nothing
} VarKind; //nothing是用于变量查找时不需要第二种情况的辅助类型

char nameforlbrace[10] = "\0";

typedef struct variable
{
    char *name;
    VarKind kind;
    int idnum;
    //int intvalue;
    int arraysize;
    int ispvar;
    TreeNode *funvars;
    int funvarsnum;
    //TreeNode* funcode;
} Variable;

// int tnum = 0;
// int fnum = 0;

#define MAXVAR 10000
Variable *vartable[MAXVAR];

int vnum = 0;

void pushvar(Variable *var)
{
    vartable[vnum++] = var;
}

Variable *popvar()
{
    return vartable[--vnum];
}

int tidtable[MAXVAR];

int tnum = 0;

void pushtid(int var)
{
    tidtable[tnum++] = var;
}

int poptid()
{
    return tidtable[--tnum];
}

// #define MAXFUN 10000
// Variable * funtable [MAXFUN];

// int fnum = 0;

// void pushfun(Variable * fun)
// {
//     funtable[fnum++] = fun;
// }

// Variable *popfun()
// {
//     return funtable[--fnum];
// }

Variable *newVariable(TreeNode *var)
{
    if (var->nodekind != StmtK)
    {
        fprintf(listing, "Not stmt error at variable %s\n", var->attr.name);
        return NULL;
    }
    Variable *t = (Variable *)malloc(sizeof(Variable));
    int i;
    if (t == NULL)
        fprintf(listing, "Out of memory error at variable %s\n", var->attr.name);
    else
    {
        t->ispvar = 0;
        if (var->kind.stmt == Var_DefnK || var->kind.stmt == Var_DeclK)
        {
            t->name = var->attr.name;
            // printf("in newvar, new %s from %s\n", t->name, var->attr.name);
            // printf("%lld\n",(long long int) t->name);
            // printf("%lld\n",(long long int) var->attr.name);
            if (var->kind.stmt == Var_DefnK)
                t->idnum = Tid++;
            else
            {
                t->idnum = pid++;
                t->ispvar = 1;
            }
            t->arraysize = var->num_for_array;
            if (var->num_for_array == -1)
            {
                t->kind = Int;
                //fprintf(listing, "var T%d\n", t->idnum);
            }
            else //-2 为decl的数组，正数为定义的数组的大小
            {
                t->kind = IntArray;
                //fprintf(listing, "var %d T%d\n", t->arraysize, t->idnum);
            }
            t->funvars = NULL;
            //t->funcode = NULL;
        }
        else if (var->kind.stmt == Func_DefnK || var->kind.stmt == Func_DeclK)
        {
            t->name = var->attr.name;
            t->kind = Fun;
            t->funvars = var->child[0];
            TreeNode *p = t->funvars;
            while (p)
            {
                t->funvarsnum++;
                p = p->sibling;
            }
        }
    }
    return t;
}

Variable *findvar(char *name, VarKind kind, VarKind kind2)
{
    int leftindex = vnum - 1;
    for (;; leftindex--)
    {
        // printf("name  = %s\n", name);
        // printf("kind  = %d\n", kind);
        // printf("leftindex = %d\n",leftindex);
        // printf("var name = %s\n", vartable[leftindex]->name);
        // printf("var kind = %d\n", vartable[leftindex]->kind);
        // int nameeq = strcmp(vartable[leftindex]->name, name) == 0;
        // printf("name eq = %d, kind eq = %d\n\n", nameeq,
        //                                             vartable[leftindex]->kind == kind);
        // printf("%lld\n",(long long int) vartable[leftindex]->name);
        if (leftindex < 0)
        {
            fprintf(listing, "error: variable not define %s\n", name);
            break;
        }
        if (strcmp(vartable[leftindex]->name, name) == 0 && (vartable[leftindex]->kind == kind ||
                                                             vartable[leftindex]->kind == kind2))
        {
            return vartable[leftindex];
        }
    }
    return NULL;
}

const char *getopchar(TokenType op)
{
    if (op == OR)
        return "||";
    if (op == AND)
        return "&&";
    if (op == EQ)
        return "==";
    if (op == NE)
        return "!=";
    if (op == LT)
        return "<";
    if (op == GT)
        return ">";
    if (op == PLUS)
        return "+";
    if (op == MINUS)
        return "-";
    if (op == TIMES)
        return "*";
    if (op == OVER)
        return "/";
    if (op == MOD)
        return "%";
    if (op == NOT)
        return "!";
    else
        fprintf(listing, "Invalid op %d\n", op);
    return NULL;
}

int funstmtfirst;//是否是函数中的第一句话阶段
int isinfun;

void geneeyore(TreeNode *tree)
{
    while (tree != NULL)
    {
        if (tree->nodekind == StmtK)
        {
            if (tree->kind.stmt == RootK)
            {
                fprintf(listing, "var 400 T0\n");//T0作为调用栈，T1作为栈指针
                fprintf(listing, "var T1\n");
                geneeyore(tree->child[0]);
                fprintf(listing, "f_main [0]\n");
                fprintf(listing, "T1 = 0\n");//初始化T1
                geneeyore(tree->child[1]);
                fprintf(listing, "end f_main\n");
            }
            if (tree->kind.stmt == FieldK)
            {
                Variable *lb = (Variable *)malloc(sizeof(Variable));
                lb->kind = lbrace;
                lb->name = nameforlbrace; //用一个空串来放置strcmp出问题
                lb->idnum = -1;
                pushvar(lb);
                if(funstmtfirst)
                {
                    savevnum = vnum; //把符号表里次函数开始的位置记下来
                    funstmtfirst = 0; //一个函数里存一次就行了
                }
                TreeNode *p = tree->child[1]; //看看这个域是不是函数调用，如果是的话把参数压栈
                pid = 0;                      //每个函数从0开始算
                while (p != NULL)
                {
                    Variable *v = newVariable(p);
                    pushvar(v);
                    p = p->sibling;
                }
                geneeyore(tree->child[0]);
                while (popvar()->kind != lbrace)
                    continue;
            }
            if (tree->kind.stmt == Var_DefnK)
            {
                Variable *p = newVariable(tree);
                pushvar(p);
                if (p->kind == Int)
                    fprintf(listing, "var T%d\n", p->idnum);
                else if (p->kind == IntArray)
                    fprintf(listing, "var %d T%d\n", 4*p->arraysize, p->idnum);
            }
            if (tree->kind.stmt == Func_DefnK)
            {
                Variable *p = newVariable(tree);
                pushvar(p);
                fprintf(listing, "f_%s [%d]\n", p->name, p->funvarsnum);
                funstmtfirst = 1;
                isinfun = 1;
                geneeyore(tree->child[1]);
                fprintf(listing, "end f_%s\n", p->name);
                isinfun =0;
            }
            if (tree->kind.stmt == Func_DeclK)
            {
                Variable *p = newVariable(tree);
                pushvar(p);
            }
            if (tree->kind.stmt == IfK)
            {
                geneeyore(tree->child[0]);
                int label1 = lid++;
                int resultid = tid - 1;
                fprintf(listing, "if t%d == 0 goto l%d\n", resultid, label1);
                geneeyore(tree->child[1]);
                if (tree->child[2] != NULL) //有else
                {
                    int label2 = lid++;
                    fprintf(listing, "goto l%d\n", label2);
                    fprintf(listing, "l%d:\n", label1);
                    geneeyore(tree->child[2]);
                    fprintf(listing, "l%d:\n", label2);
                }
                else
                    fprintf(listing, "l%d:\n", label1);
            }
            if (tree->kind.stmt == WhileK)
            {
                int label1 = lid++;
                int label2 = lid++;
                fprintf(listing, "l%d:\n", label1);
                geneeyore(tree->child[0]);
                int resultid = tid - 1;
                fprintf(listing, "if t%d == 0 goto l%d\n", resultid, label2);
                geneeyore(tree->child[1]);
                fprintf(listing, "goto l%d\n", label1);
                fprintf(listing, "l%d:\n", label2);
            }
            if (tree->kind.stmt == AssignK)
            {
                if (tree->child[1] == NULL)
                {
                    Variable *var = findvar(tree->attr.name, Int, IntArray); //变量赋值是可以直接赋值给数组地址的
                    geneeyore(tree->child[0]);
                    int resultid = tid - 1;
                    if (var->ispvar)
                        fprintf(listing, "p%d = t%d\n", var->idnum, resultid);
                    else
                        fprintf(listing, "T%d = t%d\n", var->idnum, resultid);
                }
                else //数组赋值
                {
                    geneeyore(tree->child[0]);
                    int indexid = tid - 1;
                    fprintf(listing, "t%d = 4 * t%d\n", tid++, indexid);
                    indexid++;
                    pushtid(indexid);
                    Variable *var = findvar(tree->attr.name, IntArray, nothing);
                    geneeyore(tree->child[1]);
                    int resultid = tid - 1;
                    if (var->ispvar)
                        fprintf(listing, "p%d [t%d] = t%d\n", var->idnum, indexid, resultid);
                    else
                        fprintf(listing, "T%d [t%d] = t%d\n", var->idnum, indexid, resultid);
                    poptid();
                }
            }
            if (tree->kind.stmt == ReturnK)
            {
                geneeyore(tree->child[0]);
                int resultid = tid - 1;
                fprintf(listing, "return t%d\n", resultid);
            }
        }
        else if (tree->nodekind == ExpK)
        {
            if (tree->kind.exp == ConstK)
            {
                fprintf(listing, "t%d = %d\n", tid++, tree->attr.val);
            }
            if (tree->kind.exp == IdK)
            {
                Variable *var = findvar(tree->attr.name, Int, IntArray);
                if (var->ispvar)
                    fprintf(listing, "t%d = p%d\n", tid++, var->idnum);
                else
                    fprintf(listing, "t%d = T%d\n", tid++, var->idnum);
            }
            if (tree->kind.exp == OpK)
            {
                //单目运算符,注意用child[1]会有函数调用无参数的情况，用MINUS会有减法误导的情况，因此要两个条件都满足
                if ((tree->attr.op == MINUS || tree->attr.op == NOT) && tree->child[1] == NULL)
                {
                    geneeyore(tree->child[0]);
                    int resultid = tid - 1;
                    fprintf(listing, "t%d = %s t%d\n", tid++, getopchar(tree->attr.op), resultid);
                }
                else if (tree->attr.op == LBRACK) //数组访问作为表达式
                {
                    Variable *var = findvar(tree->child[0]->attr.name, IntArray, nothing);
                    geneeyore(tree->child[1]);
                    int indexid = tid - 1;
                    fprintf(listing, "t%d = 4 * t%d\n", tid++, indexid);
                    int resultid = tid - 1;
                    if (var->ispvar)
                        fprintf(listing, "t%d = p%d [t%d]\n", tid++, var->idnum, resultid);
                    else
                        fprintf(listing, "t%d = T%d [t%d]\n", tid++, var->idnum, resultid);
                }
                else if (tree->attr.op == FUNCALL) //函数调用
                {
                    Variable *fun = findvar(tree->child[0]->attr.name, Fun, nothing); //在符号表里的函数
                    TreeNode *pfun = fun->funvars;                                    //函数的参数链表
                    TreeNode *pcall = tree->child[1];                                 //调用的函数的参数链表
                    while (pcall != NULL || pfun != NULL)
                    {
                        int isarray = (pfun->num_for_array == -2);
                        if (pcall->kind.exp == IdK)
                        {
                            Variable *param = findvar(pcall->attr.name, Int, IntArray);
                            int realarray = (param->kind == IntArray);
                            if (isarray == realarray)
                            {
                                if (param->ispvar)
                                    fprintf(listing, "param p%d\n", param->idnum);
                                else
                                    fprintf(listing, "param T%d\n", param->idnum);
                            }
                            else
                            {
                                fprintf(listing, "type error at param %s, given is %s\n", pfun->attr.name,
                                        pcall->attr.name);
                            }
                        }
                        else
                        {
                            if (isarray == 1)
                            {
                                fprintf(listing, "type error at %s\n, should be IntArray given is Int",
                                        pfun->attr.name);
                            }
                            TreeNode* save_sibling = pcall->sibling;
                            pcall->sibling = NULL; //这次只算一个，否则会把exp_seq里的都求出来
                            geneeyore(pcall);
                            pcall->sibling = save_sibling;
                            int resultid = tid - 1;
                            fprintf(listing, "param t%d\n", resultid);
                        }
                        pcall = pcall->sibling;
                        pfun = pfun->sibling;
                    }
                    if (pcall) //调用时还有多的参数
                    {
                        fprintf(listing, "call error at %s\n, given extra params", fun->name);
                    }
                    if (pfun) //本需要更多参数
                    {
                        fprintf(listing, "call error at %s\n, need more params", fun->name);
                    }
                    //要开始把这个函数中定义的T和t都压栈(p貌似它会自动保存)，之后再弹出，
                    //为了避免两次调用同一个过程时，同样的代码执行的时候回相互覆盖结果
                    if(isinfun)//main中的调用不用担心，就不用压栈了
                    {
                        for(int i= savevnum; i< vnum; i++)
                        {
                            if(vartable[i]->kind == Int || vartable[i]->kind == IntArray)
                            {
                                fprintf(listing, "T0 [T1] = T%d\n", vartable[i]->idnum);
                                fprintf(listing, "T1 = T1 + 4\n");
                            }
                        }
                        for(int i= 0; i< tnum; i++)
                        {
                            fprintf(listing, "T0 [T1] = t%d\n", tidtable[i]);
                            fprintf(listing, "T1 = T1 + 4\n");
                        }
                    }
                    fprintf(listing, "t%d = call f_%s\n", tid++, fun->name);
                    if(isinfun)//弹栈
                    {
                        for(int i= tnum -1; i>= 0; i--)//注意call的时候多了一个t
                        {
                            fprintf(listing, "T1 = T1 - 4\n");
                            fprintf(listing, "t%d = T0 [T1]\n", tidtable[i]);
                        }
                        for(int i= vnum - 1; i>= savevnum; i--)
                        {
                            if(vartable[i]->kind == Int || vartable[i]->kind == IntArray)
                            {
                                fprintf(listing, "T1 = T1 - 4\n");
                                fprintf(listing, "T0 [T1] = T%d\n", vartable[i]->idnum);
                            }
                        }
                    }
                }
                else //双目运算符
                {
                    geneeyore(tree->child[0]);
                    int resultid1 = tid - 1;
                    pushtid(resultid1);
                    geneeyore(tree->child[1]);
                    int resultid2 = tid - 1;
                    fprintf(listing, "t%d = t%d %s t%d\n", tid++, resultid1,
                            getopchar(tree->attr.op), resultid2);
                    poptid();
                }
            }
        }
        tree = tree->sibling;
    }
}

// /****************************************************/
// /* File: util.c                                     */
// /* Utility function implementation                  */
// /* for the TINY compiler                            */
// /* Compiler Construction: Principles and Practice   */
// /* Kenneth C. Louden                                */
// /****************************************************/

// // #include "globals.h"
// // #include "util.h"

// FILE* listing = stdout;
// int lineno = 0;

// #define MAXSAVENUM 1000

// char *savename[1000];
// int savenum = 0;

// /* Procedure printToken prints a token
//  * and its lexeme to the listing file
//  */
// void printToken(TokenType token, const char *tokenString)
// {
//     switch (token)
//     {
//     case IF:
//     // case THEN:
//     case ELSE:
//         // case END:
//         // case REPEAT:
//         // case UNTIL:
//         // case READ:
//         // case WRITE:
//         fprintf(listing,
//                 "reserved word: %s\n", tokenString);
//         break;

//     case FUNCALL:
//         fprintf(listing, "FUNCALL\n");
//         break;
//     case ASSIGN:
//         fprintf(listing, ":=\n");
//         break;
//     case AND:
//         fprintf(listing, "&&\n");
//         break;
//     case OR:
//         fprintf(listing, "||\n");
//         break;
//     case NOT:
//         fprintf(listing, "!\n");
//         break;
//     case LT:
//         fprintf(listing, "<\n");
//         break;
//     case GT:
//         fprintf(listing, ">\n");
//         break;
//     case EQ:
//         fprintf(listing, "==\n");
//         break;
//     case NE:
//         fprintf(listing, "!=\n");
//         break;
//     case LPAREN:
//         fprintf(listing, "(\n");
//         break;
//     case RPAREN:
//         fprintf(listing, ")\n");
//         break;
//     case LBRACK:
//         fprintf(listing, "[]\n"); //用作数组访问
//         break;
//     case SEMI:
//         fprintf(listing, ";\n");
//         break;
//     case PLUS:
//         fprintf(listing, "+\n");
//         break;
//     case MINUS:
//         fprintf(listing, "-\n");
//         break;
//     case TIMES:
//         fprintf(listing, "*\n");
//         break;
//     case OVER:
//         fprintf(listing, "/\n");
//         break;
//     case MOD:
//         fprintf(listing, "%%\n");
//         break;
//     case ENDFILE:
//         fprintf(listing, "EOF\n");
//         break;
//     case NUM:
//         fprintf(listing,
//                 "NUM, val= %s\n", tokenString);
//         break;
//     case ID:
//         fprintf(listing,
//                 "ID, name= %s\n", tokenString);
//         break;
//     case ERROR:
//         fprintf(listing,
//                 "ERROR: %s\n", tokenString);
//         break;
//     default: /* should never happen */
//         fprintf(listing, "Unknown token: %d\n", token);
//     }
// }

// /* Function newStmtNode creates a new statement
//  * node for syntax tree construction
//  */
// TreeNode *newStmtNode(StmtKind kind)
// {
//     TreeNode *t = (TreeNode *)malloc(sizeof(TreeNode));
//     int i;
//     if (t == NULL)
//         fprintf(listing, "Out of memory error at line %d\n", lineno);
//     else
//     {
//         for (i = 0; i < MAXCHILDREN; i++)
//             t->child[i] = NULL;
//         t->sibling = NULL;
//         t->nodekind = StmtK;
//         t->kind.stmt = kind;
//         t->lineno = lineno;
//         t->num_for_array = -1; //和数组无关
//     }
//     return t;
// }

// /* Function newExpNode creates a new expression
//  * node for syntax tree construction
//  */
// TreeNode *newExpNode(ExpKind kind)
// {
//     //fprintf(listing, "%d %d\n", kind, lineno);

//     TreeNode *t = (TreeNode *)malloc(sizeof(TreeNode));
//     int i;
//     if (t == NULL)
//         fprintf(listing, "Out of memory error at line %d\n", lineno);
//     else
//     {
//         for (i = 0; i < MAXCHILDREN; i++)
//             t->child[i] = NULL;
//         t->sibling = NULL;
//         t->nodekind = ExpK;
//         t->kind.exp = kind;
//         t->lineno = lineno;
//         // t->type = Void;
//         t->num_for_array = -1; //和数组无关
//     }
//     return t;
// }

// /* Function copyString allocates and makes a new
//  * copy of an existing string
//  */
// char *copyString(char *s)
// {
//     int n;
//     char *t;
//     if (s == NULL)
//         return NULL;
//     n = strlen(s) + 1;
//     t = (char*) malloc(n);
//     if (t == NULL)
//         fprintf(listing, "Out of memory error at line %d\n", lineno);
//     else
//         strcpy(t, s);
//     return t;
// }

// /* Variable indentno is used by printTree to
//  * store current number of spaces to indent
//  */
// static int indentno = 0;

// /* macros to increase/decrease indentation */
// #define INDENT indentno += 3
// #define UNINDENT indentno -= 3

// /* printSpaces indents by printing spaces */
// static void printSpaces(void)
// {
//     int i;
//     for (i = 0; i < indentno; i++)
//     {
//         if (i % 3 == 0)
//             fprintf(listing, "|");
//         else
//             fprintf(listing, " ");
//     }
// }

// /* procedure printTree prints a syntax tree to the
//  * listing file using indentation to indicate subtrees
//  */

// void PrintAll(TreeNode *root)
// {
//     printTree(root->child[0]);
//     fprintf(listing, "Int Main () {}\n");
//     printTree(root->child[1]);
// }

// void printTree(TreeNode *tree)
// {
//     // printf("In print tree\n");
//     int i;
//     INDENT;
//     while (tree != NULL)
//     {
//         printSpaces();
//         if (tree->nodekind == StmtK)
//         {
//             switch (tree->kind.stmt)
//             {
//             case IfK:
//                 fprintf(listing, "If\n");
//                 break;
//             case WhileK:
//                 fprintf(listing, "While\n");
//                 break;
//             case AssignK:
//                 fprintf(listing, "Assign to: %s\n", tree->attr.name);
//                 break;
//             case ReturnK:
//                 fprintf(listing, "Return\n");
//                 break;
//             case Var_DefnK:
//                 if (tree->num_for_array == -1)
//                     fprintf(listing, "Var_Defn : Int %s ;\n", tree->attr.name);
//                 else
//                     fprintf(listing, "Var_Defn : Int %s [%d] ;\n", tree->attr.name, tree->num_for_array);
//                 break;
//             case Var_DeclK:
//                 if (tree->num_for_array == -1)
//                     fprintf(listing, "Var_Decl : Int %s\n", tree->attr.name);
//                 else
//                     fprintf(listing, "Var_Decl : Int %s [] ;\n", tree->attr.name);
//                 break;
//             case Func_DefnK:
//                 fprintf(listing, "Func_Defn : Int %s () {}\n", tree->attr.name);
//                 break;
//             case Func_DeclK:
//                 fprintf(listing, "Func_Decl : Int %s ()\n", tree->attr.name);
//                 break;
//             case FieldK:
//                 fprintf(listing, "Field \n");
//                 break;
//             case RootK:
//                 break;
//             default:
//                 fprintf(listing, "Unknown ExpNode kind\n");
//                 break;
//             }
//         }
//         else if (tree->nodekind == ExpK)
//         {
//             switch (tree->kind.exp)
//             {
//             case OpK:
//                 fprintf(listing, "Op: ");
//                 printToken(tree->attr.op, "\0");
//                 break;
//             case ConstK:
//                 fprintf(listing, "Const: %d\n", tree->attr.val);
//                 break;
//             case IdK:
//                 fprintf(listing, "Id: %s\n", tree->attr.name);
//                 break;
//             default:
//                 fprintf(listing, "Unknown ExpNode kind\n");
//                 break;
//             }
//         }
//         else
//             fprintf(listing, "Unknown node kind\n");
//         // if(tree->nodekind == StmtK && tree->kind.stmt == RootK)
//         // {
//         //     // printf("%d\n", tree->child[0]->kind.stmt);
//         //     // printf("%s\n", tree->child[0]->attr.name);
//         //     printTree(tree->child[0]);
//         //     fprintf(listing, "Int Main () {}\n");
//         //     printTree(tree->child[1]);
//         //     break;
//         // }
//         for (i = 0; i < MAXCHILDREN; i++)
//         {
//             // if(tree->nodekind == StmtK && tree->kind.stmt == IF && i == 1 && tree->child[i] !=NULL)
//             // {
//             //     printSpaces();
//             //     fprintf(listing, "Else\n");
//             // }
//             printTree(tree->child[i]);
//         }
//         tree = tree->sibling;
//     }
//     UNINDENT;
// }

// void pushname(char *idname)
// {
//     savename[savenum++] = copyString(idname);
// }

// char *popname()
// {
//     return savename[--savenum];
// }

// int main()
// {
//     TreeNode * tree = newStmtNode(RootK);
//     tree ->child[0] = NULL;
//     tree ->child[1] = newStmtNode(FieldK);
//     TreeNode * s1 = newStmtNode(Var_DefnK);
//     char name1[20];
//     name1[0]='a';
//     name1[1]='\0';
//     char name2[20];
//     name2[0]='b';
//     name2[1]='\0';
//     s1->attr.name = name1;
//     TreeNode * s2 = newStmtNode(Var_DefnK);
//     s2->attr.name = name2;
//     TreeNode * e1 = newExpNode(ConstK);
//     e1->attr.val = 10;
//     TreeNode * s3 = newStmtNode(AssignK);
//     s3->child[0] = e1;
//     s3->attr.name = name1;
//     s1->sibling=s2;
//     s2->sibling = s3;
//     tree ->child[1]->child[0] = s1;
//     geneeyore(tree);
//     return 0;
// }