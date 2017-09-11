#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define die(e) do { fprintf(stderr, "%s\n", e); exit(EXIT_FAILURE); } while (0);

char * do_ping() {
    char * value;
    int statval;
    int link[2];
    pid_t pid;
    char foo[4096];
  
    if (pipe(link)==-1)
      die("pipe");
  
    if ((pid = fork()) == -1)
      die("fork");
  
    if(pid == 0) {
  
      dup2 (link[1], STDOUT_FILENO);
      close(link[0]);
      close(link[1]);
      execl("/bin/ping", "ping", "8.8.8.8", "-c", "1", NULL);
      die("execl");
  
    } else {
  
      close(link[1]);
      read(link[0], foo, sizeof(foo));
      value = strtok (foo, "=");
      value = strtok (NULL, "=");
      value = strtok (NULL, "=");
      value = strtok (NULL, "=");
      value = strtok (value, " ");

      wait(NULL);
              
      return strcat(value, " ms");
      
    }
}

int main() {
  char * value;
    while(getchar() != 27) {
        value = do_ping();
        printf("%s", value);
    }
  return 0;
}