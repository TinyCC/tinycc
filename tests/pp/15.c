#define Y(x) Z(x)
#define X Y
X(1)
X(X(1))
X(X(X(X(X(1)))))

#define A B
#define B A
return A + B;

#undef A
#undef B

#define A B+1
#define B A
return A + B;

#define A1 B1+1
#define B1 C1+2
#define C1 A1+3
return A1 + B1;

#define i() x
#define n() 1
i()i()n()n()i()
i()+i()-n()+n()-
