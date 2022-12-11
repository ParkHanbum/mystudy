#ifndef _UTIL_H_
#define _UTIL_H_

#if defined(__aarch64__) || defined(__arm__)
#define MARK()  {\
        asm volatile("b 1f; .int 0xdeadbeaf;1:");\
}
#elif defined(__i386__) || defined(__amd64__)
#define MARK()  {\
        asm volatile("jmp 1f; .int 0xdeadbeaf;1:");\
}
#else
#error "not expected architecture"
#endif

typedef struct _wipe_point {
    unsigned long start;
    unsigned long end;
} wipe_point;


static unsigned long get_func_size(unsigned long faddr)
{
    unsigned long res = 0;
    unsigned char *fptr = (unsigned char *)faddr;
    for(int MAX_TRY = 100; MAX_TRY > 0; fptr++, MAX_TRY--) {
        if (*(unsigned int *)fptr == 0xdeadbeaf) {
            res = ((unsigned long)fptr) - faddr;
            break;
        }
    }

    return res;
}

#endif // _UTIL_H_
