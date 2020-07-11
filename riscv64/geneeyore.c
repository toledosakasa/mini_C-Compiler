#include "globals.h"
#include "geneeyore.h"
#include "util.h"
// #include "gentrigger.h"

int eelino = 0;



Eestmt *newEestmt(stmtkind kind,char op0, int val0, char op1, int val1, char op2, int val2)
{
    Eestmt *t = (Eestmt *)malloc(sizeof(Eestmt));
    for (int i = 0; i < 3; i++)
    {
        t->prev[i] = NULL;
        t->next[i] = NULL;
    }
    t->funname = NULL;
    t->kind = kind;
    t->lineno = eelino++;
    t->newlineno = -1;
    t->val[0] = val0;
    t->val[1] = val1;
    t->val[2] = val2;
    t->op[0] = op0;
    t->op[1] = op1;
    t->op[2] = op2;
    t->prev[0] = now;
    memset(t->alive_T,0,sizeof(t->alive_T));
    memset(t->alive_t,0,sizeof(t->alive_t));
    memset(t->alive_p,0,sizeof(t->alive_p));
    t->alivecount = 0;
    now->next[0] = t;
    now = t;
    return t;
}



Eeglobal *newEeglobal()
{

    Eeglobal *t = (Eeglobal *)malloc(sizeof(Eeglobal));
    t->next = NULL;
    t->varid = -1;//初始化varid，之后要用
    if(globalnow)
        globalnow->next = t;
    else
        globalstart = t;
    
    globalnow = t;
    return t;
}





int Tid = 2;
int tid = 0;
int lid = 0;
int pid = 0;
int savevnum;



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


Variable *newVariable(TreeNode *var)
{
    if (var->nodekind != StmtK)
    {
        fprintf(listing, "Not stmt error at variable %s at line %d\n", var->attr.name, var->lineno);
        exit(0);
    }
    Variable *t = (Variable *)malloc(sizeof(Variable));
    int i;
    if (t == NULL)
    {
        fprintf(listing, "Out of memory error at variable %s at line %d\n", var->attr.name, var->lineno);
        exit(0);
    }
    else
    {
        t->ispvar = 0;
        if (var->kind.stmt == Var_DefnK || var->kind.stmt == Var_DeclK)
        {
            t->name = var->attr.name;
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

            }
            else //-2 为decl的数组，正数为定义的数组的大小
            {
                t->kind = IntArray;
            }
            t->funvars = NULL;

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
        if (leftindex < 0)
        {
            fprintf(listing, "error: variable %s not define at line %d\n", name, lineno);
            exit(0);
        }
        if (strcmp(vartable[leftindex]->name, name) == 0 && (vartable[leftindex]->kind == kind ||
                                                             vartable[leftindex]->kind == kind2))
        {
            return vartable[leftindex];
        }
    }
    return NULL;
}

stmtkind eeop;

const char *getopchar(TokenType op)
{
    eeop = OR_E;
    if (op == OR)
        return "||";
    eeop = AND_E;
    if (op == AND)
        return "&&";
    eeop = EQ_E;
    if (op == EQ)
        return "==";
    eeop = NE_E;
    if (op == NE)
        return "!=";
    eeop = LT_E;
    if (op == LT)
        return "<";
    eeop = GT_E;
    if (op == GT)
        return ">";
    eeop = PLUS_E;
    if (op == PLUS)
        return "+";
    eeop = MINUS_E;
    if (op == MINUS)
        return "-";
    eeop = TIMES_E;
    if (op == TIMES)
        return "*";
    eeop = OVER_E;
    if (op == OVER)
        return "/";
    eeop = MOD_E;
    if (op == MOD)
        return "%";
    eeop = NOT_E;
    if (op == NOT)
        return "!";
    else
    {
        fprintf(listing, "error : Invalid op %d at line %d\n", op, lineno);
        exit(0);
    }
    return NULL;
}

int funstmtfirst;//是否是函数中的第一句话阶段
int isinfun; //是否当前的代码在fun中（非main）

