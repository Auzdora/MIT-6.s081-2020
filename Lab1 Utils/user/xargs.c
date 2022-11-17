#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

int main(int argc, char *argv[])
{
   char buf[512];
   char store[512];
   char *args[MAXARG];
   char *p;
   int n;

   // extract the string after the xargs
   for(int i=1; i<argc; i++){
      args[i-1] = argv[i];
   }

   n = read(0, buf, sizeof(buf));

   p = store;
   for(int i=0; i<n; i++){
      if(buf[i] == '\n'){
         *p = '\0';
         args[argc-1]=store;

         if(fork()==0){
            exec(argv[1], args);
         }
         else{
            wait((int *) 0);
         }
         p=store;
         continue;
      }
      *p = buf[i];
      p++;
   }


   exit(0);
}