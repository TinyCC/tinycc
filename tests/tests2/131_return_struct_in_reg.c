#include<stdio.h>

#define DATA 0x1234567890abcdll, 0x5a5aa5a5f0f00f0fll

struct s {
	long long int a;
	long long int b;
};

struct {
	struct s d;
} g = { { DATA } }, *gp = &g;

struct s
foo1(void)
{
	struct s d = { DATA };
	struct s *p = &d;
	long long int x = 0;
	return *p;
}

struct s
foo2(void)
{
	long long int unused = 0;
	return gp->d;
}

struct s
foo3(void)
{
	struct s d = { DATA };
	long long int unused = 0;
	return d;
}

int
main(void)
{
	struct s d;
	d = foo1();
	printf("%llx %llx\n", d.a, d.b);
	d = foo2();
	printf("%llx %llx\n", d.a, d.b);
	d = foo3();
	printf("%llx %llx\n", d.a, d.b);
	return 0;
}
