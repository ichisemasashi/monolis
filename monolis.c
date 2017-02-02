/*
    File name: monolis.c
    ichise masashi
    2017.02.02
*/
#include <stdlib.h>
#include <string.h>

/* 関数一覧 */
void initcell (void);
int makesym(char *name);
void assocsym(int sym, int val);
int cons(int car, int cdr);

/* セル構造 */
typedef enum tag {EMP,NUM,SYM,LIS,SUBR,FSUBR,FUNC} tag;
typedef enum flag {FRE,USE} flag;

struct cell {
	tag tag; /* 種類 */
	flag flag; /* GCのとき利用 */
	char *name; /* 文字列、シンボル名 */
	union {
		int num; /* 数 */
		int bind; /* 他の番地のセルを参照する場合の番地 */
		int (*subr)(); /* 関数ポインタ */
	} val;
    int car;
    int cdr;
};
typedef struct cell cell;

/* 初期化して自由リストを生成 */
int hp,fc,ep,sp,ap;
#define HEAPSIZE 32768
#define SYMSIZE 256
#define NIL 0
cell heap[HEAPSIZE];
void initcell (void) {
	int i;
	for (i = 0;i <= HEAPSIZE; i++) {
		heap[i].flag = FRE;
		heap[i].cdr = i + 1;
	}
	hp = 0; /* 自由リストの先頭番地 */
	fc = HEAPSIZE;

	/* 0番地はnil, 環境レジスタを設定。初期環境 */
	ep = makesym("nil"); /* 環境リストの先頭番地 */
	assocsym(makesym("nil"),NIL);
	assocsym(makesym("t"),makesym("t"));
	
	sp = 0; /* スタックの先頭番地 */
	ap = 0; /* 引数リストの先頭番地. GCで利用する。 */
}

#define GET_CAR(addr) heap[addr].car
#define GET_CDR(addr) heap[addr].cdr
#define SET_CAR(addr,x)     heap[addr].car = x
#define SET_CDR(addr,x)     heap[addr].cdr = x
#define SET_TAG(addr,x)     heap[addr].tag = x
#define SET_NAME(addr,x)    heap[addr].name = (char *)malloc(SYMSIZE); strcpy(heap[addr].name,x);

int freshcell(void){
    int res;
    
    res = hp;
    hp = heap[hp].cdr;
    SET_CDR(res,0);
    fc--;
    return(res);
}

int makesym(char *name){
    int addr;
    
    addr = freshcell();
    SET_TAG(addr,SYM);
    SET_NAME(addr,name);
    return(addr);
}

void assocsym(int sym, int val){
    ep = cons(cons(sym,val),ep);
}

int cons(int car, int cdr){
    int addr;
    
    addr = freshcell();
    SET_TAG(addr,LIS);
    SET_CAR(addr,car);
    SET_CDR(addr,cdr);
    return(addr);
}
