// #include "globals.h"
// #include "gentrigger.h"
#include "globals.h"
#include "geneeyore.h"

const char *getstmtchar(stmtkind op)
{
    if (op == OR_E)
        return "||";

    if (op == AND_E)
        return "&&";

    if (op == EQ_E)
        return "==";

    if (op == NE_E)
        return "!=";

    if (op == LT_E)
        return "<";

    if (op == GT_E)
        return ">";

    if (op == PLUS_E)
        return "+";

    if (op == MINUS_E)
        return "-";

    if (op == TIMES_E)
        return "*";

    if (op == OVER_E)
        return "/";

    if (op == MOD_E)
        return "%";

    if (op == NOT_E)
        return "!";

    if (op == NEG_E)
        return "-";

    if (op == ASSIGN_E)
        return "=";

    if (op == ASS_TO_ARR_E)
        return "[] =";

    if (op == ASS_FROM_ARR_E)
        return "= []";

    if (op == IF_GOTO_E)
        return "if goto";

    if (op == GOTO_E)
        return "goto";

    if (op == PARAM_E)
        return "param";

    if (op == CALL_E)
        return "call";
    if (op == START_E)
        return "start this fun";
    if (op == RETURN_E)
        return "return";
    if (op == LABEL_E)
        return "l";
    if (op == ARRAY_DEF_E)
        return "var array ";
    return NULL;
}

void print_stmt(Eestmt *stmt) //打印
{
    fprintf(listing, "%s", getstmtchar(stmt->kind));
    for (int i = 0; stmt->val[i] != -1 && stmt->op[i] != 0; i++)
    {
        fprintf(listing, " %c%d ", stmt->op[i], stmt->val[i]);
    }
    if (stmt->kind == CALL_E)
        fprintf(listing, " f_%s ", stmt->funname);
    fprintf(listing, "\n");
}

void print_alive(Eestmt *stmt)
{
    int countblank = 0;
    fprintf(listing, "alive: ");
    for (int i = 0; i < Tid; i++)
    {
        if (stmt->alive_T[i])
        {
            countblank += fprintf(listing, "T%d ", i);
        }
    }
    for (int i = 0; i < tid; i++)
    {
        if (stmt->alive_t[i])
        {
            countblank += fprintf(listing, "t%d ", i);
        }
    }
    for (int i = 0; i < 10; i++)
    {
        if (stmt->alive_p[i])
        {
            countblank += fprintf(listing, "p%d ", i);
        }
    }
    for (int i = 30 - countblank; i >= 0; i--)
        fprintf(listing, " ");
    fprintf(listing, "|| ");
    print_stmt(stmt);
}

Eestmt *second[10000];
int second_index = 0;

void pushsecond(Eestmt *var)
{
    second[second_index++] = var;
}

Eestmt *popsecond()
{
    // if(second_index == 0)
    //     exit(0);
    return second[--second_index];
}

#define MAXVAR 10000
int Tval_id[MAXVAR]; //存放所有的T的值,配合op一起使用, 应该不用初始化？
char Tval_op[MAXVAR];
int tval_id[MAXVAR];
char tval_op[MAXVAR];
int pval_id[MAXVAR];
char pval_op[MAXVAR];

int *val_id;
char *val_op;

int globalTid[10000];
int globalkind[10000];
int globalTid_index = 0;

void val_choose(char a)
{
    if (a == 'T')
    {
        val_id = Tval_id;
        val_op = Tval_op;
    }
    else if (a == 't')
    {
        val_id = tval_id;
        val_op = tval_op;
    }
    else if (a == 'p')
    {
        val_id = pval_id;
        val_op = pval_op;
    }
    else
    {
        //fprintf(listing, "debug: val_choose error , not var\n");
    }
}

void copy_optimize(Eestmt *stmt)
{
    //复写传播
    stmtkind thekind = stmt->kind;
    if (thekind == ASSIGN_E)
    {
        if (stmt->op[1] == 'n') //右侧值为数字
        {
            val_choose(stmt->op[0]);
            val_op[stmt->val[0]] = stmt->op[1];
            val_id[stmt->val[0]] = stmt->val[1];
        }
        else //右侧为变量p, T, t
        {
            val_choose(stmt->op[1]);
            char op1 = val_op[stmt->val[1]];
            int id1 = val_id[stmt->val[1]];
            if (op1 != 0)
            {
                val_choose(stmt->op[0]);
                val_op[stmt->val[0]] = op1;
                val_id[stmt->val[0]] = id1;
                stmt->op[1] = op1;
                stmt->val[1] = id1;
            }
            else
            {
                val_choose(stmt->op[0]);
                val_op[stmt->val[0]] = stmt->op[1];
                val_id[stmt->val[0]] = stmt->val[1];
            }
        }
    }
    else if (thekind <= PARAM_E) //对于语句中的右值，check是否可以复写
    {
        int starti = 1;
        if (thekind == RETURN_E || thekind == PARAM_E)
            starti = 0;
        for (int i = starti; i < 3; i++)
        {
            if (stmt->op[i] == 't' || stmt->op[i] == 'T' || stmt->op[i] == 'p')
            {
                val_choose(stmt->op[i]);
                char op1 = val_op[stmt->val[i]];
                int id1 = val_id[stmt->val[i]];

                if (op1 != 0)
                {
                    stmt->op[i] = op1;
                    stmt->val[i] = id1;
                }
            }
        }
        if (!(thekind == RETURN_E || thekind == PARAM_E))
        {
            val_choose(stmt->op[0]);
            val_op[stmt->val[0]] = 0;//删掉左值的复写值
        }
    }
    else if (thekind == CALL_E) //对函数调用的优化
    {
        //对全局变量，删掉复写值
        for (int i = 0; i < globalTid_index; i++)
            Tval_op[globalTid[i]] = 0;

        // if (stmt->val[0] == stmt->next[0]->val[1] && stmt->next[0]->op[1] == 't')
        // {
        //     stmt->val[0] = stmt->next[0]->val[0];
        //     stmt->op[0] = stmt->next[0]->op[0];
        //     stmt->next[0] = stmt->next[0]->next[0]; //删掉T1 = t1 这种语句
        //     free(stmt->next[0]->prev[0]);
        //     stmt->next[0]->prev[0] = stmt;
        // }

        val_choose(stmt->op[0]);
        val_op[stmt->val[0]] = 0;//删掉左值的复写值
    }
}

int newlineno = 1;
//这两个都是为了生成trigger的时候确定栈的大小
int maxalivenum = 0; //统计最大的活跃变量数
int arraysize[1000]; //同时把函数内开数组的情况记下来
int arrayid[1000];
int array_index = 0;

typedef struct chainnode
{
    int val;
    struct chainnode *next;
} cn;

cn *Tuse[2000];
cn *tuse[5000];
cn *puse[2000];

