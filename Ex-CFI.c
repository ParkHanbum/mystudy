#include <stdio.h>
#include <string.h>

#define AUTHMAX 4

struct auth {
    char pass[AUTHMAX];
    void (*func)(struct auth*);
};

void fnptr(struct auth *a) {
}

int main(int argc, char **argv) {
    struct auth a;
    a.func = &fnptr;
    a.func(&a);
}
