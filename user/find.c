#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void
find(char *path, char *name)
{

}

int
main(int argc, char *argv[])
{
  if(argc != 3){
    fprintf(2,"Invalid\n");
    exit();
  }
  find(argv[1],argv[2]);
  exit();
}