//得到活性区间，顺便做一些杂事
void alive_scan(Eestmt *stmt)
{
    if (stmt->kind == ARRAY_DEF_E)
    {
        arraysize[array_index] = stmt->val[0];
        arrayid[array_index] = stmt->val[1];
        array_index++;
    }

    if (stmt->kind == LT_E && stmt->op[1] == 'n') //小于且左边是数字的话，交换一下,省的后面处理
    {
        stmt->kind = GT_E;
        stmt->op[1] = stmt->op[2];
        stmt->op[2] = 'n';
        int tmp = stmt->val[1];
        stmt->val[1] = stmt->val[2];
        stmt->val[2] = tmp;
    }

    if (stmt->newlineno = -1)
        stmt->newlineno = newlineno++;

    if (stmt->alivecount > maxalivenum)
        maxalivenum = stmt->alivecount;

    if (stmt->kind <= IF_GOTO_E)
    {
        int starti = 1;
        if (stmt->kind >= ASS_TO_ARR_E)
            starti = 0;
        for (int i = starti; i < 3; i++)
        {
            cn *tmp;
            if (stmt->op[i] == 'T')
                tmp = Tuse[stmt->val[i]];
            else if (stmt->op[i] == 't')
                tmp = tuse[stmt->val[i]];
            else if (stmt->op[i] == 'p')
                tmp = puse[stmt->val[i]];
            else //'n' 和'l' 直接跳走
                continue;
            // printf("%c%d\n",stmt->op[i],stmt->val[i]);
            while (tmp->next != NULL)
            {
                tmp = tmp->next;
            }
            tmp->next = (cn *)malloc(sizeof(cn));
            tmp->next->val = stmt->newlineno;
            tmp->next->next = NULL;
            // fprintf(listing,"%c%d use in %d\n",stmt->op[i],stmt->val[i],tmp->val);
        }
    }
}

int domath(int a, int b, stmtkind op)
{
    if (op == PLUS_E)
        return a + b;
    if (op == MINUS_E)
        return a - b;
    if (op == TIMES_E)
        return a * b;
    if (op == OVER_E)
        return a / b;
    if (op == MOD_E)
        return a % b;
    if (op == AND_E)
        return a && b;
    if (op == OR_E)
        return a || b;
    if (op == EQ_E)
        return a == b;
    if (op == NE_E)
        return a != b;
    if (op == LT_E)
        return a < b;
    if (op == GT_E)
        return a > b;
    if (op == NOT_E)
        return !a;
    if (op == NEG_E)
        return -a;

    //fprintf(listing, "debug: error op %d in domath\n", op);
    return -1;
}

//变量的符号表, op= 'r'说明在寄存器里，从1到27编号
//op ='s'说明在栈里， 如果是store进去的，就会存位置
//op = 'g'说明为全局变量，数组的话也在这里,数组的地址会被加载，因为要参与运算。所有变量被加载了又存进去的话
//的话还要切回来，全局变量放'g'对应的位置，根据globalT索引，局部变量放栈里
int Tp_val[2000];
char Tp_op[2000];
int tp_val[5000];
char tp_op[5000];
int pp_val[10];
char pp_op[10];
int vid = 0;
int *p_val;
char *p_op;
void pchoose(char a)
{
    if (a == 'T')
    {
        p_val = Tp_val;
        p_op = Tp_op;
    }
    else if (a == 't')
    {
        p_val = tp_val;
        p_op = tp_op;
    }
    else if (a == 'p')
    {
        p_val = pp_val;
        p_op = pp_op;
    }
    else
    {
        //fprintf(listing, "debug: pchoose error , not var\n");
    }
}

int ri;
char rc;
void reg_change(int reg)
{
    if (reg <= 12)
    {
        rc = 's';
        ri = reg - 1;
    }
    else if (reg <= 19)
    {
        rc = 't';
        ri = reg - 13;
    }
    else if (reg <= 27)
    {
        rc = 'a';
        ri = reg - 20;
    }
    else
    {
        //fprintf(listing, "debug: reg error at reg %d \n", reg);
    }
}

//描述寄存器存放的东西，每次新函数要清零
int reg_val[28];
char reg_op[28];
//描述局部栈存放的东西，每次新函数也清零

char stack_op[10000];
int stack_val[10000];

int thes[2000][2000]; //语句距离的数组

//选择一个reg，appoint_reg是指定的寄存器
int choose_reg(Eestmt *stmt, int appoint_reg)
{
    int maxend = 0;
    int retid;
    int isdead = 0;
    for (int i = 1; i < 28; i++)
    {
        if(i == 19 || i == 18)//19号专门用来存全局变量的地址以及其他临时变量, 18号用力爱对付需要两个临时变量的指令
            continue;
        if (appoint_reg != -1 && appoint_reg != i) //对于存在指定寄存器的情况，其余的全部continue
            continue;
        cn *tmp;
        if (reg_op[i] == 'T')
        {
            if (!stmt->alive_T[reg_val[i]])
            {
                isdead = 1;
                retid = i;
                break;
            }
            tmp = Tuse[reg_val[i]]->next;
        }
        if (reg_op[i] == 't')
        {
            if (!stmt->alive_t[reg_val[i]])
            {
                isdead = 1;
                retid = i;
                break;
            }
            tmp = tuse[reg_val[i]]->next;
        }
        if (reg_op[i] == 'p')
        {
            if (!stmt->alive_p[reg_val[i]])
            {
                isdead = 1;
                retid = i;
                break;
            }
            tmp = puse[reg_val[i]]->next;
        }
        if (reg_op[i] == 'T' || reg_op[i] == 't' || reg_op[i] == 'p') //查看存放变量的活跃区间
        {
            if (tmp) //活跃且有过程内的使用
            {
                int useinthestate = 0;
                while (tmp)
                {
                    int thelength = thes[stmt->newlineno][tmp->val];
                    if (thelength > 0)
                        useinthestate = 1;
                    if (maxend < thelength)
                    {
                        maxend = thelength;
                        retid = i;
                    }
                    tmp = tmp->next;
                }
                if (!useinthestate) //没有可达路径被用却活跃，说明是全局变量，且要被保存
                    retid = i;
            }
            else
            {
                retid = i; //就返回它了，可能是局部变量，虽然在末尾也活跃，但是就存起来
                // break;
            }
        }
        else if (reg_op[i] == 0) //op是0，代表是空的，直接返回
        {
            isdead = 1;
            retid = i;
            break;
        }
    }

    int isglobal = 0;
    int gid = -1;
    for (int i = 0; i < globalTid_index; i++)
    {
        // fprintf(listing,"g%d , %d\n",i,globalTid[i]);
        // fprintf(listing,"reg%d : %c%d\n",retid, reg_op[retid],reg_val[retid]);
        if (globalTid[i] == reg_val[retid] && reg_op[retid] == 'T')
        {
            gid = i;
            isglobal = globalkind[i];
            break;
        }
    }

    if (isglobal == 1) // 全局变量,要存回去
    {
        int reg1 = 19;
        reg_change(reg1);
        fprintf(listing, "loadaddr v%d %c%d \n", gid, rc, ri);
        fprintf(listing, "%c%d [0] = ", rc, ri);
        reg_change(retid);
        fprintf(listing, "%c%d\n", rc, ri);
        Tp_op[reg_val[retid]] = 'g';
        Tp_val[reg_val[retid]] = gid;
    }
    else if (isglobal == 2)
    {
        //数组首地址的话，就只改状态
        Tp_op[reg_val[retid]] = 'g';
        Tp_val[reg_val[retid]] = gid;
    }
    else //局部变量
    {
        if (!isdead) //仍然活跃的局部变量
        {
            for (int i = 0; i < maxalivenum; i++)
            {
                // fprintf(listing,"stack[%d] %d\n",i,stack_op[i]);
                if (stack_op[i] == 0)
                {
                    reg_change(retid);
                    fprintf(listing, "store %c%d %d\n", rc, ri, i);
                    stack_op[i] = reg_op[retid];
                    stack_val[i] = reg_val[retid];
                    pchoose(reg_op[retid]);
                    p_op[reg_val[retid]] = 's';
                    p_val[reg_val[retid]] = i;
                    break;
                }
            }
        }
    }
    reg_op[retid] = 0; //不管怎么样，取出来了之后，就认为这是一个空的寄存器，之后用了它的话其状态会在那里修改

    return retid; //返回寄存器的值
}

