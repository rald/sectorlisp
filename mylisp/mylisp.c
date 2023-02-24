#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>



#define kT          4
#define kQuote      6
#define kCond       12
#define kRead       17
#define kPrint      22
#define kAtom       28
#define kCar        33
#define kCdr        37
#define kCons       41
#define kEq         46

#define M (RAM + sizeof(RAM) / sizeof(RAM[0]) / 2)
#define S "NIL\0T\0QUOTE\0COND\0READ\0PRINT\0ATOM\0CAR\0CDR\0CONS\0EQ"

int cx; /* stores negative memory use */
int dx; /* stores lookahead character */
int RAM[0100000]; /* your own ibm7090 */

char *trim(char *a);
char *getstr(const char *prompt);

int Intern(void);
int GetChar(void);
void PrintChar(int b);
int GetToken(void);
int AddList(int x);
int GetList(void);
int GetObject(int c);
int Read(void);
void PrintAtom(int x);
void PrintList(int x);
void PrintObject(int x);
void Print(int e);
void PrintNewLine();

int Car(int x);
int Cdr(int x);
int Cons(int car, int cdr);
int Gc(int x, int m, int k);
int Evlis(int m, int a);
int Pairlis(int x, int y, int a);
int Assoc(int x, int y);
int Evcon(int c, int a);
int Apply(int f, int x, int a);
int Eval(int e, int a);



char *trim(char *a) {
    char *p=a,*q=a;
    while(isspace(*q)) ++q;
    while(*q) *p++=*q++;
    *p='\0';
    while(p>a && isspace(*--p)) *p='\0';
    return a;
}

#define STRING_MAX 65536
char *getstr(const char *prompt) {
	char *line = NULL;
	int i=0, c=0;
	printf("%s",prompt);
	if(!feof(stdin)) {
		line=malloc(sizeof(*line)*STRING_MAX);
		line[0]='\0';
		while((c=fgetc(stdin))!='\n' && c!=EOF) {
			line[i++]=c;
			if(i>=STRING_MAX) {
				errno=EMSGSIZE;
				free(line);
				line=NULL;
				return NULL;
			}
		}
		line[i]='\0';
		trim(line);
//		printf("%s\n",line);
	}
	return line;
}



int Intern(void) {
  int i, j, x;
  for (i = 0; (x = M[i++]);) {
    for (j = 0;; ++j) {
      if (x != RAM[j]) break;
      if (!x) return i - j - 1;
      x = M[i++];
    }
    while (x)
      x = M[i++];
  }
  j = 0;
  x = --i;
  while ((M[i++] = RAM[j++]));
  return x;
}

int GetChar(void) {
  int c, t;
  static char *l, *p;
  if (l || (l = p = getstr("* "))) {
    if (*p) {
      c = *p++ & 255;
    } else {
      free(l);
      l = p = 0;
      c = '\n';
    }
    t = dx;
    dx = c;
    return t;
  } else {
    PrintChar('\n');
    exit(0);
  }
}

void PrintChar(int b) {
  fputc(b, stdout);
}

int GetToken(void) {
  int c, i = 0;
  do if ((c = GetChar()) > ' ') RAM[i++] = c;
  while (c <= ' ' || (c > ')' && dx > ')'));
  RAM[i] = 0;
  return c;
}

int AddList(int x) {
  return Cons(x, GetList());
}

int GetList(void) {
  int c = GetToken();
  if (c == ')') return 0;
  return AddList(GetObject(c));
}

int GetObject(int c) {
  if (c == '(') return GetList();
  return Intern();
}

int Read(void) {
  return GetObject(GetToken());
}

void PrintAtom(int x) {
  int c;
  for (;;) {
    if (!(c = M[x++])) break;
    PrintChar(c);
  }
}

void PrintList(int x) {
  PrintChar('(');
  PrintObject(Car(x));
  while ((x = Cdr(x))) {
    if (x < 0) {
      PrintChar(' ');
      PrintObject(Car(x));
    } else {
      PrintChar('.');
      PrintObject(x);
      break;
    }
  }
  PrintChar(')');
}

void PrintObject(int x) {
  if (x < 0) {
    PrintList(x);
  } else {
    PrintAtom(x);
  }
}

void Print(int e) {
  PrintObject(e);
}

void PrintNewLine() {
  PrintChar('\n');
}






int Car(int x) {
  return M[x];
}

int Cdr(int x) {
  return M[x + 1];
}

int Cons(int car, int cdr) {
  M[--cx] = cdr;
  M[--cx] = car;
  return cx;
}

int Gc(int x, int m, int k) {
  return x < m ? Cons(Gc(Car(x), m, k), 
                      Gc(Cdr(x), m, k)) + k : x;
}

int Evlis(int m, int a) {
  if (m) {
    int x = Eval(Car(m), a);
    return Cons(x, Evlis(Cdr(m), a));
  } else {
    return 0;
  }
}

int Pairlis(int x, int y, int a) {
  return x ? Cons(Cons(Car(x), Car(y)),
                  Pairlis(Cdr(x), Cdr(y), a)) : a;
}

int Assoc(int x, int y) {
  if (!y) return 0;
  if (x == Car(Car(y))) return Cdr(Car(y));
  return Assoc(x, Cdr(y));
}

int Evcon(int c, int a) {
  if (Eval(Car(Car(c)), a)) {
    return Eval(Car(Cdr(Car(c))), a);
  } else {
    return Evcon(Cdr(c), a);
  }
}

int Apply(int f, int x, int a) {
  if (f < 0)       return Eval(Car(Cdr(Cdr(f))), Pairlis(Car(Cdr(f)), x, a));
  if (f > kEq)     return Apply(Eval(f, a), x, a);
  if (f == kEq)    return Car(x) == Car(Cdr(x)) ? kT : 0;
  if (f == kCons)  return Cons(Car(x), Car(Cdr(x)));
  if (f == kAtom)  return Car(x) < 0 ? 0 : kT;
  if (f == kCar)   return Car(Car(x));
  if (f == kCdr)   return Cdr(Car(x));
  if (f == kRead)  return Read();
  if (f == kPrint) return (x ? Print(Car(x)) : PrintNewLine()), 0;
  return 0;
}

int Eval(int e, int a) {
  int A, B, C;
  if (e >= 0)
    return Assoc(e, a);
  if (Car(e) == kQuote)
    return Car(Cdr(e));
  A = cx;
  if (Car(e) == kCond) {
    e = Evcon(Cdr(e), a);
  } else {
    e = Apply(Car(e), Evlis(Cdr(e), a), a);
  }
  B = cx;
  e = Gc(e, A, A - B);
  C = cx;
  while (C < B)
    M[--A] = M[--B];
  cx = A;
  return e;
}



int main() {
  size_t i;
  for(i = 0; i < sizeof(S); ++i) M[i] = S[i];
  for (;;) {
    cx = 0;
    Print(Eval(Read(), 0));
    PrintNewLine();
  }
  return 0;
}
