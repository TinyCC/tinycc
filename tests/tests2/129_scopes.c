#include <stdio.h>
enum{ in = 0};
#define myassert(X) do{ if(!X) printf("%d: assertion failed\n", __LINE__); }while(0)
int main(){
    {
        myassert(!in);
        if(sizeof(enum{in=1})) myassert(in);
        myassert(!in); //OOPS
    }
    {
        myassert(!in);
        switch(sizeof(enum{in=1})) { default: myassert(in); }
        myassert(!in); //OOPS
    }
    {
        myassert(!in);
        while(sizeof(enum{in=1})) { myassert(in); break; }
        myassert(!in); //OOPS
    }
    {
        myassert(!in);
        do{ myassert(!in);}while(0*sizeof(enum{in=1}));
        myassert(!in); //OOPS
    }

    {
        myassert(!in);
        for(sizeof(enum{in=1});;){ myassert(in); break; }
        myassert(!in); //OK
    }
    {
        myassert(!in);
        for(;;sizeof(enum{in=1})){ myassert(in); break; }
        myassert(!in); //OK
    }
    {
        myassert(!in);
        for(;sizeof(enum{in=1});){ myassert(in); break; }
        myassert(!in); //OK
    }

}