//把要的变量加到寄存器里去，appoint是指定的寄存器
int getvar(Eestmt *stmt, char op, int val, int appoint)
{
    int reg;
    pchoose(op);
    // printf("%c%d\n",stmt->op[vari],stmt->val[vari]);
    // printf("%d\n",p_op[stmt->val[vari]]);
    // if(op == 'T' && val == 2)
    // {
    //     fprintf(listing,"%c%d\n",Tp_op[val],Tp_val[val]);
    // }

    if (p_op[val] == 'r') //需要的变量就在寄存器里
    {
        if (appoint == -1)
        {
            reg = p_val[val];
        }
        else
        {
            //刚好相同
            if (p_val[val] == appoint) //刚好在appoint里了
                return appoint;
            else //把那个寄存器的东西放到appoint里去
            {
                int reg1;
                reg1 = p_val[val];
                choose_reg(stmt, appoint); //如果a0有活跃东西，那就通过这个指定选择把其存起来
                reg_change(appoint);
                fprintf(listing, "%c%d = ", rc, ri);
                reg_change(reg1);
                fprintf(listing, "%c%d\n", rc, ri);
                return appoint;
                //这里就不修改变量的保存情况了，视为原来的reg1还是那个变量的保存者,
                //因为只有函数调用和返回的时候才会让appoint!=-1,存疑???
            }
        }
    }
    else if (p_op[val] == 's') //在栈里
    {
        reg = choose_reg(stmt, appoint);
        reg_change(reg);
        pchoose(op);//要重新pchoose，因为choose_reg可能出错
        fprintf(listing, "load %d %c%d\n", p_val[val], rc, ri);

        if (appoint == -1) //因为只有函数调用和返回的时候才会让appoint!=-1，此时不需要修改变量状态
        {
            reg_val[reg] = val;
            reg_op[reg] = op;
            stack_op[p_val[val]] = 0;
            p_op[val] = 'r';
            p_val[val] = reg;
        }
    }
    else if (p_op[val] == 'g') //在全局变量里
    {
        reg = choose_reg(stmt, appoint);
        reg_change(reg);
        int isglobalarray = 0;
        int gid = -1;
        for (int i = 0; i < globalTid_index; i++)
        {
            if (globalTid[i] == val)
            {
                gid = i;
                if (globalkind[i] == 2)
                    isglobalarray = 1;
                break;
            }
        }
        if (isglobalarray)
            fprintf(listing, "loadaddr v%d %c%d\n", gid, rc, ri);
        else
            fprintf(listing, "load v%d %c%d\n", gid, rc, ri);

        // if (gid != p_val[val])
        //     fprintf(listing, "debug: error in global var gid\n");

        if (appoint == -1) //因为只有函数调用和返回的时候才会让appoint!=-1，此时不需要修改变量状态
        {
            reg_val[reg] = val;
            reg_op[reg] = op;
            // fprintf(listing,"T%d alive is %d\n",val,stmt->alive_T[val]);
            pchoose(op);//同理，重新pchoose
            p_op[val] = 'r';
            p_val[val] = reg;
        }
    }

    else //这里没有这个变量,可能是数据流中该变量此处未定义导致的
    {
        return -2;
    }

    // printf("get var reg = %d\n",reg);
    return reg;
}

typedef struct env
{
    char savep_op[2000];
    int savep_val[2000];
    int savereg_val[28];
    char savereg_op[28];
    char savestack_op[1000];
    int savestack_val[1000];
} Env;

Env save_stack[1000];
int save_stack_index = 0;

//把stmt处的变量ci存到栈的stackn那里去
void save_assist(Eestmt *stmt, int stackn, char c, int i, int *savestackn)
{
    pchoose(c);
    if (p_op[i] == 's' && p_val[i] == stackn) //已经在合适的位置了，不用处理了
        return;
    else if (p_op[i] == 'r') //在寄存器里
    {
        if (stack_op[stackn] != 0) //要存的位置已经被占用了
        {
            int reg = 19; //用一个临时寄存器把这个占地的变量移动位置
            reg_change(reg);
            fprintf(listing, "load %d %c%d\n", stackn, rc, ri);
            fprintf(listing, "store %c%d %d\n", rc, ri, *savestackn);
            pchoose(stack_op[stackn]);
            p_val[stack_val[stackn]] = *savestackn;
            *savestackn = *savestackn + 1;
        }
        pchoose(c);
        if(reg_op[p_val[i]] != 1)//没被上锁
            reg_op[p_val[i]] = 0; //逻辑上清空这个寄存器存的东西
        reg_change(p_val[i]);
        fprintf(listing, "store %c%d %d\n", rc, ri, stackn);
    }

    else if (p_op[i] == 's') //在栈里
    {

        if (stack_op[stackn] != 0) //要存的位置已经被占用了
        {
            int reg = 19; //用一个临时寄存器把这个占地的变量移动位置
            reg_change(reg);
            fprintf(listing, "load %d %c%d\n", stackn, rc, ri);
            fprintf(listing, "store %c%d %d\n", rc, ri, *savestackn);
            pchoose(stack_op[stackn]);
            p_val[stack_val[stackn]] = *savestackn;
            *savestackn = *savestackn + 1;
        }
        pchoose(c);
        int reg = 19; //用一个临时寄存器把这个要移动的变量移动到适当的位置
        reg_change(reg);
        fprintf(listing, "load %d %c%d\n", p_val[i], rc, ri);
        fprintf(listing, "store %c%d %d\n", rc, ri, stackn);
        stack_op[p_val[i]] = 0; //清空原来那地方的栈的op
    }

    //若此处没定义这个变量，但是还是要存，因为要和其他的分支对应交汇之后的情况
    pchoose(c);
    p_op[i] = 's';
    p_val[i] = stackn;
    stack_op[stackn] = c;
    stack_val[stackn] = i;
}

//保存活跃变量，stmt2用来里指定是存哪一条语句的那些活跃变量,stmt用来表示是当前的状态
//只有funcall那里的savealive会用到stmt2
void save_alive(Eestmt *stmt, Eestmt *stmt2)
{
    if (stmt2 == NULL)
        stmt2 = stmt;
    int stackn = -1;
    int savestackn = maxalivenum;

    for (int i = 0; i < Tid; i++)
    {
        if (stmt2->alive_T[i])
        {
            if (Tp_op[i] == 'g') //全局变量已经在全局变量那里了
                continue;

            int gid = -1;
            int isglobal = 0;
            for (int j = 0; j < globalTid_index; j++)
                if (globalTid[j] == i) //该变量是全局变量,且在寄存器中（不然的话上一步就continue走了）
                {
                    gid = j;
                    isglobal = globalkind[j];
                    break;
                }

            if (gid > -1) //这个变量是全局变量且不是全局数组，而且在寄存器中，要把它存起来
            {
                if(isglobal == 1)
                {
                    int reg1 = 19;
                    reg_change(reg1);
                    fprintf(listing, "loadaddr v%d %c%d \n", gid, rc, ri);
                    fprintf(listing, "%c%d [0] = ", rc, ri);
                    reg_change(Tp_val[i]);
                    fprintf(listing, "%c%d\n", rc, ri);
                }
                if(reg_op[Tp_val[i]] != 1)//没被上锁
                    reg_op[Tp_val[i]] = 0;//要清空寄存器
                Tp_op[i] = 'g';
                Tp_val[i] = gid;
                continue;
            }
            //这个变量是局部变量，要存到栈里去
            stackn++; //按这个顺序一个一个地存,这种方式下，没定义却已经活跃的变量也没关系，
            save_assist(stmt, stackn, 'T', i, &savestackn);
        }
    }

    for (int i = 0; i < tid; i++)
    {
        if (stmt2->alive_t[i])
        {
            stackn++; //按这个顺序一个一个地存
            save_assist(stmt, stackn, 't', i, &savestackn);
        }
    }

    for (int i = 0; i < 10; i++)
    {
        if (stmt2->alive_p[i])
        {
            stackn++; //按这个顺序一个一个地存
            save_assist(stmt, stackn, 'p', i, &savestackn);
        }
    }
}

