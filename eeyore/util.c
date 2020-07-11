/****************************************************/
/* File: util.c                                     */
/* Utility function implementation                  */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "util.h"

#define MAXSAVENUM 1000

char *savename[1000];
int savenum = 0;



/* Procedure printToken prints a token 
 * and its lexeme to the listing file
 */
void printToken(TokenType token, const char *tokenString)
{
    switch (token)
    {
    case IF:
    // case THEN:
    case ELSE:
        // case END:
        // case REPEAT:
        // case UNTIL:
        // case READ:
        // case WRITE:
        fprintf(listing,
                "reserved word: %s\n", tokenString);
        break;

    case FUNCALL:
        fprintf(listing, "FUNCALL\n");
        break;
    case ASSIGN:
        fprintf(listing, ":=\n");
        break;
    case AND:
        fprintf(listing, "&&\n");
        break;
    case OR:
        fprintf(listing, "||\n");
        break;
    case NOT:
        fprintf(listing, "!\n");
        break;
    case LT:
        fprintf(listing, "<\n");
        break;
    case GT:
        fprintf(listing, ">\n");
        break;
    case EQ:
        fprintf(listing, "==\n");
        break;
    case NE:
        fprintf(listing, "!=\n");
        break;
    case LPAREN:
        fprintf(listing, "(\n");
        break;
    case RPAREN:
        fprintf(listing, ")\n");
        break;
    case LBRACK:
        fprintf(listing, "[]\n"); //用作数组访问
        break;
    case SEMI:
        fprintf(listing, ";\n");
        break;
    case PLUS:
        fprintf(listing, "+\n");
        break;
    case MINUS:
        fprintf(listing, "-\n");
        break;
    case TIMES:
        fprintf(listing, "*\n");
        break;
    case OVER:
        fprintf(listing, "/\n");
        break;
    case MOD:
        fprintf(listing, "%%\n");
        break;
    case ENDFILE:
        fprintf(listing, "EOF\n");
        break;
    case NUM:
        fprintf(listing,
                "NUM, val= %s\n", tokenString);
        break;
    case ID:
        fprintf(listing,
                "ID, name= %s\n", tokenString);
        break;
    case ERROR:
        fprintf(listing,
                "ERROR: %s\n", tokenString);
        break;
    default: /* should never happen */
        fprintf(listing, "Unknown token: %d\n", token);
    }
}

/* Function newStmtNode creates a new statement
 * node for syntax tree construction
 */
TreeNode *newStmtNode(StmtKind kind)
{
    TreeNode *t = (TreeNode *)malloc(sizeof(TreeNode));
    int i;
    if (t == NULL)
        fprintf(listing, "Out of memory error at line %d\n", lineno);
    else
    {
        for (i = 0; i < MAXCHILDREN; i++)
            t->child[i] = NULL;
        t->sibling = NULL;
        t->nodekind = StmtK;
        t->kind.stmt = kind;
        t->lineno = lineno;
        t->num_for_array = -1; //和数组无关
    }
    return t;
}

/* Function newExpNode creates a new expression 
 * node for syntax tree construction
 */
TreeNode *newExpNode(ExpKind kind)
{
    //fprintf(listing, "%d %d\n", kind, lineno);

    TreeNode *t = (TreeNode *)malloc(sizeof(TreeNode));
    int i;
    if (t == NULL)
        fprintf(listing, "Out of memory error at line %d\n", lineno);
    else
    {
        for (i = 0; i < MAXCHILDREN; i++)
            t->child[i] = NULL;
        t->sibling = NULL;
        t->nodekind = ExpK;
        t->kind.exp = kind;
        t->lineno = lineno;
        // t->type = Void;
        t->num_for_array = -1; //和数组无关
    }
    return t;
}

/* Function copyString allocates and makes a new
 * copy of an existing string
 */
char *copyString(char *s)
{
    int n;
    char *t;
    if (s == NULL)
        return NULL;
    n = strlen(s) + 1;
    t = (char*) malloc(n);
    if (t == NULL)
        fprintf(listing, "Out of memory error at line %d\n", lineno);
    else
        strcpy(t, s);
    return t;
}

