/*
 * Mdriver.c - Autolab version of the CS:APP Malloc Lab Driver
 *
 * Uses a collection of trace files to tests a malloc/free
 * implementation in mm.c.
 *
 * Copyright (c) 2004-2016, R. Bryant and D. O'Hallaron, All rights
 * reserved.  May not be used, modified, or copied without permission.
 */
#include <assert.h>
#include <errno.h>
#include <float.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include "config.h"
#include "cmd.h"

/**********************
 * Constants and macros
 **********************/

/* Misc */
#define MAXLINE     1024          /* max string size */
#define HDRLINES       4          /* number of header lines in a trace file */
#define LINENUM(i) (i+HDRLINES+1) /* cnvt trace request nums to linenums (origin 1) */


/******************************
 * The key compound data types
 *****************************/

/*
 * There are two different, easily-confusable concepts:
 * - opnum: which line in the file.
 * - index: the block number ; corresponds to something allocated.
 * Remember that index (-1) is the null pointer.
 */



/* Characterizes a single trace operation (allocator request) */
typedef struct {
    enum op_type type;                  /* type of request */
    long index;                         /* index for free() to use later */
    size_t size;                        /* byte size of alloc/realloc request */
} traceop_t;


/* Holds the information for one trace file */
typedef struct {
    char filename[MAXLINE];
    size_t data_bytes;    /* Peak number of data bytes allocated during trace */
    int num_ids;          /* number of alloc/realloc ids */
    int num_ops;          /* number of distinct requests */
    traceop_t *ops;       /* array of requests */
    char **blocks;        /* array of ptrs returned by malloc/realloc... */
    size_t *block_sizes;  /* ... and a corresponding array of payload sizes */
} trace_t;


/********************
 * Global variables
 *******************/

/* Directory where default tracefiles are found */
static char tracedir[MAXLINE] = TRACEDIR;

/* The following are null-terminated lists of tracefiles that may or may not get used */

/* The filenames of the default tracefiles */
static char *default_tracefiles[] = {
    DEFAULT_TRACEFILES, NULL
};

/* Store names of trace files as array of char *'s */
static int num_global_tracefiles = 0;
static char **global_tracefiles = NULL;


/*********************
 * Function prototypes
 *********************/

/* This function enables generating the set of trace files */
static void add_tracefile(char *trace);

/* These functions read, allocate, and free storage for traces */
static trace_t *read_trace(const char *tracedir, const char *filename);
static void reinit_trace(trace_t *trace);
static void free_trace(trace_t *trace);

/* Routines for evaluating the utilization of libc malloc */
static double eval_libc_util(trace_t *trace);


static void usage(char *prog);

static void unix_error(const char *fmt, ...)
    __attribute__((format(printf, 1,2), noreturn));


int main(int argc, char **argv)
{
    int i;
    global_tracefiles = NULL;  /* array of trace file names */
    num_global_tracefiles = 0;    /* the number of traces in that array */

    setbuf(stdout, 0);
    setbuf(stderr, 0);

    char c;
    /*
     * Read and interpret the command line arguments
     */
    while ((c = getopt(argc, argv, "hf:")) != EOF) {
        switch (c) {

        case 'f': /* Use one specific trace file only (relative to curr dir) */
            add_tracefile(optarg);
            strcpy(tracedir, "./");
            break;

        case 'h':
            usage(argv[0]);
            exit(0);

        default:
            usage(argv[0]);
            exit(1);
        }
    }

    if (num_global_tracefiles == 0) {
        int i;
        for (i = 0; default_tracefiles[i]; i++) {
            add_tracefile(default_tracefiles[i]);
        }
    }

    double util;
    trace_t *trace;
    for (i=0; i < num_global_tracefiles; i++) {
        trace = read_trace(tracedir, global_tracefiles[i]);
        util = eval_libc_util(trace) * 100.0;
        printf("tracefile: %-40s utilization: %.2lf%%\n", trace->filename, util);
        free_trace(trace);
    }

    return 0;
}


/*****************************************************************
 * Add trace to global list of tracefiles
 ****************************************************************/
static void add_tracefile(char *trace) {
    global_tracefiles = realloc(global_tracefiles,
                                (num_global_tracefiles+1) * sizeof(char *));
    global_tracefiles[num_global_tracefiles++] = strdup(trace);
}


/**********************************************
 * The following routines manipulate tracefiles
 *********************************************/

/*
 * read_trace - read a trace file and store it in memory
 */