int paramnum = 0;

void gentrigger(Eestmt *stmt)
{
    //op2 运算
    if (stmt->kind <= GT_E)
    {
        int reg1 = -1, reg2 = -1;
        char saveop1 = 0;
        if (stmt->op[1] != 'n')
        {
            reg1 = getvar(stmt, stmt->op[1], stmt->val[1], -1);
            saveop1  =  reg_op[reg1];
            reg_op[reg1] = 1;//加锁,防止之后getvar时由于那个变量在栈里会生成指令占用reg1
            // if(reg1 == -2)
            //     return;
        }
        if (stmt->op[2] != 'n')
        {
            reg2 = getvar(stmt, stmt->op[2], stmt->val[2], -1);
            // if(reg2 == -2)
            //     return;
        }
        if(saveop1)
            reg_op[reg1] = saveop1;

        int reg0;
        if (reg1 == -1 && reg2 == -1) //数字op数字,直接算出来赋值给寄存器
        {
            if (stmt->op[1] != 'n' || stmt->op[2] != 'n')
            {
                //fprintf(listing, "debug: Not number error at variable at line two num opreators\n");
            }
            
            int res = domath(stmt->val[1], stmt->val[2], stmt->kind);
            reg0 = choose_reg(stmt->next[0], -1); //取reg0的时候有可能会存全局变量,不过也没关系了
            reg_change(reg0);
            fprintf(listing, "%c%d = %d\n", rc, ri, res);
        }
        else if (reg1 == -1 || reg2 == -1) //有一个是数字
        {
            int thereg = reg1;
            int thenum = stmt->val[2];
            if (reg1 == -1)
            {
                thereg = reg2;
                thenum = stmt->val[1];
            }
            if (stmt->kind == PLUS_E || stmt->kind == LT_E) //可以用reg = reg op2 int 小于的话，右边一定是数字
            {
                reg0 = choose_reg(stmt->next[0], -1);
                reg_change(reg0);
                fprintf(listing, "%c%d = ", rc, ri);
                reg_change(thereg);
                fprintf(listing, "%c%d %s %d\n", rc, ri, getstmtchar(stmt->kind), thenum);
            }
            else
            {
                reg0 = choose_reg(stmt->next[0], -1);
                int reg3 = 19;
                reg_change(reg3);
                fprintf(listing, "%c%d = %d\n", rc, ri, thenum);
                if (reg1 == -1)   //左数是num
                {
                    reg_change(reg0);
                    fprintf(listing, "%c%d = ", rc, ri);
                    reg_change(reg3);
                    fprintf(listing, "%c%d %s ", rc, ri, getstmtchar(stmt->kind));
                    reg_change(thereg);
                    fprintf(listing, "%c%d\n", rc, ri);
                }
                else
                {
                    reg_change(reg0);
                    fprintf(listing, "%c%d = ", rc, ri);
                    reg_change(thereg);
                    fprintf(listing, "%c%d %s ", rc, ri, getstmtchar(stmt->kind));
                    reg_change(reg3);
                    fprintf(listing, "%c%d\n", rc, ri);
                }
            }
        }
        else
        {
            reg0 = choose_reg(stmt->next[0], -1); //先取reg0出来，算完了再改状态
            reg_change(reg0);
            fprintf(listing, "%c%d = ", rc, ri);
            reg_change(reg1);
            fprintf(listing, "%c%d %s ", rc, ri, getstmtchar(stmt->kind));
            reg_change(reg2);
            fprintf(listing, "%c%d\n", rc, ri);
        }
        //把设置关于reg0的状态

        //清空此时存了左操作数的寄存器和栈，防止以后出问题
        for (int i = 1; i < 28; i++)
            if (reg_op[i] == stmt->op[0] && reg_val[i] == stmt->val[0])
                reg_op[i] = 0;

        for (int i = 0; i < maxalivenum; i++)
            if (stack_op[i] == stmt->op[0] && stack_val[i] == stmt->val[0])
                stack_op[i] = 0;

        reg_val[reg0] = stmt->val[0];
        reg_op[reg0] = stmt->op[0];
        pchoose(stmt->op[0]);
        p_op[stmt->val[0]] = 'r';
        p_val[stmt->val[0]] = reg0;
    }

    //op1运算
    else if (stmt->kind <= NEG_E)
    {
        int reg0;
        int reg1 = -1;
        if (stmt->op[1] == 'n')
        {
            int res = domath(stmt->val[1], 0, stmt->kind); //单目运算
            reg0 = choose_reg(stmt->next[0], -1);
            reg_change(reg0);
            fprintf(listing, "%c%d = %d\n", rc, ri, res);
        }
        else
        {
            reg1 = getvar(stmt, stmt->op[1], stmt->val[1], -1);
            reg0 = choose_reg(stmt->next[0], -1);
            reg_change(reg0);
            fprintf(listing, "%c%d = %s ", rc, ri, getstmtchar(stmt->kind));
            reg_change(reg1);
            fprintf(listing, "%c%d\n", rc, ri);
        }

        //清空此时存了左操作数的寄存器和栈，防止以后出问题
        for (int i = 1; i < 28; i++)
            if (reg_op[i] == stmt->op[0] && reg_val[i] == stmt->val[0])
                reg_op[i] = 0;

        for (int i = 0; i < maxalivenum; i++)
            if (stack_op[i] == stmt->op[0] && stack_val[i] == stmt->val[0])
                stack_op[i] = 0;

        reg_val[reg0] = stmt->val[0];
        reg_op[reg0] = stmt->op[0];
        pchoose(stmt->op[0]);
        p_op[stmt->val[0]] = 'r';
        p_val[stmt->val[0]] = reg0;
    }

    //有点怪
    else if (stmt->kind == ASSIGN_E)
    {
        int reg0;
        if (stmt->op[1] == 'n')
        {
            reg0 = choose_reg(stmt->next[0], -1);
            reg_change(reg0);
            fprintf(listing, "%c%d = %d\n", rc, ri, stmt->val[1]);
        }
        else
        {
            int reg1 = getvar(stmt, stmt->op[1], stmt->val[1], -1);
            reg0 = choose_reg(stmt->next[0], -1); //先取reg0出来
            if (reg0 != reg1)           //如果二者相等，说明reg1对应的变量在之后失活，说明只是复写，可以优化掉
            {
                reg_change(reg0);
                fprintf(listing, "%c%d = ", rc, ri);
                reg_change(reg1);
                fprintf(listing, "%c%d\n", rc, ri);
            }
        }

        //清空此时存了左操作数的寄存器和栈，防止以后出问题
        for (int i = 1; i < 28; i++)
            if (reg_op[i] == stmt->op[0] && reg_val[i] == stmt->val[0])
                reg_op[i] = 0;

        for (int i = 0; i < maxalivenum; i++)
            if (stack_op[i] == stmt->op[0] && stack_val[i] == stmt->val[0])
                stack_op[i] = 0;

        reg_val[reg0] = stmt->val[0];
        reg_op[reg0] = stmt->op[0];
        pchoose(stmt->op[0]);
        p_op[stmt->val[0]] = 'r';
        p_val[stmt->val[0]] = reg0;
    }

    else if (stmt->kind == ARRAY_DEF_E)
    {
        int stackp = 2 * maxalivenum; //0-maxlivenum-1都是存变量用的，maxliveunum到 2m -1是用于save_alive的临时地址
                                      //之后是一个一个的局部数组
        for (int i = 0; i < array_index; i++)
        {
            if (arrayid[i] == stmt->val[1])
            {
                break;
            }
            stackp += arraysize[i]; //计算这个数组的起始地址
        }
        int reg0 = choose_reg(stmt, -1);
        reg_change(reg0);
        fprintf(listing, "loadaddr %d %c%d \n", stackp, rc, ri);
        reg_val[reg0] = stmt->val[1];
        reg_op[reg0] = stmt->op[1];
        pchoose(stmt->op[1]);
        p_op[stmt->val[1]] = 'r';
        p_val[stmt->val[1]] = reg0;
    }

    //涉及到数组
    else if (stmt->kind == ASS_FROM_ARR_E)
    {
        int reg1 = getvar(stmt, stmt->op[1], stmt->val[1], -1); //起始地址，用getvar来得到
        char saveop1 = reg_op[reg1];
        reg_op[reg1] = 1;//加锁
        int reg2 = getvar(stmt, stmt->op[2], stmt->val[2], -1); //得到偏移量
        reg_op[reg1] = saveop1;
        int reg0 = choose_reg(stmt->next[0], -1); //先取reg0出来，算完了再改状态
        int reg3 = 19;                        //临时寄存器

        reg_change(reg3);
        fprintf(listing, "%c%d = ", rc, ri);
        reg_change(reg1);
        fprintf(listing, "%c%d + ", rc, ri);
        reg_change(reg2);
        fprintf(listing, "%c%d\n", rc, ri); //计算偏移量

        reg_change(reg0);
        fprintf(listing, "%c%d = ", rc, ri);
        reg_change(reg3);
        fprintf(listing, "%c%d[0]\n", rc, ri);
        //清空此时存了左操作数的寄存器和栈，防止以后出问题
        for (int i = 1; i < 28; i++)
            if (reg_op[i] == stmt->op[0] && reg_val[i] == stmt->val[0])
                reg_op[i] = 0;

        for (int i = 0; i < maxalivenum; i++)
            if (stack_op[i] == stmt->op[0] && stack_val[i] == stmt->val[0])
                stack_op[i] = 0;
        reg_val[reg0] = stmt->val[0];
        reg_op[reg0] = stmt->op[0];
        pchoose(stmt->op[0]);
        p_op[stmt->val[0]] = 'r';
        p_val[stmt->val[0]] = reg0;
    }

    //对数组赋值
    else if (stmt->kind == ASS_TO_ARR_E)
    {
        //fprintf(listing,"%c%d in %c%d\n",stmt->op[0],stmt->val[0], Tp_op[stmt->val[0]], Tp_val[stmt->val[0]]);
        int reg1 = getvar(stmt, stmt->op[0], stmt->val[0], -1); //起始地址，用getvar来得到
        char saveop1 = reg_op[reg1];
        int reg2 = getvar(stmt, stmt->op[1], stmt->val[1], -1); //得到偏移量
        reg_op[reg1] = saveop1;
        int reg3 = 19; //临时寄存器
        reg_change(reg3);
        fprintf(listing, "%c%d = ", rc, ri);

        reg_change(reg1);
        fprintf(listing, "%c%d + ", rc, ri);
        reg_change(reg2);
        fprintf(listing, "%c%d\n", rc, ri); //计算偏移量

        int reg4;
        if (stmt->op[2] == 'n')
        {
            reg4 = 18; //临时寄存器
            reg_change(reg4);
            fprintf(listing, "%c%d = %d\n", rc, ri, stmt->val[2]);
        }
        else
            reg4 = getvar(stmt, stmt->op[2], stmt->val[2], -1);
        reg_op[reg3] = 0;
        reg_change(reg3);
        fprintf(listing, "%c%d[0] = ", rc, ri);
        reg_change(reg4);
        fprintf(listing, "%c%d\n", rc, ri);
    }

    else if (stmt->kind == IF_GOTO_E)
    {
        // if(stmt->next[1]->prev[1] == NULL)//有else的 if  goto，要保存现场
        // {
        for (int i = 1; i < 28; i++) //要按照栈的顺序保存
        {
            save_stack[save_stack_index].savereg_val[i] = reg_val[i];
            save_stack[save_stack_index].savereg_op[i] = reg_op[i];
        }
        for (int i = 0; i < maxalivenum; i++)
        {
            save_stack[save_stack_index].savestack_op[i] = stack_op[i];
            save_stack[save_stack_index].savestack_val[i] = stack_val[i];
        }

        int j = 0;
        for (int i = 0; i < Tid; i++)
        {
            if (stmt->next[1]->alive_T[i])
            {
                save_stack[save_stack_index].savep_op[j] = Tp_op[i];
                save_stack[save_stack_index].savep_val[j] = Tp_val[i];
                j++;
            }
            //接下来要把不跳转的分支中，失活的变量给从栈里消除,存疑???
            if (!stmt->next[0]->alive_T[i] && stmt->alive_T[i] && Tp_op[i] == 's')
                stack_op[Tp_val[i]] = 0;
        }
        for (int i = 0; i < tid; i++)
        {
            if (stmt->next[1]->alive_t[i])
            {
                save_stack[save_stack_index].savep_op[j] = tp_op[i];
                save_stack[save_stack_index].savep_val[j] = tp_val[i];
                j++;
            }
            //接下来要把不跳转的话，失活的变量给从栈里消除
            if (!stmt->next[0]->alive_t[i] && stmt->alive_t[i] && tp_op[i] == 's')
                stack_op[tp_val[i]] = 0;
        }
        for (int i = 0; i < 10; i++)
        {
            if (stmt->next[1]->alive_p[i])
            {
                save_stack[save_stack_index].savep_op[j] = pp_op[i];
                save_stack[save_stack_index].savep_val[j] = pp_val[i];
                j++;
            }
            //接下来要把不跳转的话，失活的变量给从栈里消除
            if (!stmt->next[0]->alive_p[i] && stmt->alive_p[i] && pp_op[i] == 's')
                stack_op[pp_val[i]] = 0;
        }

        save_stack_index++;
        // else//要保存活跃变量
        // {
        //     save_alive(stmt);
        // }
        int reg1 = getvar(stmt, stmt->op[0], stmt->val[0], -1);
        reg_change(reg1);
        fprintf(listing, "if %c%d == x0 goto l%d\n", rc, ri, stmt->val[1]);
    }
    else if (stmt->kind == GOTO_E) //下一个语句肯定是交汇处的label，要存活跃变量
    {
        save_alive(stmt, NULL);
        fprintf(listing, "goto l%d\n", stmt->val[0]);
    }
    else if (stmt->kind == LABEL_E)
    {
        if (stmt->prev[1] != NULL) //交汇的label，即上一条语句结束了基本快，要存活跃变量
        {
            save_alive(stmt, NULL);
        }
        else //从if goto过来的，恢复当时的环境
        {
            save_stack_index--;
            for (int i = 1; i < 28; i++)
            {
                reg_val[i] = save_stack[save_stack_index].savereg_val[i];
                reg_op[i] = save_stack[save_stack_index].savereg_op[i];
            }

            for (int i = 0; i < maxalivenum; i++)
            {
                stack_op[i] = save_stack[save_stack_index].savestack_op[i];
                stack_val[i] = save_stack[save_stack_index].savestack_val[i];
            }
            int j = 0;
            for (int i = 0; i < Tid; i++)
            {
                if (stmt->alive_T[i])
                {
                    Tp_op[i] = save_stack[save_stack_index].savep_op[j];
                    Tp_val[i] = save_stack[save_stack_index].savep_val[j];
                    j++;
                }
            }
            for (int i = 0; i < tid; i++)
            {
                if (stmt->alive_t[i])
                {
                    tp_op[i] = save_stack[save_stack_index].savep_op[j];
                    tp_val[i] = save_stack[save_stack_index].savep_val[j];
                    j++;
                }
            }
            for (int i = 0; i < 10; i++)
            {
                if (stmt->alive_p[i])
                {
                    pp_op[i] = save_stack[save_stack_index].savep_op[j];
                    pp_val[i] = save_stack[save_stack_index].savep_val[j];
                    j++;
                }
            }
        }
        // fprintf(listing,"s0: %c%d , s1 %c%d\n",reg_op[1],reg_val[1],reg_op[2],reg_val[2]);
        fprintf(listing, "l%d:\n", stmt->val[0]);
    }

    else if (stmt->kind == PARAM_E)
    {
        if (stmt->op[0] == 'n')
        {
            choose_reg(stmt, 20 + paramnum); //先保存那个寄存器的内容
            reg_change(20 + paramnum);
            fprintf(listing, "%c%d = %d\n", rc, ri, stmt->val[0]);
        }
        else
        {
            // pchoose(stmt->op[0]);
            // fprintf(listing,"1188: %c%d in %c%d\n", stmt->op[0],stmt->val[0], p_op[stmt->val[0]],
            //     p_val[stmt->val[0]]);

            getvar(stmt, stmt->op[0], stmt->val[0], 20 + paramnum);
        }
        reg_op[20 + paramnum] = 1; //锁住它，不让之后的任何操作的时候用这个寄存器了
        paramnum++;
    }

    else if (stmt->kind == CALL_E)
    {
        save_alive(stmt, NULL);//存活跃变量

        //函数调用之前，把不活跃却也在寄存器里的全局变量存起来
        for (int i = 1; i < 28; i++)
        {
            for (int j = 0; j < globalTid_index; j++)
            {
                //fprintf(listing,"1196:T%d  in %c%d\n",globalTid[j], Tp_op[globalTid[j]],Tp_val[globalTid[j]]);
                if (reg_op[i] == 'T' && reg_val[i] == globalTid[j] && globalkind[j] == 1) //是全局变量
                {
                    int reg1 = 19;
                    reg_change(reg1);
                    fprintf(listing, "loadaddr v%d %c%d \n", j, rc, ri);
                    fprintf(listing, "%c%d [0] = ", rc, ri);
                    reg_change(i);
                    fprintf(listing, "%c%d\n", rc, ri);
                    Tp_op[reg_val[i]] = 'g';
                    Tp_val[reg_val[i]] = j;
                }
            }
        }
        for (int i = 1; i < 28; i++)
            reg_op[i] = 0; //给所有寄存器清空，函数调用之后，认为所有的变量就算活跃，也会在栈里
        paramnum = 0;
        fprintf(listing, "call f_%s\n", stmt->funname);

        //清空此时存了左操作数的寄存器和栈，防止以后出问题
        for (int i = 1; i < 28; i++)
            if (reg_op[i] == stmt->op[0] && reg_val[i] == stmt->val[0])
                reg_op[i] = 0;

        for (int i = 0; i < maxalivenum; i++)
            if (stack_op[i] == stmt->op[0] && stack_val[i] == stmt->val[0])
                stack_op[i] = 0;

        reg_val[20] = stmt->val[0];
        reg_op[20] = stmt->op[0];
        pchoose(stmt->op[0]);
        p_op[stmt->val[0]] = 'r';
        p_val[stmt->val[0]] = 20;
    }

    //return 注意a0
    else if (stmt->kind == RETURN_E)
    {
        //函数返回之前，把不活跃却也在寄存器里的全局变量存起来
        for (int i = 1; i < 28; i++)
        {
            for (int j = 0; j < globalTid_index; j++)
            {
                if (reg_op[i] == 'T' && reg_val[i] == globalTid[j] && globalkind[j] == 1) //是全局变量
                {
                    int reg1 = 19;
                    reg_change(reg1);
                    fprintf(listing, "loadaddr v%d %c%d \n", j, rc, ri);
                    fprintf(listing, "%c%d [0] = ", rc, ri);
                    reg_change(i);
                    fprintf(listing, "%c%d\n", rc, ri);
                    Tp_op[reg_val[i]] = 'g';
                    Tp_val[reg_val[i]] = j;
                }
            }
        }
        // printf("get return\n");
        if (stmt->op[0] == 'n')
        {
            choose_reg(stmt, 20);
            reg_change(20);
            fprintf(listing, "%c%d = %d\n", rc, ri, stmt->val[0]);
            fprintf(listing, "return\n");
        }
        else
        {
            getvar(stmt, stmt->op[0], stmt->val[0], 20);
            fprintf(listing, "return\n");
        }
    }
    // else
    // {
    //     fprintf(listing, "debug: not yet\n");
    // }
}