/* Variable indentno is used by printTree to
 * store current number of spaces to indent
 */
static int indentno = 0;

/* macros to increase/decrease indentation */
#define INDENT indentno += 3
#define UNINDENT indentno -= 3

/* printSpaces indents by printing spaces */
static void printSpaces(void)
{
    int i;
    for (i = 0; i < indentno; i++)
    {
        if (i % 3 == 0)
            fprintf(listing, "|");
        else
            fprintf(listing, " ");
    }
}


/* procedure printTree prints a syntax tree to the 
 * listing file using indentation to indicate subtrees
 */

void PrintAll(TreeNode *root)
{
    printTree(root->child[0]);
    fprintf(listing, "Int Main () {}\n");
    printTree(root->child[1]);
}

void printTree(TreeNode *tree)
{
    // printf("In print tree\n");
    int i;
    INDENT;
    while (tree != NULL)
    {
        printSpaces();
        if (tree->nodekind == StmtK)
        {
            switch (tree->kind.stmt)
            {
            case IfK:
                fprintf(listing, "If\n");
                break;
            case WhileK:
                fprintf(listing, "While\n");
                break;
            case AssignK:
                fprintf(listing, "Assign to: %s\n", tree->attr.name);
                break;
            case ReturnK:
                fprintf(listing, "Return\n");
                break;
            case Var_DefnK:
                if (tree->num_for_array == -1)
                    fprintf(listing, "Var_Defn : Int %s ;\n", tree->attr.name);
                else
                    fprintf(listing, "Var_Defn : Int %s [%d] ;\n", tree->attr.name, tree->num_for_array);
                break;
            case Var_DeclK:
                if (tree->num_for_array == -1)
                    fprintf(listing, "Var_Decl : Int %s\n", tree->attr.name);
                else
                    fprintf(listing, "Var_Decl : Int %s [] ;\n", tree->attr.name);
                break;
            case Func_DefnK:
                fprintf(listing, "Func_Defn : Int %s () {}\n", tree->attr.name);
                break;
            case Func_DeclK:
                fprintf(listing, "Func_Decl : Int %s ()\n", tree->attr.name);
                break;
            case FieldK:
                fprintf(listing, "Field \n");
                break;
            case RootK:
                break;
            default:
                fprintf(listing, "Unknown ExpNode kind\n");
                break;
            }
        }
        else if (tree->nodekind == ExpK)
        {
            switch (tree->kind.exp)
            {
            case OpK:
                fprintf(listing, "Op: ");
                printToken(tree->attr.op, "\0");
                break;
            case ConstK:
                fprintf(listing, "Const: %d\n", tree->attr.val);
                break;
            case IdK:
                fprintf(listing, "Id: %s\n", tree->attr.name);
                break;
            default:
                fprintf(listing, "Unknown ExpNode kind\n");
                break;
            }
        }
        else
            fprintf(listing, "Unknown node kind\n");
        // if(tree->nodekind == StmtK && tree->kind.stmt == RootK)
        // {
        //     // printf("%d\n", tree->child[0]->kind.stmt);
        //     // printf("%s\n", tree->child[0]->attr.name);
        //     printTree(tree->child[0]);
        //     fprintf(listing, "Int Main () {}\n");
        //     printTree(tree->child[1]);
        //     break;
        // }
        for (i = 0; i < MAXCHILDREN; i++)
        {
            // if(tree->nodekind == StmtK && tree->kind.stmt == IF && i == 1 && tree->child[i] !=NULL)
            // {
            //     printSpaces();
            //     fprintf(listing, "Else\n");
            // }
            printTree(tree->child[i]);
        }
        tree = tree->sibling;
    }
    UNINDENT;
}

void pushname(char *idname)
{
    savename[savenum++] = copyString(idname);
}

char *popname()
{
    return savename[--savenum];
}


