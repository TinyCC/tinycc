#include<stdio.h>

struct large1 {
    int a[768];
};

struct large2 {
    int a[1920];
};

void pass_large_struct1(struct large1 in)
{
    printf("%d %d\n", in.a[200], in.a[767]);
    return;
}

void pass_large_struct2(struct large2 in)
{
    printf("%d %d %d\n", in.a[200], in.a[1023], in.a[1919]);
    return;
}

void pass_many_args(int a, int b, int c, int d, int e, int f, int g, int h, int i,
                    int j, int k, int l, int m)
{
    printf("%d %d %d %d %d %d %d %d %d %d %d %d %d\n", a, b, c, d, e, f, g, h, i,
           j, k, l, m);
    return;
}

struct large1 l1 = { .a = { [200] = 1, [767] = 2 } };
struct large2 l2 = { .a = { [200] = 3, [1023] = 4, [1919] = 5} };

int main(void)
{
    pass_large_struct1(l1);
    pass_large_struct2(l2);
    pass_many_args(13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1);

    return 0;
}