int map[2000][2000]; //语句间的距离

void cal_s(Eestmt *stmt)
{
    if (stmt->kind == GOTO_E)
    {
        map[stmt->newlineno][stmt->next[0]->newlineno] = 1;
        map[stmt->newlineno][stmt->newlineno + 1] = 0;
    }
    if (stmt->kind == IF_GOTO_E)
    {
        map[stmt->newlineno][stmt->next[1]->newlineno] = 1;
    }
}

void forward_pass(Eestmt *stmt, int is_print, int operator) //前向遍历stmt的过程，中间可以加入打印，优化等功能
{
    Eestmt *last;
    while (stmt)
    {
        //从while 后面回来 或者是 if分支直接走过去的， 从前面进到while 开头的label 不切换到second
        if (stmt->prev[1] != NULL && last == stmt->prev[1])
        {
            stmt = popsecond();
            // fprintf(listing,"second branch\n");
            // fprintf(listing,"new block\n");
            if (operator== 0)
            {
                memset(Tval_op, 0, sizeof(Tval_op));
                memset(tval_op, 0, sizeof(tval_op));
                memset(pval_op, 0, sizeof(pval_op));
            }
        }

        //打印
        if (is_print == 2 && operator == 3)
        {
            fprintf(listing,"\n");
            print_alive(stmt);
        }
        else if (is_print == 1 &&operator == 3)
            print_stmt(stmt);
        if(is_print == 2 && operator == 3)
        {
            for(int i=1;i<28;i++)
            {
                if(reg_op[i]!=0)
                {
                    reg_change(i);
                    fprintf(listing,"%c%d:%c%d ",rc,ri,reg_op[i],reg_val[i]);
                }
            }
            fprintf(listing,"\n");
            for(int i=0;i<100;i++)
            {
                if(stack_op[i]!=0)
                {
                    fprintf(listing,"s%d:%c%d ",i,stack_op[i],stack_val[i]);
                }
            }
            fprintf(listing,"\n");
        }

        //复写传播优化过程
        if (operator== 0)
            copy_optimize(stmt);

        //第二遍要做的工作
        if (operator== 1)
        {
            alive_scan(stmt);
        }

        //第三遍，计算出距离
        if (operator== 2)
            cal_s(stmt);

        //第四遍，生成trigger代码
        if (operator== 3)
            gentrigger(stmt);

        //打印
        if (is_print == 2 && operator != 3)
            print_alive(stmt);
        else if (is_print == 1 && operator != 3)
            print_stmt(stmt);

        if (stmt->kind == LABEL_E) //进入新的block
        {
            // fprintf(listing,"new block\n");
            if (operator== 0)
            {
                memset(Tval_op, 0, sizeof(Tval_op));
                memset(tval_op, 0, sizeof(tval_op));
                memset(pval_op, 0, sizeof(pval_op));
            }
        }
        //没有else的if就不压入second了
        if (stmt->next[1] != NULL)
        {
            // if(stmt->next[1]->prev[1] == NULL)
            pushsecond(stmt->next[1]);
            // fprintf(listing,"first branch\n");
            // fprintf(listing,"new block\n");
            if (operator== 0)
            {
                memset(Tval_op, 0, sizeof(Tval_op));
                memset(tval_op, 0, sizeof(tval_op));
                memset(pval_op, 0, sizeof(pval_op));
            }
        }
        last = stmt;
        stmt = stmt->next[0];
    }
}

