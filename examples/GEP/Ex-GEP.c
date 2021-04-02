#include <stdio.h>

struct integer_struct {
    int f1;
    int f2;
};

struct auth {
    char pass[128];
    void (*func)(struct auth*);
};

void example(struct integer_struct *P) {
    P[0].f1 = P[1].f1 + P[2].f1;
    P[0].f2 = P[1].f2 + P[2].f2;
}

void fnptr(struct auth *a)
{
}

int main()
{
    struct integer_struct ARRAY[3];
    struct auth a;
    int i;

    for (i = 0; i < 3; i++) {
        ARRAY[i].f1 = i;
        ARRAY[i].f2 = i;
    }
    example(ARRAY);

    a.func = &fnptr;
    a.func(&a);
}
