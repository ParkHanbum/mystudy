#ifndef _SIGNAL_HANDLER_H_
#define _SIGNAL_HANDLER_H_

#include <stdbool.h>

static sigjmp_buf point;

#define ARRAY_COUNT(x) sizeof(x)/sizeof(x[0])


static int handle_signals[] = {
SIGILL, SIGTRAP, SIGABRT, SIGBUS, SIGSEGV, SIGCHLD
};

static struct sigaction old_sigact[ARRAY_COUNT(handle_signals)];

typedef struct _watch_func {
    unsigned long func_addr;
    unsigned long end;
} watch_func;
__thread watch_func watched;

typedef struct _Signal_Exception
{
    int code;
    unsigned long addr;
} SException;

typedef enum _signal_order
{
    sigill = 1 << SIGILL,   // 4
    sigtrap = 1 << SIGTRAP, // 5
    sigabrt = 1 << SIGABRT, // 6
    sigbus = 1 << SIGBUS,   // 7
    sigsegv = 1 << SIGSEGV, // 11
    sigchld = 1 << SIGCHLD, // 17
} sig_order;

__thread SException e;

static void set_watch_func(unsigned long func, unsigned long func_size)
{
    watched.func_addr = func;
    watched.end = func + func_size;
}

static sig_order sig_to_order(int sig)
{
    return (sig_order) 1 << sig;
}

static int order_to_index(sig_order order)
{
    int i, idx = -1;

    for (idx = ARRAY_COUNT(handle_signals); idx >= 0; --idx)
        if ((1 << handle_signals[idx]) == order) break;

    return idx;
}

static int sig_to_index(int sig)
{
    int i, idx = -1;

    for (i = 0; i < ARRAY_COUNT(handle_signals); i++) {
        if (handle_signals[i] == sig) {
            idx = i;
            break;
        }
    }

    return idx;
}

static bool is_watched(unsigned long addr)
{
    fprintf(stderr, "check %lx %lx %lx\n", watched.func_addr, addr, watched.end);
    return (watched.func_addr <= addr && watched.end > addr);
}

static void signal_handler(int sig, siginfo_t *si, void *arg)
{
    int idx;
    fprintf(stderr, "received [%d][%d]\n", sig, si->si_signo);
    ucontext_t *ctx = (ucontext_t *)arg;
    mcontext_t mc = ctx->uc_mcontext;

    unsigned long pc = 0;
    int pc_order = 0;
    int sp_order = 0;

#ifdef __x86_64__
    pc_order = 16;
    sp_order = 15;

#elif if __aarch64__
    //  NGREG 34 /* x0..x30 + sp + pc + pstate */
    fprintf(stderr, "address: %p, IP 0x%llx RET 0x%llx", si->si_addr, mc.regs[32], mc.regs[31]);
    pc_order = 32;
    sp_order = 31;
#endif

    if (is_watched((unsigned long)mc.gregs[pc_order])) {
        e.code = sig;
        e.addr = (unsigned long)mc.gregs[pc_order]; // for test
        siglongjmp(point, sig);
    } else {
        // emit signal if there is handler or sigaction
        if (old_sigact[idx].sa_flags & SA_SIGINFO)
        {
            if (old_sigact[idx].sa_sigaction != 0)
                old_sigact[idx].sa_sigaction(sig, si, ctx);
        }
        else
        {
            if (old_sigact[idx].sa_handler != 0)
                old_sigact[idx].sa_handler(sig);
        }
    }
}

static void set_signal_handler(sig_order order)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    for (int i = 0; i < ARRAY_COUNT(handle_signals); i++) {
        int sig = handle_signals[i];
        if ((1 << sig) & order) {
            fprintf(stderr, "sig %d setted \n", sig);
            sigaction(handle_signals[i], &sa, &old_sigact[i]);
        }
    }
}

static void unset_signal_handler(sig_order order) {
     for (int i = 0; i < ARRAY_COUNT(handle_signals); i++) {
        int sig = handle_signals[i];
        if ((1 << sig) & order) {
            fprintf(stderr, "sig %d unsetted \n", sig);
            sigaction(sig, &old_sigact[i], NULL);
        }
    }
}

/**
 * int sigsetjmp(sigjmp_buf env, int savesigs);
 * If, and only if, the savesigs argument provided to sigsetjmp() is nonzero,
 * the process's current signal mask is saved in env and will be restored
 * if a siglongjmp() is later performed with this env.
 */
#define TRY(sig, order, func)        \
    set_signal_handler(order); \
    set_watch_func((unsigned long)&func, get_func_size((unsigned long)&func));       \
    sig = sigsetjmp(point, 1); \
    if (sig == 0)

#define CATCH(sig, sigorder, exception) \
    sig_order handle_sigorder = sigorder; \
    if (sig_to_order(sig) && sigorder)

#define FINAL(order) \
    unset_signal_handler(order);

#endif // _SIGNAL_HANDLER_H_