int var_alive(Eestmt *stmt)
{

    //数据流分析过程
    int haschanged = 0;
    int save_alive_T[2000];
    int save_alive_t[5000];
    int save_alive_p[10];
    for (int i = 0; i < Tid; i++)
        save_alive_T[i] = stmt->alive_T[i];
    for (int i = 0; i < tid; i++)
        save_alive_t[i] = stmt->alive_t[i];
    for (int i = 0; i < 10; i++)
        save_alive_p[i] = stmt->alive_p[i];

    //先得到后继的并
    if (stmt->kind != RETURN_E && stmt->next[0]) //return语句没有next,防止一下没return的语句
    {
        for (int i = 0; i < Tid; i++)
        {
            stmt->alive_T[i] = stmt->next[0]->alive_T[i];
            if (stmt->next[1])
            {
                stmt->alive_T[i] = stmt->alive_T[i] || stmt->next[1]->alive_T[i];
            }
        }
        for (int i = 0; i < tid; i++)
        {
            stmt->alive_t[i] = stmt->next[0]->alive_t[i];
            if (stmt->next[1])
            {
                stmt->alive_t[i] = stmt->alive_t[i] || stmt->next[1]->alive_t[i];
            }
        }
        for (int i = 0; i < 10; i++)
        {
            stmt->alive_p[i] = stmt->next[0]->alive_p[i];
            if (stmt->next[1])
            {
                stmt->alive_p[i] = stmt->alive_p[i] || stmt->next[1]->alive_p[i];
            }
        }
    }
    //kill 给左值赋值语句的左值的活性
    if (stmt->kind <= ASS_FROM_ARR_E || stmt->kind == CALL_E)
    {
        if (stmt->op[0] == 'T')
        {
            stmt->alive_T[stmt->val[0]] = 0;
        }
        if (stmt->op[0] == 't')
        {
            stmt->alive_t[stmt->val[0]] = 0;
        }
        if (stmt->op[0] == 'p')
        {
            stmt->alive_p[stmt->val[0]] = 0;
        }
    }

    if (stmt->kind == ARRAY_DEF_E) //局部数组定义
    {
        stmt->alive_T[stmt->val[1]] = 0;
    }

    //gen 右值语的活性
    if (stmt->kind <= IF_GOTO_E)
    {
        int starti = 1;
        if (stmt->kind >= ASS_TO_ARR_E) //从给数组赋值开始，第0个都是使用的变量
            starti = 0;
        for (int i = starti; i < 3; i++)
        {
            //局部T的数组会变成A，就不管了，p的数组和全局的T数组
            //还是要通过p+index 再 reg[0]来访问的，所以p数组和全局T数组还是有活性的
            if (stmt->op[i] == 'T')
            {
                stmt->alive_T[stmt->val[i]] = 1;
            }
            if (stmt->op[i] == 't')
            {
                stmt->alive_t[stmt->val[i]] = 1;
            }
            if (stmt->op[i] == 'p')
            {
                stmt->alive_p[stmt->val[i]] = 1;
            }
        }
    }
    stmt->alivecount = 0; // 统计活跃变量个数
    for (int i = 0; i < Tid; i++)
    {
        if (save_alive_T[i] != stmt->alive_T[i])
            haschanged = 1;
        if (save_alive_T[i])
            stmt->alivecount++;
    }
    for (int i = 0; i < tid; i++)
    {
        if (save_alive_t[i] != stmt->alive_t[i])
            haschanged = 1;
        if (save_alive_t[i])
            stmt->alivecount++;
    }
    for (int i = 0; i < 10; i++)
    {
        if (save_alive_p[i] != stmt->alive_p[i])
            haschanged = 1;
        if (save_alive_p[i])
            stmt->alivecount++;
    }
    //end 数据流分析过程
    return haschanged;
}

