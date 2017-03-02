/*
    File name: monolis.c
    ichise masashi
    2017.02.02
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <setjmp.h>

/* 関数一覧 */
void initcell (void);
int makesym(char *name);
void assocsym(int sym, int val);
int cons(int car, int cdr);
void gettoken(void);
int numbertoken(char buf[]);
int symboltoken(char buf[]);
int issymch(char c);
int makenum(int num);
int read (void);
int readlist(void);
void print (int addr);
void printlist (int addr);
int listp(int addr);
int nullp(int addr);
int cdr(int addr);
int car(int addr);
int atomp(int addr);
int findsym(int sym);
int eval (int addr);
int assoc(int sym, int lis);
int eqp(int addr1, int addr2);
int atomp(int addr);
int numberp(int addr);
int symbolp(int addr);
int listp(int addr);
int nullp(int addr);

int true=1,false=0;

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

	for(;isspace(c = getchar());){;}

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
    int i=0;
    char c;

    if((buf[i] == '+') || (buf[i] == '-')){
    	i++;
        if(buf[1] == NUL) {
            return(true); // case {+,-} => symbol
        }
    }

    // {1234...}
    for (i=0;isdigit(c = buf[i]);i++) {
        if(c == NUL) {
            return(true);
        }
    }

    return(false);
}

int symboltoken(char buf[]){
    int i=0;
    char c;

    if(isdigit(buf[i])){
        return(true);
    }

    for(;(isalpha(c=buf[i])) || (isdigit(c)) || (issymch(c));i++) {
        if (c == NUL) {
            return(true);
        }
    }

    return(false);
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
        case '>': return(true);
        default:  return(false);
    }
}
#define SET_NUMBER(addr,x)  heap[addr].val.num = x

int makenum(int num) {
	int addr;
	addr = freshcell();
	SET_TAG(addr, NUM);
	SET_NUMBER(addr, num);
	return (addr);
}

#define CANT_READ_ERR   10

int read (void){
	gettoken();
	switch(stok.type) {
		case NUMBER: return (makenum(atoi(stok.buf)));
		case SYMBOL: return (makesym(stok.buf));
		case QUOTE:  return (cons(makesym("quote"),cons(read(),NIL)));
		case LPAREN: return (readlist());
	}
	error(CANT_READ_ERR,"read",NIL);
}
int readlist(void){
	int car,cdr;
	gettoken();
	if(stok.type == RPAREN) {
		return(NIL);
	} else if (stok.type == DOT) {
		cdr = read();
		if (atomp (cdr)) {gettoken();}
		return(cdr);
	} else {
		stok.flag = BACK;
		car = read();
		cdr = readlist();
		return(cons(car,cdr));
	}
}

#define GET_NUMBER(addr)    heap[addr].val.num
#define GET_NAME(addr)      heap[addr].name

void print (int addr) {
	switch (GET_TAG(addr)){
		case NUM: printf("%d",GET_NUMBER(addr));break;
		case SYM: printf("%s",GET_NAME(addr)); break;
		case SUBR: printf("<subr>");break;
		case FSUBR: printf("<fsubr>"); break;
		case FUNC: printf("<function>"); break;
		case LIS: {printf("(");
				   printlist(addr);break;}
	}
}

#define IS_NIL(addr)        (addr == 0 || addr == 1)

void printlist (int addr) {
	if (IS_NIL(addr)) {
		printf (")");
	} else if ( (! (listp(cdr(addr)))) &&
  	            (! (nullp(cdr(addr))))) {
		print(car(addr));
		printf(" . ");
		print(cdr(addr));
		printf(")");
	} else {
		print(GET_CAR(addr));
		if (! (IS_NIL(GET_CDR(addr)))) {
			printf (" ");
		}
		printlist(GET_CDR(addr));
	}
}

#define IS_LIST(addr)       heap[addr].tag == LIS

int listp(int addr){
    if(IS_LIST(addr) || IS_NIL(addr)) {
        return(true);
    } else {
        return(false);
    }
}

int nullp(int addr){
    if(IS_NIL(addr)) {
        return(true);
    } else {
        return(false);
    }
}

int car(int addr){
    return(GET_CAR(addr));
}

