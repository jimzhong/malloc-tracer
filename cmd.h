#ifndef __CMD_H__
#define __CMD_H__

enum op_type {ALLOC, FREE, REALLOC};

typedef struct {
    enum op_type type;
    size_t newsize;     // defined if type is ALLOC or REALLOC
    char *oldp;     // defined if type is FREE or REALLOC
} request_t;


typedef struct {
    char *p;
} response_t;


#endif