static trace_t *read_trace(const char *tracedir, const char *filename)
{
    FILE *tracefile;
    trace_t *trace;
    char type[MAXLINE];
    int index;
    size_t size;
    int max_index = 0;
    int op_index;
    int ignore = 0;

    /* Allocate the trace record */
    if ((trace = (trace_t *) malloc(sizeof(trace_t))) == NULL) {
        unix_error("malloc 1 failed in read_trace");
    }

    /* Read the trace file header */
    strcpy(trace->filename, tracedir);
    strcat(trace->filename, filename);
    if ((tracefile = fopen(trace->filename, "r")) == NULL) {
        unix_error("Could not open %s in read_trace", trace->filename);
    }

    int iweight;
    ignore += fscanf(tracefile, "%d", &iweight);
    ignore += fscanf(tracefile, "%d", &trace->num_ids);
    ignore +=  fscanf(tracefile, "%d", &trace->num_ops);
    ignore +=  fscanf(tracefile, "%zd", &trace->data_bytes);

    /* We'll store each request line in the trace in this array */
    if ((trace->ops =
         (traceop_t *)malloc(trace->num_ops * sizeof(traceop_t))) == NULL)
        unix_error("malloc 2 failed in read_trace");

    /* We'll keep an array of pointers to the allocated blocks here... */
    if ((trace->blocks =
         (char **)calloc(trace->num_ids, sizeof(char *))) == NULL)
        unix_error("malloc 3 failed in read_trace");

    /* ... along with the corresponding byte sizes of each block */
    if ((trace->block_sizes =
         (size_t *)calloc(trace->num_ids,  sizeof(size_t))) == NULL)
        unix_error("malloc 4 failed in read_trace");


    /* read every request line in the trace file */
    index = 0;
    op_index = 0;
    while (fscanf(tracefile, "%s", type) != EOF) {
        switch(type[0]) {
            case 'a':
                ignore += fscanf(tracefile, "%u %lu", &index, &size);
                trace->ops[op_index].type = ALLOC;
                trace->ops[op_index].index = index;
                trace->ops[op_index].size = size;
                max_index = (index > max_index) ? index : max_index;
                break;
            case 'r':
                ignore += fscanf(tracefile, "%u %lu", &index, &size);
                trace->ops[op_index].type = REALLOC;
                trace->ops[op_index].index = index;
                trace->ops[op_index].size = size;
                max_index = (index > max_index) ? index : max_index;
                break;
            case 'f':
                ignore += fscanf(tracefile, "%u", &index);
                trace->ops[op_index].type = FREE;
                trace->ops[op_index].index = index;
                break;
            default:
                printf("Bogus type character (%c) in tracefile %s\n",
                          type[0], trace->filename);
                exit(-1);
        }
        op_index++;
        if (op_index == trace->num_ops) break;
    }
    fclose(tracefile);
    assert(max_index == trace->num_ids - 1);
    assert(trace->num_ops == op_index);

    return trace;
}

/*
 * reinit_trace - get the trace ready for another run.
 */
static void reinit_trace(trace_t *trace)
{
    memset(trace->blocks, 0, trace->num_ids * sizeof(*trace->blocks));
    memset(trace->block_sizes, 0, trace->num_ids * sizeof(*trace->block_sizes));
    /* block_rand_base is unused if size is zero */
}

/*
 * free_trace - Free the trace record and the four arrays it points
 *              to, all of which were allocated in read_trace().
 */
static void free_trace(trace_t *trace)
{
    free(trace->ops);         /* free the three arrays... */
    free(trace->blocks);
    free(trace->block_sizes);
    free(trace);              /* and the trace record itself... */
}



static double eval_libc_util(trace_t *trace)
{
    int fd[2];
    reinit_trace(trace);

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) < 0) {
        unix_error("socketpair");
    }

    pid_t pid = fork();

    if (pid < 0) {
        unix_error("fork");
    } else if (pid == 0) {
        // child process
        close(fd[0]);
        close(STDIN_FILENO);
        char fdstr[10];
        sprintf(fdstr, "%d", fd[1]);
        if (execlp("./runtrace", "./runtrace", fdstr, (char *)NULL) < 0)
            unix_error("execlp");
    }

    // parent process
    close(fd[1]);

    int index;
    char *oldp, *p;
    size_t oldsize, newsize, size;

    size_t max_total_size = 0;
    size_t total_size = 0;

    request_t req;
    response_t res;

    for (int i = 0; i < trace->num_ops; i++) {
        switch (trace->ops[i].type) {
            case ALLOC:
                index = trace->ops[i].index;
                size = trace->ops[i].size;
                req.type = ALLOC;
                req.newsize = size;
                write(fd[0], &req, sizeof(req));
                read(fd[0], &res, sizeof(res));
                trace->blocks[index] = res.p;
                trace->block_sizes[index] = size;
                total_size += size;
                break;

            case REALLOC:
                index = trace->ops[i].index;
                newsize = trace->ops[i].size;
                oldsize = trace->block_sizes[index];
                oldp = trace->blocks[index];
                req.type = REALLOC;
                req.newsize = newsize;
                req.oldp = oldp;
                write(fd[0], &req, sizeof(req));
                read(fd[0], &res, sizeof(res));
                /* Remember region and size */
                trace->blocks[index] = res.p;
                trace->block_sizes[index] = newsize;
                total_size += (newsize - oldsize);
                break;

            case FREE:
                index = trace->ops[i].index;
                if (index < 0) {
                    size = 0;
                    p = NULL;
                } else {
                    size = trace->block_sizes[index];
                    p = trace->blocks[index];
                }

                if (p != NULL) {
                    req.type = FREE;
                    req.oldp = p;
                    write(fd[0], &req, sizeof(req));
                    read(fd[0], &res, sizeof(res));
                }

                total_size -= size;
                break;

            default:
                assert(0);
        }

        /* update the high-water mark */
        max_total_size = (total_size > max_total_size) ?
            total_size : max_total_size;
    }

    close(fd[0]);
    waitpid(pid, NULL, 0);

    return (double)max_total_size / 10000;
}

/*
 * unix_error - Report the error and its errno.
 */
void unix_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    printf(": %s\n", strerror(errno));
    va_end(ap);
    fflush(NULL);
    exit(1);
}


/*
 * usage - Explain the command line arguments
 */
static void usage(char *prog)
{
    fprintf(stderr, "Usage: %s [-h] [-f <file>]\n", prog);
    fprintf(stderr, "Options\n");
    fprintf(stderr, "\t-h         Print this message.\n");
    fprintf(stderr, "\t-f <file>  Use <file> as the trace file\n");
}
