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

/*
  函数名: parsecmd
  功能:   将s分割成一个个参数,保存在args中
  输入参数: 
    char *s     待分割字符串
    char **args 指针数组,每个元素指向分割后的字符串首地址
  返回值:
    int cnt     分割后参数个数
*/
int
parsecmd(char *s, char **args)
{
  int cnt = 0;
  while(1)
  {
    while(strchr(whitespace, *s) && *s != '\n')   // parse whitespace
      s++;
    if(*s == '\n')
      break;
    *args = s;
    args++;
    cnt++;
    while(!strchr(whitespace, *s) && *s != '\n')  // parse string
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

/*
  函数名: execcmd
  功能:   执行命令
  输入参数: 
    char **argv 待执行的命令和参数
    int argc    待执行参数个数
*/
void
execcmd(char **argv, int argc)
{
  argv[argc] = 0; // 置0以表示结束
  exec(argv[0], argv);
  fprintf(2, "exec %s failed\n", argv[0]);
  exit(-1);
}

/*
  函数名: redirectcmd
  功能:   若参数中存在"<"或">",进行输入或输出重定向
  输入参数: 
    char **args 参数集
    int argc    参数总数
*/
void
redirectcmd(char **args, int argc)
{
  int c = argc;
  for(int i = argc-1; i >= 0; i--)
  {
    if(*args[i] == '<' && strlen(args[i]) == 1 && args[i+1] != 0) // 若存在'<',输入重定向
    {
      close(0);
      if(open(args[i+1], O_RDONLY) < 0){
        fprintf(2, "open %s failed\n", args[i+1]);
        exit(-1);
      }
      c = i;
    }

    if(*args[i] == '>' && strlen(args[i]) == 1 && args[i+1] != 0) // 若存在'>',输出重定向
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

/*
  函数名: pipecmd
  功能: 创建管道和子进程;
        通道左侧,输出重定向至写通道,通道右侧,输入重定向至读通道;
  输入参数: 
    char **args 参数集
    int argc    参数总数
*/
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

/*
  函数名:runcmd
  功能:首先处理参数集的函数,根据是否存在'|'调用不同的接口
  输入参数: 
    char **args 参数集
    int argc    参数总数
*/
void
runcmd(char **args, int argc)
{
  for(int i = 0; i < argc; i++)
  {
    if(*args[i] == '|' && strlen(args[i]) == 1 )  // 若存在'|'
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
      runcmd(args, argc);         // 子进程执行命令
    }
    wait(0);
  }
  exit(0);
}