//删掉死代码，顺便处理定义局部数组
int dead_optimize(Eestmt *stmt, Eestmt **save)
{
    //call就算左边变量赋值完之后没活性，也不能删，因为函数调用有副作用
    if (stmt->kind <= ASS_FROM_ARR_E)
    {
        int Tkill = stmt->op[0] == 'T' && stmt->next[0]->alive_T[stmt->val[0]] == 0;
        int tkill = stmt->op[0] == 't' && stmt->next[0]->alive_t[stmt->val[0]] == 0;
        // fprintf(listing,"%c%d\n",stmt->op[0],stmt->val[0]);
        int pkill = stmt->op[0] == 'p' && stmt->next[0]->alive_p[stmt->val[0]] == 0;

        int isglobal = 0;
        for (int i = 0; i < globalTid_index; i++)
        {
            if (globalTid[i] == stmt->val[0] && stmt->op[0] == 'T')
            {
                if (globalkind[i] == 2)
                    ;//fprintf(listing, "debug: error in write to global address\n");
                else
                    isglobal = 1;
                break;
            }
        }

        if (isglobal == 0 && (Tkill || tkill || pkill))
        {
            //有两个next的只有 if goto语句
            stmt->prev[0]->next[0] = stmt->next[0];
            stmt->next[0]->prev[0] = stmt->prev[0];
            *save = stmt->prev[0]; // stmt不会是return，所以一定有next的
            free(stmt);
            return 1;
        }
    }
    //数组定义，之后如果不用的话，就删了
    if (stmt->kind == ARRAY_DEF_E)
    {
        if (stmt->next[0]->alive_T[stmt->val[1]] == 0)
        {
            stmt->prev[0]->next[0] = stmt->next[0];
            stmt->next[0]->prev[0] = stmt->prev[0];
            *save = stmt->prev[0]; // stmt不会是return，所以一定有next的
            free(stmt);
            return 1;
        }
    }

    return 0;
}

int backward_pass(Eestmt *stmt, int is_print, int operator, int *ret) //后向遍历stmt的过程，用于数据流分析
{
    // fprintf(listing,"start pass\n");
    int haschanged = 0;
    Eestmt *last = NULL;
    while (stmt)
    {
        //严重的bug: 要改成while，而不是if ，因为后向parse的时候，压入栈中的并不像前向的时候一样
        //一定是label，而有可能是另一句需要触发跳转的if goto，可能一次跳转会触发多次跳转。
        //两个while叠加的时候就会这样
        while (stmt->next[1] != NULL && last == stmt->next[0])
        {
            stmt = popsecond();
            last = stmt->next[0];
            // fprintf(listing,"second branch\n");
        }
        if (operator== 0)
        {
            int res = var_alive(stmt);
            if (res)
                haschanged = 1;
        }
        if (operator== 1)
        {
            int ret = dead_optimize(stmt, &stmt);
            if (ret)
                continue;
        }

        //打印
        if (is_print == 2)
            print_alive(stmt);
        else if (is_print == 1)
            print_stmt(stmt);

        if (stmt->prev[1] != NULL)
        {
            pushsecond(stmt->prev[0]);
            // fprintf(listing,"first branch\n");
            last = stmt;
            stmt = stmt->prev[1];
        }
        else
        {
            last = stmt;
            stmt = stmt->prev[0];
        }
    }
    if (ret)
        *ret = haschanged;
}

