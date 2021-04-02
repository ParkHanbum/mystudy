#include <stdio.h>
#include <string.h>

void fnptr(void) {
}

int main(int argc, char **argv) {
    void (*func)(void) = fnptr;
    func();
}
