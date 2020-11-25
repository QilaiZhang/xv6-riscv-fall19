#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define MAXBUF 100
#define MAXARGS 10

char *whitespace = " ";

void
getcmd(char *buf, int nbuf)
{
  printf("@ ");
  gets(buf, nbuf);
}

void
panic(char *s)
{
  fprintf(2, "%s\n", s);
  exit(-1);
}

int
parsecmd(char *s, char **args)
{
  int cnt = 0;
  while(1)
  {
    while(strchr(whitespace, *s) && *s != '\n')
      s++;
    if(*s == '\n')
      break;
    *args = s;
    args++;
    cnt++;
    while(!strchr(whitespace, *s) && *s != '\n')
      s++;
    if(*s == '\n')
      break;
    *s = 0;
    s++;
  }
  *s = 0;
  *args = 0;
  return cnt;
}

void
execcmd(char **argv, int argc)
{
  argv[argc] = 0;
  exec(argv[0], argv);
  fprintf(2, "exec %s failed\n", argv[0]);
}

void
redirectcmd(char **args, int argc)
{
  int c = argc;
  for(int i = argc-1; i >= 0; i--)
  {
    if(*args[i] == '<' && strlen(args[i]) == 1 && args[i+1] != 0)
    {
      close(0);
      if(open(args[i+1], O_RDONLY) < 0){
        fprintf(2, "open %s failed\n", args[i+1]);
        exit(-1);
      }
      c = i;
    }
    if(*args[i] == '>' && strlen(args[i]) == 1 && args[i+1] != 0)
    {
      close(1);
      if(open(args[i+1], O_WRONLY|O_CREATE) < 0){
        fprintf(2, "open %s failed\n", args[i+1]);
        exit(-1);
      }
      c = i;
    }
  }
  execcmd(args, c);
}

void
pipecmd(char **args, int argc, int q)
{
  int p[2];
  if(pipe(p) < 0)
    panic("pipe");
  if(fork() == 0){
    close(1);
    dup(p[1]);
    close(p[0]);
    close(p[1]);
    redirectcmd(args, q);
  }
  if(fork() == 0){
    close(0);
    dup(p[0]);
    close(p[0]);
    close(p[1]);
    args += q+1;
    argc -= q+1;
    redirectcmd(args, argc);
  }
  close(p[0]);
  close(p[1]);
  wait(0);
  wait(0);
  exit(0);
}

void
runcmd(char **args, int argc)
{
  for(int i = 0; i < argc; i++)
  {
    if(*args[i] == '|' && strlen(args[i]) == 1)
      pipecmd(args, argc, i);
  }
  redirectcmd(args, argc);
}


int
main()
{
  static char buf[MAXBUF];
  static char *args[MAXARGS];
  int argc;

  while(1){
    getcmd(buf, sizeof(buf));
    if(buf[0] == 0) // EOF
      exit(0);
    if(fork()==0){
      argc = parsecmd(buf, args);
      runcmd(args, argc);
    }
    wait(0);
  }
  exit(0);
}