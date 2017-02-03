/*
    File name: monolis.c
    ichise masashi
    2017.02.02
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* 関数一覧 */
void initcell (void);
int makesym(char *name);
void assocsym(int sym, int val);
int cons(int car, int cdr);
void gettoken(void);
int numbertoken(char buf[]);
int symboltoken(char buf[]);
int issymch(char c);

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


typedef enum toktype {LPAREN,RPAREN,QUOTE,DOT,NUMBER,SYMBOL,OTHER} toktype;
typedef enum backtrack {GO,BACK} backtrack;
#define BUFFSIZE 256

struct token {
	char ch;
	backtrack flag;
	toktype type;
	char buf[BUFFSIZE];
} stok;

#define EOL     '\n'
#define TAB     '\t'
#define SPACE   ' '
#define NUL     '\0'

void gettoken (void) {
	char c;
	int pos;

	if(stok.flag == BACK) {
		stok.flag = GO;
		return;
	}
	if (stok.ch == ')') {
		stok.type = RPAREN;
		stok.ch = NIL;
		return;
	}
	if (stok.ch == '(') {
		stok.type = LPAREN;
		stok.ch = NIL;
		return;
	}
	c = getchar();
	while ((c == SPACE) || (c == EOL) || (c == TAB)) {
		c = getchar();
	}
	switch(c) {
		case '(': stok.type = LPAREN; break;
		case ')': stok.type = RPAREN; break;
		case '\'': stok.type = QUOTE; break;
		case '.': stok.type = DOT; break;
		default: 
		    pos = 0;
		    stok.buf[pos++] = c;
		    while ((c=getchar()!=EOL) &&
		    	   (pos < BUFFSIZE) &&
		    	   (c != SPACE) &&
		    	   (c != '(') &&
		    	   (c != ')')) {
		    	stok.buf[pos++] = c;
		    }
		    if (numbertoken(stok.buf)) {
		    	stok.type = NUMBER;
		    	break;
		    }
		    if (symboltoken(stok.buf)) {
		    	stok.type = SYMBOL;
		    	break;
		    }
		    stok.type = OTHER;
	}
}
int numbertoken(char buf[]){
    int i;
    char c;
    
    if((buf[0] == '+') || (buf[0] == '-')){
        if(buf[1] == NUL)
            return(0); // case {+,-} => symbol
        i = 1;
        while((c=buf[i]) != NUL)
            if(isdigit(c))
                i++;  // case {+123..., -123...}
            else
                return(0); 
    }
    else {       
        i = 0;    // {1234...}
        while((c=buf[i]) != NUL)
            if(isdigit(c))
                i++;
            else 
                return(0);
    }
    return(1);
}

int symboltoken(char buf[]){
    int i;
    char c;
    
    if(isdigit(buf[0]))
        return(0);
    
    i = 0;
    while((c=buf[i]) != NUL)
        if((isalpha(c)) || (isdigit(c)) || (issymch(c)))
            i++;
        else 
            return(0);
    
    return(1);
}

int issymch(char c){
    switch(c){
        case '!':
        case '?':
        case '+':
        case '-':
        case '*':
        case '/':
        case '=':
        case '<':
        case '>': return(1);
        default:  return(0);
    }
}  














