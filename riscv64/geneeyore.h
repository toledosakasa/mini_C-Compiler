#ifndef __GENEEYORE_H__
#define __GENEEYORE_H__


typedef enum {PLUS_E,MINUS_E,TIMES_E,OVER_E,MOD_E,AND_E,OR_E,EQ_E,NE_E,LT_E,GT_E,NOT_E,NEG_E,
                ASSIGN_E,ASS_FROM_ARR_E,ASS_TO_ARR_E,RETURN_E,PARAM_E,
                IF_GOTO_E,GOTO_E,CALL_E,START_E,LABEL_E,ARRAY_DEF_E,SLTI_E} stmtkind;

typedef struct eestmt
    { 
        struct eestmt * next[2];
        struct eestmt * prev[2];
        int lineno;
        int newlineno;
        stmtkind kind;
        int val[3];
        char op[3];
        char * funname;
        int alive_T[2000];
        int alive_t[5000];
        int alive_p[10];
        int alivecount;

    } Eestmt;

Eestmt* now;

typedef struct eeglobal
    { 
        struct eeglobal * next;
        Eestmt * start;
        Eestmt * end;
        Eestmt** return_stmt;
        int return_num;
        stmtkind kind; //0 为fun ，1为var 2 为array
        char * funname;
        int funnum;
        int varid;
        int arraynum;
    } Eeglobal;

Eeglobal * globalstart;
Eeglobal * globalnow;

extern int Tid;
extern int tid;

void geneeyore(TreeNode *tree);

 
#endif