void geneeyore(TreeNode *tree)
{
    while (tree != NULL)
    {
        if (tree->nodekind == StmtK)
        {
            if (tree->kind.stmt == RootK)
            {
                // fprintf(listing, "var 400 T0\n");//T0作为调用栈，T1作为栈指针
                // fprintf(listing, "var T1\n");
                geneeyore(tree->child[0]);
                // fprintf(listing, "f_main [0]\n");
                
                Eeglobal * tmp = newEeglobal();
                tmp->funname = (char *)malloc(10);
                strcpy(tmp->funname,"main");
                tmp->funnum = 0;
                tmp->return_stmt = (Eestmt** )malloc(100* sizeof(Eestmt *));
                tmp->return_num = 0;
                funstmtfirst = 1;

                // fprintf(listing, "T1 = 0\n");//初始化T1
                geneeyore(tree->child[1]);
                // fprintf(listing, "end f_main\n");
                tmp->end = now;
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

                    now = (Eestmt *)malloc(sizeof(Eestmt)); //函数开始时见一个START stmt 作为now
                    now->kind = START_E;
                    now->val[0] = -1;
                    now ->op[0] = 0;
                    globalnow->start = now;
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
                int leftindex = vnum - 1;
                int isglobal = 0;
                for (;; leftindex--)
                {
                    if (leftindex < 0 || vartable[leftindex]->kind == lbrace)
                    {
                        if(leftindex<0) //全局变量
                            isglobal = 1;
                        break;
                    }
                    if (strcmp(vartable[leftindex]->name, tree->attr.name) == 0 && (vartable[leftindex]->kind != Fun))
                    {
                        fprintf(listing, "var %s has already defined at line %d \n", tree->attr.name, tree->lineno);
                        exit(0);
                    }
                }

                Variable *p = newVariable(tree);
                pushvar(p);
                if (p->kind == Int)
                    ;
                    // fprintf(listing, "var T%d\n", p->idnum);
                else if (p->kind == IntArray)
                {
                    // fprintf(listing, "var %d T%d\n", 4*p->arraysize, p->idnum);
                }
                if(isglobal)
                {
                    Eeglobal * tmp = newEeglobal();
                    tmp->varid = p->idnum;
                    tmp->arraynum = 4*p->arraysize;//对数组，内存大小*4
                    if(tmp->arraynum == -4) // -1 * 4
                        tmp->kind = 1;
                    else
                        tmp->kind = 2; 
                }
                else if (p->kind == IntArray)
                {
                    newEestmt(ARRAY_DEF_E,'n',4*p->arraysize,'T',p->idnum,0,-1);
                }
            }
            if (tree->kind.stmt == Func_DefnK)
            {
                int leftindex = vnum - 1;
                for (;; leftindex--)
                {
                    if (leftindex < 0 )
                    {
                        break;
                    }
                    //可能有先decline 后define 的情况，这里就先不区分
                    // if (strcmp(vartable[leftindex]->name, tree->attr.name) == 0 && (vartable[leftindex]->kind == Fun))
                    // {
                    //     fprintf(listing, "fun %s has already defined at line %d \n", tree->attr.name, tree->lineno);
                    //     exit(0);
                    // }
                }

                Variable *p = newVariable(tree);
                pushvar(p);
                // fprintf(listing, "f_%s [%d]\n", p->name, p->funvarsnum);
                Eeglobal * tmp = newEeglobal();
                tmp->funname = p->name;
                tmp->funnum = p->funvarsnum;
                tmp->return_stmt = (Eestmt** )malloc(100* sizeof(Eestmt *));
                tmp->return_num = 0;
                funstmtfirst = 1;
                isinfun = 1;
                geneeyore(tree->child[1]);
                tmp->end = now;
                // fprintf(listing, "end f_%s\n", p->name);
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
                // fprintf(listing, "if t%d == 0 goto l%d\n", resultid, label1);
                Eestmt* tmp1 = newEestmt(IF_GOTO_E,'t',resultid,'l',label1,0,-1);
                geneeyore(tree->child[1]);
                // if (tree->child[2] != NULL) //有else
                // {
                int label2 = lid++;
                // fprintf(listing, "goto l%d\n", label2);
                Eestmt* tmp2 = newEestmt(GOTO_E,'l',label2,0,-1,0,-1);
                // fprintf(listing, "l%d:\n", label1);
                Eestmt* tmp3 = newEestmt(LABEL_E,'l',label1,0,-1,0,-1);
                //本来tmp3->prev[0]会是tmp2的，但是由于goto强制跳转，所以要除掉tmp2作为其prev
                tmp3->prev[0] = tmp1;   
                tmp1->next[1] = tmp3;
                geneeyore(tree->child[2]);
                // fprintf(listing, "l%d:\n", label2);
                Eestmt* tmp4 = newEestmt(LABEL_E,'l',label2,0,-1,0,-1);
                tmp4->prev[1] = tmp2;
                tmp2->next[0] = tmp4;//同理，tmp2->next[0]不会是初始自动生成的tmp3,要修改为tmp4
                // }
            }
            if (tree->kind.stmt == WhileK)
            {
                int label1 = lid++;
                int label2 = lid++;
                // fprintf(listing, "l%d:\n", label1);
                Eestmt* tmp1 = newEestmt(LABEL_E,'l',label1,0,-1,0,-1);
                geneeyore(tree->child[0]);
                int resultid = tid - 1;
                // fprintf(listing, "if t%d == 0 goto l%d\n", resultid, label2);
                Eestmt* tmp2 = newEestmt(IF_GOTO_E,'t',resultid,'l',label2,0,-1);
                geneeyore(tree->child[1]);
                // fprintf(listing, "goto l%d\n", label1);
                Eestmt* tmp3 = newEestmt(GOTO_E,'l',label1,0,-1,0,-1);
                // fprintf(listing, "l%d:\n", label2);
                Eestmt* tmp4 = newEestmt(LABEL_E,'l',label2,0,-1,0,-1);
                tmp1->prev[1] = tmp3;
                tmp3->next[0] = tmp1;
                tmp2->next[1] = tmp4;
                tmp4->prev[0] = tmp2;
            }
            if (tree->kind.stmt == AssignK)
            {
                if (tree->child[1] == NULL)
                {
                    lineno = tree->lineno;
                    Variable *var = findvar(tree->attr.name, Int, IntArray); //变量赋值是可以直接赋值给数组地址的
                    geneeyore(tree->child[0]);
                    int resultid = tid - 1;
                    if (var->ispvar)
                    {
                        // fprintf(listing, "p%d = t%d\n", var->idnum, resultid);
                        newEestmt(ASSIGN_E,'p',var->idnum,'t',resultid,0,-1);

                    }
                    else
                    {
                        // fprintf(listing, "T%d = t%d\n", var->idnum, resultid);
                        newEestmt(ASSIGN_E,'T',var->idnum,'t',resultid,0,-1);
                    }
                }
                else //数组赋值
                {
                    geneeyore(tree->child[0]);
                    int indexid = tid - 1;
                    // fprintf(listing, "t%d = 4 * t%d\n", tid, indexid);
                    newEestmt(TIMES_E,'t',tid++,'n',4,'t',indexid);

                    indexid++;
                    pushtid(indexid);
                    lineno = tree->lineno;
                    Variable *var = findvar(tree->attr.name, IntArray, nothing);
                    geneeyore(tree->child[1]);
                    int resultid = tid - 1;
                    if (var->ispvar)
                    {
                        // fprintf(listing, "p%d [t%d] = t%d\n", var->idnum, indexid, resultid);
                        newEestmt(ASS_TO_ARR_E,'p',var->idnum,'t',indexid,'t',resultid);

                    }
                    else
                    {
                        // fprintf(listing, "T%d [t%d] = t%d\n", var->idnum, indexid, resultid);
                        // int flag = 0;
                        // for(Eeglobal* tmp = globalstart;tmp; tmp = tmp->next)
                        // {
                        //     if(tmp->varid == var->idnum)
                        //         flag = 1;
                        // }
                        // if(flag)//用的数组是全局的，因此这个地址之后会被作为变量运算，还是要参与活性分析的
                           newEestmt(ASS_TO_ARR_E,'T',var->idnum,'t',indexid,'t',resultid);
                        // else//局部数组，地址可以直接根据栈得到，不会作为运算数，不参与活性分析
                        //     newEestmt(ASS_TO_ARR_E,'A',var->idnum,'t',indexid,'t',resultid);
                    }
                    poptid();
                }
            }
            if (tree->kind.stmt == ReturnK)
            {
                geneeyore(tree->child[0]);
                int resultid = tid - 1;
                // fprintf(listing, "return t%d\n", resultid);
                tree->sibling = NULL;//return 之后的语句无效
                Eestmt* tmp = newEestmt(RETURN_E,'t',resultid,0,-1,0,-1);
                globalnow->return_stmt[globalnow->return_num++] = tmp; //把return stmt加到该函数的return_stmt中
            }
        }
        else if (tree->nodekind == ExpK)
        {
            if (tree->kind.exp == ConstK)
            {
                // fprintf(listing, "t%d = %d\n", tid, tree->attr.val);
                newEestmt(ASSIGN_E,'t',tid++,'n',tree->attr.val,0,-1);
            }
            if (tree->kind.exp == IdK)
            {
                lineno = tree->lineno;
                Variable *var = findvar(tree->attr.name, Int, IntArray);
                if (var->ispvar)
                {
                    // fprintf(listing, "t%d = p%d\n", tid, var->idnum);
                    newEestmt(ASSIGN_E,'t',tid++,'p',var->idnum,0,-1);
                }
                else
                {
                    // fprintf(listing, "t%d = T%d\n", tid, var->idnum);
                    newEestmt(ASSIGN_E,'t',tid++,'T',var->idnum,0,-1);
                }
            }
            if (tree->kind.exp == OpK)
            {
                //单目运算符,注意用child[1]会有函数调用无参数的情况，用MINUS会有减法误导的情况，因此要两个条件都满足
                if ((tree->attr.op == MINUS || tree->attr.op == NOT) && tree->child[1] == NULL)
                {
                    geneeyore(tree->child[0]);
                    int resultid = tid - 1;
                    lineno = tree->lineno;
                    getopchar(tree->attr.op);
                    // fprintf(listing, "t%d = %s t%d\n", tid, getopchar(tree->attr.op), resultid);
                    if(eeop == MINUS_E)
                        eeop = NEG_E;
                    newEestmt(eeop,'t',tid++,'t',resultid,0,-1);
                }
                else if (tree->attr.op == LBRACK) //数组访问作为表达式
                {
                    lineno = tree->child[0]->lineno;
                    Variable *var = findvar(tree->child[0]->attr.name, IntArray, nothing);
                    geneeyore(tree->child[1]);
                    int indexid = tid - 1;
                    // fprintf(listing, "t%d = 4 * t%d\n", tid, indexid);
                    newEestmt(TIMES_E,'t',tid++,'n',4,'t',indexid);
                    int resultid = tid - 1;
                    if (var->ispvar)
                    {
                        // fprintf(listing, "t%d = p%d [t%d]\n", tid, var->idnum, resultid);
                        newEestmt(ASS_FROM_ARR_E,'t',tid++,'p',var->idnum,'t',resultid);
                    }
                    else
                    {
                        // fprintf(listing, "t%d = T%d [t%d]\n", tid, var->idnum, resultid);
                        // int flag = 0;
                        // for(Eeglobal* tmp = globalstart;tmp; tmp = tmp->next)
                        // {
                        //     if(tmp->varid == var->idnum)
                        //         flag = 1;
                        // }
                        // if(flag)//用的数组是全局的，因此这个地址之后会被作为变量运算，还是要参与活性分析的
                           newEestmt(ASS_FROM_ARR_E,'t',tid++,'T',var->idnum,'t',resultid);
                        // else//局部数组，地址可以直接根据栈得到，不会作为运算数，不参与活性分析
                        //     newEestmt(ASS_FROM_ARR_E,'t',tid++,'A',var->idnum,'t',resultid);
                    }
                }
                else if (tree->attr.op == FUNCALL) //函数调用
                {
                    lineno = tree->child[0]->lineno;
                    Variable *fun = findvar(tree->child[0]->attr.name, Fun, nothing); //在符号表里的函数
                    TreeNode *pfun = fun->funvars;                                    //函数的参数链表
                    TreeNode *pcall = tree->child[1];                                 //调用的函数的参数链表
                    while (pcall != NULL && pfun != NULL)
                    {
                        int isarray = (pfun->num_for_array == -2); //函数需求的参数是不是数组
                        if (pcall->kind.exp == IdK) //函数调用的参数是ID
                        {
                            lineno = pcall->lineno;
                            Variable *param = findvar(pcall->attr.name, Int, IntArray);
                            int realarray = (param->kind == IntArray); //实际上传进去的是不是数组
                            if (isarray == realarray)
                            {
                                if (param->ispvar)
                                {
                                    // fprintf(listing, "param p%d\n", param->idnum);
                                    newEestmt(PARAM_E,'p',param->idnum,0,-1,0,-1);
                                }
                                else
                                {
                                    // fprintf(listing, "param T%d\n", param->idnum);
                                    
                                    if(realarray)
                                    {
                                        // int flag = 0;
                                        // for(Eeglobal* tmp = globalstart;tmp; tmp = tmp->next)
                                        // {
                                        //     if(tmp->varid == param->idnum)
                                        //         flag = 1;
                                        // }
                                        // if(flag == 1)//压得是全局数组的地址
                                           newEestmt(PARAM_E,'T',param->idnum,0,-1,0,-1);
                                        // else
                                        //     newEestmt(PARAM_E,'A',param->idnum,0,-1,0,-1);
                                    }
                                    else
                                        newEestmt(PARAM_E,'T',param->idnum,0,-1,0,-1);
                                }
                            }
                            else//类型检查出出错
                            {
                                fprintf(listing, "type error at param %s, given is %s at line %d\n", pfun->attr.name,
                                        pcall->attr.name, lineno);
                                exit(0);
                            }
                        }
                        else                //函数调用的参数是表达式
                        {
                            if (isarray == 1)//表达式只会是Int型，所以如果要求是数组，则类型检查出错
                            {
                                fprintf(listing, "type error at %s, should be IntArray given is Int at line %d\n",
                                        pfun->attr.name, lineno);
                                exit(0);
                            }
                            TreeNode* save_sibling = pcall->sibling;
                            pcall->sibling = NULL; //这次只算一个，否则会把exp_seq里的都求出来
                            geneeyore(pcall);
                            pcall->sibling = save_sibling;
                            int resultid = tid - 1;
                            // fprintf(listing, "param t%d\n", resultid);
                            newEestmt(PARAM_E,'t',resultid,0,-1,0,-1);
                        }
                        pcall = pcall->sibling;
                        pfun = pfun->sibling;
                        //printf("%lld %lld\n",pcall, pfun);
                    }
                    if (pcall) //调用时还有多的参数
                    {
                        fprintf(listing, "call error at %s, given extra params at line %d\n", fun->name, lineno);
                        exit(0);
                    }
                    if (pfun) //本需要更多参数
                    {
                        fprintf(listing, "call error at %s, need more params at line %d\n", fun->name, lineno);
                        exit(0);
                    }
                    //要开始把这个函数中定义的T和t都压栈(p貌似它会自动保存)，之后再弹出，
                    //为了避免两次调用同一个过程时，同样的代码执行的时候回相互覆盖结果
                    if(isinfun)//main中的调用不用担心，就不用压栈了
                    {
                        for(int i= savevnum; i< vnum; i++)
                        {
                            if(vartable[i]->kind == Int || vartable[i]->kind == IntArray)
                            {
                                // fprintf(listing, "T0 [T1] = T%d\n", vartable[i]->idnum);
                                // fprintf(listing, "T1 = T1 + 4\n");
                            }
                        }
                        for(int i= 0; i< tnum; i++)
                        {
                            // fprintf(listing, "T0 [T1] = t%d\n", tidtable[i]);
                            // fprintf(listing, "T1 = T1 + 4\n");
                        }
                    }

                    // fprintf(listing, "t%d = call f_%s\n", tid, fun->name);
                    Eestmt* tmp = newEestmt(CALL_E,'t',tid++,0,-1,0,-1);
                    tmp->funname = fun->name;

                    if(isinfun)//弹栈
                    {
                        for(int i= tnum -1; i>= 0; i--)//注意call的时候多了一个t
                        {
                            // fprintf(listing, "T1 = T1 - 4\n");
                            // fprintf(listing, "t%d = T0 [T1]\n", tidtable[i]);
                        }
                        for(int i= vnum - 1; i>= savevnum; i--)
                        {
                            if(vartable[i]->kind == Int || vartable[i]->kind == IntArray)
                            {
                            //     fprintf(listing, "T1 = T1 - 4\n");
                            //     fprintf(listing, "T0 [T1] = T%d\n", vartable[i]->idnum);
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
                    lineno = tree->lineno;
                    getopchar(tree->attr.op);
                    // fprintf(listing, "t%d = t%d %s t%d\n", tid, resultid1,
                    //         getopchar(tree->attr.op), resultid2);
                    newEestmt(eeop,'t',tid++,'t',resultid1,'t',resultid2);
                    poptid();
                }
            }
        }
        tree = tree->sibling;
    }
}


