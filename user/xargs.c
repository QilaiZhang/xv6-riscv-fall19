#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

#define ARG_MAX_LENGTH 50

int main(int argc, char* argv[]) {

    if (argc < 2) {
        fprintf(2, "no command specified!\n");
        exit();
    }
    // read arguments from stdin
    char argbuf[ARG_MAX_LENGTH];
    // arguments for subcommand.
    char *subargv[MAXARG];
    int subargc = 0;
    // read subargv from argv to get subcommand and arguments
    for (; subargc + 1 < argc; subargc++) {
        subargv[subargc] = argv[subargc + 1];
    }

    int subargc_start = subargc;
    int runcmd = 0, argbuf_index = 0;
    while (read(0, argbuf + argbuf_index, sizeof(char)) == sizeof(char)) {
        if (argbuf[argbuf_index] == '\n' || argbuf[argbuf_index] == ' ') {
            runcmd = (argbuf[argbuf_index] == '\n') ? 1 : 0;
            argbuf[argbuf_index] = 0;
            subargv[subargc] = malloc(ARG_MAX_LENGTH);
            memmove(subargv[subargc], argbuf, argbuf_index);
            subargc++;
            argbuf_index = 0;
        } else {
            if (argbuf_index == ARG_MAX_LENGTH) continue;
            argbuf_index++;
        }

        if (runcmd) {
            if (fork() == 0) {
                exec(subargv[0], subargv);
            } else {
                wait();
                runcmd = 0;
                argbuf_index = 0;
                subargc = subargc_start;
            }
        }
    }
    exit();
}