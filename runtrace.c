#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <unistd.h>

#include "cmd.h"

#ifdef DEBUG
/* When debugging is enabled, the underlying functions get called */
#define dbg_printf(...) printf(__VA_ARGS__)
#else
/* When debugging is disabled, no code gets generated */
#define dbg_printf(...)
#endif

static void run(int fd)
{
    ssize_t nread;
    request_t req;
    response_t res;

    for (;;) {
        nread = read(fd, &req, sizeof(req));
        if (nread == 0) {
            // EOF
            break;
        } else if (nread < 0) {
            perror("read");
            exit(1);
        }
        switch (req.type) {
            case ALLOC:
                dbg_printf("alloc %zd\n", req.newsize);
                res.p = malloc(req.newsize);
                break;
            case REALLOC:
                dbg_printf("realloc %zd, %p\n", req.newsize, req.oldp);
                res.p = realloc(req.oldp, req.newsize);
                break;
            case FREE:
                dbg_printf("free %p\n", req.oldp);
                free(req.oldp);
                res.p = NULL;
                break;
            default:
                assert(0);
                break;
        }
        dbg_printf("res %p\n", res.p);
        write(fd, &res, sizeof(res));
    }
}


int main(int argc, char **argv)
{
    assert(argc == 2);
    int fd = atoi(argv[1]);
    dbg_printf("runtrace started. fd = %d\n", fd);
    assert(mallopt(M_MMAP_MAX, 0));
    //disable the use of mmap for large allocation requests.
    assert(mallopt(M_TRIM_THRESHOLD, -1));
    // do not shrink the heap
    run(fd);
    close(fd);
    dbg_printf("runtrace finished.\n");
    return 0;
}
