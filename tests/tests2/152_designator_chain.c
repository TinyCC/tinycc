#include <stdio.h>

struct S { struct { int a, b; } x[3]; };

struct S g[] = { [1].x[0].b = 5, 6, 7 };
struct S h[] = { [0].x[2].b = 10, 11 };
struct S k[] = { [1].x[0].b = 5, 6, [0].x[1].a = 1, 2, 3 };

static void show(const char *n, struct S *s, int cnt)
{
    int i, j;
    printf("%s %d:", n, cnt);
    for (i = 0; i < cnt; i++)
        for (j = 0; j < 3; j++)
            printf(" %d,%d", s[i].x[j].a, s[i].x[j].b);
    printf("\n");
}

int main(void)
{
    struct S y[] = { [1].x[0].b = 5, 6, 7 };
    struct S z[] = { [0].x[2].b = 10, 11 };
    struct S w[] = { [1].x[0].b = 5, 6, [0].x[1].a = 1, 2, 3 };
    struct S v[] = { [0].x[0].a = 1, [0].x[1].b = 2, [0].x[0].a = 0 };
    int m[2][2][2] = { [1][0][1] = 1, 2, 3 };
    struct { int a; struct { int b, c; } s; int d; } t = { .s.c = 4, 5 };

    show("g", g, sizeof g / sizeof *g);
    show("h", h, sizeof h / sizeof *h);
    show("k", k, sizeof k / sizeof *k);
    show("y", y, sizeof y / sizeof *y);
    show("z", z, sizeof z / sizeof *z);
    show("w", w, sizeof w / sizeof *w);
    show("v", v, sizeof v / sizeof *v);
    printf("m: %d %d %d %d %d %d %d %d\n", m[0][0][0], m[0][0][1], m[0][1][0],
           m[0][1][1], m[1][0][0], m[1][0][1], m[1][1][0], m[1][1][1]);
    printf("t: %d %d %d %d\n", t.a, t.s.b, t.s.c, t.d);
    return 0;
}
