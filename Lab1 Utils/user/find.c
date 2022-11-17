#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

char*
fmtTarget(char *filename){
  static char bufs[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=filename+strlen(filename); p >= filename && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(bufs, p, strlen(p));
  memset(bufs+strlen(p), ' ', DIRSIZ-strlen(p));
  return bufs;
}


void
look(char *path, char *target)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  // open the file and get the file descriptor
  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  // get the status of this file descriptor
  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  // main logic, check file is FILE or DIRECTORY
  
  if(st.type == T_FILE){
    if(strcmp(fmtname(path), fmtTarget(target))==0){
        printf("%s\n", path);
    }
    
    close(fd);
    return;
  }


  if(st.type ==  T_DIR){
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      return;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("find: cannot stat %s\n", buf);
        continue;
      }
      if(strcmp(fmtname(buf), ".             ")==0 || 
        strcmp(fmtname(buf), "..            ")==0){
        continue;
      }
      else{
        look(buf, target);
      }
    }
    return;
  }
  else{
    return;
  }
}

int
main(int argc, char *argv[]){

    if(argc < 3){
        fprintf(2, "You have to pass a path and a specific filename\n");
        exit(1);
    }
 
    look(argv[1], argv[2]);

    exit(0);
}