void dataflow(Eeglobal *tree)
{
    for (int i = 0; i < tree->return_num; i++)
    {
        // for (int j = 0; j < globalTid_index; j++)
        // {
        //     if (globalkind[j] == 1)
        //         tree->return_stmt[i]->alive_T[globalTid[j]] = 1;
        // }
        if (tree->return_stmt[i]->op[0] == 'T')
            tree->return_stmt[i]->alive_T[tree->return_stmt[i]->val[0]] = 1;
        else if (tree->return_stmt[i]->op[0] == 't')
            tree->return_stmt[i]->alive_t[tree->return_stmt[i]->val[0]] = 1;
        else if (tree->return_stmt[i]->op[0] == 'p')
            tree->return_stmt[i]->alive_p[tree->return_stmt[i]->val[0]] = 1;
    }
    int res;
    backward_pass(tree->end, 0, 0, &res);
    while (res)
    {
        backward_pass(tree->end, 0, 0, &res);
    }
}

int startp;
void dfs(int n, int depth)
{
    // printf("n = %d , d = %d\n",n,depth);
    for (int i = 1; i < newlineno; i++)
    {
        if (map[n][i])
        {
            if ((thes[startp][i] == -1) || thes[startp][i] > depth)
            {
                thes[startp][i] = depth;
                dfs(i, depth + 1);
            }
        }
    }
}

//优化并生成trigger代码
//还可以尝试一下窥孔优化，把eeyore中的t1 = n1 + n0 这种和常数加法优化了
//以及由于基本快不同导致的t1 = n1 + T1  T1= t1 这样的给优化了(T1在别的基本块里还会用，所以此处会活跃)
// 可用表达式和可达定值可以试着做一下


void start_optimize(Eeglobal *tree)
{
    while (tree)
    {
        if (tree->kind == 0)
        {
            // fprintf(listing,"new block\n");
            memset(Tval_op, 0, sizeof(Tval_op));
            memset(tval_op, 0, sizeof(tval_op));
            memset(pval_op, 0, sizeof(pval_op));
            forward_pass(tree->start->next[0], 0, 0); //从START_stmt的下一个语句开始,复写传播优化
            dataflow(tree);
            backward_pass(tree->end, 0, 1, NULL); //根据活分析的结果删掉死代码,要后向地做
            for (int i = 0; i < Tid; i++)
            {
                Tuse[i] = (cn *)malloc(sizeof(cn));
                Tuse[i]->next = NULL;
            }
            for (int i = 0; i < tid; i++)
            {
                tuse[i] = (cn *)malloc(sizeof(cn));
                tuse[i]->next = NULL;
            }
            for (int i = 0; i < 10; i++)
            {
                puse[i] = (cn *)malloc(sizeof(cn));
                puse[i]->next = NULL;
            }

            //再扫一遍，得到活性区间
            maxalivenum = 0; //初始化
            array_index = 0;
            newlineno = 1;
            forward_pass(tree->start->next[0], 0, 1);

            //计算出语句的距离
            memset(map, 0, sizeof(map));
            for (int i = 1; i < newlineno - 1; i++)
                map[i][i + 1] = 1;
            forward_pass(tree->start->next[0], 0, 2);

            // fprintf(listing,"map\n");
            // for(int i=1;i<newlineno;i++)
            // {
            //     for(int j=1;j<newlineno;j++)
            //     {
            //         fprintf(listing,"%d ",map[i][j]);
            //     }
            //     fprintf(listing,"\n");
            // }
            // fprintf(listing,"\n");

            memset(thes, -1, sizeof(thes));
            for (int i = 1; i < newlineno; i++)
            {
                startp = i;
                dfs(i, 1); //把语句的距离求出来
            }

            // for (int i = 0; i < Tid; i++)
            // {
            //     cn* tmp = Tdef[i]->next;
            //     while(tmp)
            //     {
            //         fprintf(listing,"T%d: def at %d\n",i,tmp->val);
            //         tmp = tmp->next;
            //     }
            //     tmp = Tuse[i]->next;
            //     while(tmp)
            //     {
            //         fprintf(listing,"T%d: use at %d\n",i,tmp->val);
            //         tmp = tmp->next;
            //     }
            // }
            // for (int i = 0; i < tid; i++)
            // {
            //     cn* tmp = tdef[i]->next;
            //     while(tmp)
            //     {
            //         fprintf(listing,"t%d: def at %d\n",i,tmp->val);
            //         tmp = tmp->next;
            //     }
            //     tmp = tuse[i]->next;
            //     while(tmp)
            //     {
            //         fprintf(listing,"t%d: use at %d\n",i,tmp->val);
            //         tmp = tmp->next;
            //     }
            // }
            // for (int i = 0; i < 10; i++)
            // {
            //     cn* tmp = pdef[i]->next;
            //     while(tmp)
            //     {
            //         fprintf(listing,"p%d: def at %d\n",i,tmp->val);
            //         tmp = tmp->next;
            //     }
            //     tmp = puse[i]->next;
            //     while(tmp)
            //     {
            //         fprintf(listing,"p%d: use at %d\n",i,tmp->val);
            //         tmp = tmp->next;
            //     }
            // }

            // fprintf(listing,"thes:\n");
            // for(int i=1;i<newlineno;i++)
            // {
            //     for(int j=1;j<newlineno;j++)
            //     {
            //         fprintf(listing,"%d   ",thes[i][j]);
            //     }
            //     fprintf(listing,"\n");
            // }
            // fprintf(listing,"\n");

            // fprintf(listing, "\n\n");
            int stacknum = 2 * maxalivenum;
            for (int i = 0; i < array_index; i++)
                stacknum += arraysize[i];
            fprintf(listing, "f_%s [%d] [%d]\n", tree->funname, tree->funnum, stacknum);
            for (int i = 1; i <= 27; i++)//初始化
            {
                reg_op[i] = 0;
            }
            for(int i = 0;i<Tid || i< tid || i<10;i++)//初始化
            {
                Tp_op[i] = 0;
                tp_op[i] = 0;
                pp_op[i] = 0;
            }
            for(int i = 0;i<globalTid_index;i++)//初始化全局变量的位置
            {
                Tp_op[globalTid[i]] = 'g';
                Tp_val[globalTid[i]] = i;
            }

            for (int i = 0; i < tree->funnum; i++)//初始化函数参数的位置
            {
                pp_val[i] = 20 + i;
                pp_op[i] = 'r';
                reg_op[20 + i] = 'p';
                reg_val[20 + i] = i;
            }
            for (int i = 0; i < 1000; i++)
                stack_op[i] = 0;
            forward_pass(tree->start->next[0], 0,3); //生成trigger代码
            fprintf(listing, "end f_%s \n", tree->funname);
        }
        else if (tree->kind == 1)
        {
            globalTid[globalTid_index] = tree->varid;
            globalkind[globalTid_index++] = 1;
            Tp_val[tree->varid] = vid;
            Tp_op[tree->varid] = 'g';
            fprintf(listing, "v%d = 0 \n", vid++);
        }
        else if (tree->kind == 2)
        {
            Tp_val[tree->varid] = vid;
            Tp_op[tree->varid] = 'g';
            fprintf(listing, "v%d = malloc %d \n", vid++, tree->arraynum);
            globalTid[globalTid_index] = tree->varid;
            globalkind[globalTid_index++] = 2;
        }
        tree = tree->next;
    }
}
