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
    int i;
    int num_ops;
    request_t req;
    response_t res;

    read(fd, &num_ops, sizeof(num_ops));

    for (i = 0; i < num_ops; i++) {
        read(fd, &req, sizeof(req));
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

    //wait for EOF
    read(fd, &num_ops, sizeof(num_ops));
}


int main(int argc, char **argv)
{
    assert(argc == 2);
    int fd = atoi(argv[1]);
    dbg_printf("runtrace started. fd = %d\n", fd);
    run(fd);
    close(fd);
    dbg_printf("runtrace finished.\n");
    return 0;
}
