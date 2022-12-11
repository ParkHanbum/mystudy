#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ucontext.h>
#include <unistd.h>
#include <setjmp.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#include "util.h"
#include "signal_handler.h"

void do_segv();

__attribute__((optimize("-O1")))
void do_segv()
{
    fprintf(stderr, "%s\n", __func__);
    int *segv = 0;
    *segv = 1;

    MARK();
}

__attribute__((optimize("-O1")))
void do_segv1()
{
    fprintf(stderr, "%s\n", __func__);
    int *segv = 0;
    *segv = 1;

    MARK();
}

__attribute__((optimize(0)))
void do_signal_handling()
{
    int sig = 0;
    int watch_signals = sigill | sigtrap | sigabrt | sigbus | sigsegv;
    fprintf(stderr, "check : %lx\n", get_func_size((unsigned long)&do_segv));
    TRY(sig, watch_signals, do_segv1) {
        //sleep(10);
        do_segv1();
    }
    CATCH (sig, sigsegv, e) {
        fprintf(stderr, "inside catch!!\n");
        fprintf(stderr, "catch!! signal %d\n", sig);
        fprintf(stderr, "check %d  %lx \n", e.code, e.addr);
    }
    FINAL(watch_signals);
}

static void unittest();
int main() {
    fprintf(stderr, "[%d] INIT NATIVE!!!!!!!!!!!!!!!!\n", getpid());
    unittest();

    int pid = fork();
    if (pid) {
        do_signal_handling();
        do_segv();
    } else {
        sleep(1);
    }
    return 0;
}

static void unittest()
{
    assert(order_to_index(0) == -1);
    assert(order_to_index(sigill) == 0);
    assert(order_to_index(sigchld) == 5);

    assert(get_func_size((unsigned long)&do_segv) > 0);
    printf("%lx  %lx\n", (unsigned long)&do_segv, get_func_size((unsigned long)&do_segv));
}
