#include <stdio.h>

const int a = 0;

struct a {
	int a;
};

struct b {
	int a;
};

int a_f()
{
	return 20;
}

int b_f()
{
	return 10;
}

#define gen_sw(a) _Generic(a, const char *: 1, default: 8, int: 123);

int main()
{
	int i = 0;
	struct b titi;
	const int * const ptr;
	const char *ti;

	i = _Generic(a, int: a_f, const int: b_f)();
	printf("%d\n", i);
	i = _Generic(ptr, int *:1, int * const:2, default:20);
	printf("%d\n", i);
	i = gen_sw(a);
	printf("%d\n", i);
	i = _Generic(titi, struct a:1, struct b:2, default:20);
	printf("%d\n", i);
	i = _Generic(a, char:1, int[4]:2, default:5);
	printf("%d\n", i);
	i = _Generic(17, int :1, int **:2);
	printf("%d\n", i);
	i = _Generic(17L, int :1, long :2);
	printf("%d\n", i);
	i = _Generic("17, io", const char *:1, char *:3, const int :2);
	printf("%d\n", i);
	i = _Generic(ti, const char *:1, char *:3, const int :2);
	printf("%d\n", i);
	return 0;
}
