#include <stdio.h>
int addParens(int argc, char *argv[]){
  printf("((((");
  for(int i=1;i!=argc;i++){
    if(argv[i] && !argv[i][1]){
      switch(argv[i]){
      case '^': printf(")^("); continue;
      case '*': printf("))*(("); continue;
      case '/': printf("))/(("); continue;
      case '+': printf(")))+((("); continue;
      case '-': printf(")))-((("); continue;
      }
    }
    printf("%s", argv[i]);
  }
  printf("))))\n");
  return 0;
}
