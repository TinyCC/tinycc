#include <stdio.h>

typedef struct {
    int data[4];
    double d1;
    double d2;
} Node;

Node init(Node self) {
    self.data[0] = 0;
    self.data[1] = 1;
    self.data[2] = 2;
    self.data[3] = 3;
    self.d1 = 1234;
    self.d2 = 2345;
    return self;
}

void dummy(Node self) {
}

void print_data(Node data) {
    printf ("%d %d %d %d %g %g\n",
            data.data[0], data.data[1], data.data[2], data.data[3],
            data.d1, data.d2);
}

int main(void) {
    /* This code resulted in a bounds checking error */
    Node data;
    dummy (data);
    char val;
    data = init (data);
    print_data(data);
}