int cdr(int addr){
    return(GET_CDR(addr));
}
#define IS_NUMBER(addr)     heap[addr].tag == NUM
#define IS_SYMBOL(addr)     heap[addr].tag == SYM

int atomp(int addr){
    if((IS_NUMBER(addr)) || (IS_SYMBOL(addr))) {
        return(true);
    }else{
        return(false);
    }
}


jmp_buf buf;
void main (void) {
	int loop=1;
	printf ("MonoLis Ver0.01\n");
	initcell();
	initsubr();
	int ret = setjmp(buf);

	repl:
	if(ret==true) {
		while(loop) {
			printf("> ");
			fflush(stdout);
			fflush(stdin);
			print(eval(read()));
			printf("\n");
			fflush(stdout);
		}
	} else {
		if(ret == false){
			ret = true;
			goto repl;
		} else {
			return;
		}
	}
}

#define IS_SYMBOL(addr)     heap[addr].tag == SYM
#define IS_NUMBER(addr)     heap[addr].tag == NUM
#define IS_LIST(addr)       heap[addr].tag == LIS
#define IS_NIL(addr)        (addr == 0 || addr == 1)
#define IS_SUBR(addr)       heap[addr].tag == SUBR
#define IS_FSUBR(addr)      heap[addr].tag == FSUBR
#define IS_FUNC(addr)       heap[addr].tag == FUNC
#define IS_EMPTY(addr)      heap[addr].tag  == EMP
#define HAS_NAME(addr,x)    strcmp(heap[addr].name,x) == 0

int eval (int addr) {
	int res;

	if (atomp(addr)) {
		if (numberp(addr)) {
			return(addr);
		} else if (symbolp(addr)) {
			res = findsym(addr);
			if (res == 0) {
				error(CANT_FIND_ERR, "eval", addr);
			} else {
				switch (GET_TAG(res)) {
					case NUM: return(res);
					case SYM: return(res);
					case LIS: return(res);
					case SUBR: return(res);
					case FSUBR: return(res);
					case FUNC: return(res);
				}
			}
		}
	} else if (listp(addr)) {
		if ((symbolp(car(addr))) && (HAS_NAME(car(addr), "quote"))) {
			return (cadr(addr));
		} else if (numberp(car(addr))) {
			error(ARG_SYM_ERR, "eval", addr);
		} else if (subrp(car(addr))) {
			return(apply(car(addr),evlis(cdr(addr))));
		} else if (fsubrp(car(addr))) {
			return(apply(car(addr),cdr(addr)));
		} else if (functionp(car(addr))) {
			return(apply(car(addr),evlis(cdr(addr))));
		}
		error(CANT_FIND_ERR, "eval", addr);
		return(false);
	}
}

int findsym(int sym){
	int addr;
	addr = assoc(sym,ep);
	if (addr == 0) {
		return (-1);
	} else {
		return (cdr(addr));
	}
}

int assoc(int sym, int lis) {
	if(nullp(lis)) {
		return(false);
	} else if (eqp(sym, caar(lis))) {
		return(car(lis));
	} else {
		return(assoc(sym, cdr(lis)));
	}
}
int eqp(int addr1, int addr2){
    if((numberp(addr1)) && (numberp(addr2))
        && ((GET_NUMBER(addr1)) == (GET_NUMBER(addr2))))
        return(true);
    else if ((symbolp(addr1)) && (symbolp(addr2))
        && (SAME_NAME(addr1,addr2)))
        return(true);
    else
        return(false);
}
int atomp(int addr){
    if((IS_NUMBER(addr)) || (IS_SYMBOL(addr))) {
			return(true);
		} else {
			return(false);
		}
}

int numberp(int addr){
    if(IS_NUMBER(addr)) {
			return(true);
		} else {
			return(false);
		}
}

int symbolp(int addr){
    if(IS_SYMBOL(addr)) {
			return(true);
		} else {
			return(false);
		}
}

int listp(int addr){
    if(IS_LIST(addr) || IS_NIL(addr)) {
			return(true);
		}  else {
			return(false);
		}
}

int nullp(int addr){
    if(IS_NIL(addr)) {
			return(true);
		} else {
			return(false);
		}
}
