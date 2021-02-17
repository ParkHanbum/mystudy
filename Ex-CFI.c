#include <stdio.h>
#include <string.h>

#define AUTHMAX 4

struct auth {
    char pass[AUTHMAX];
    void (*func)(struct auth*);
};

void success() {
    printf("Authenticated successfully\n");
}

void failure() {
    printf("Authentication failed\n");
}

void auth(struct auth *a) {
    if (strcmp(a->pass, "pass") == 0)
        a->func = &success;
    else
        a->func = &failure;

}

int main(int argc, char **argv) {
    struct auth a;

    a.func = &auth;

    printf("Enter your password:\n");
    scanf("%s", &a.pass);

    a.func(&a);
}
