#include "daemon.h"

void daemon_ini(void){
  pid_t pid;

  pid = fork();
  if (pid < 0) exit(EXIT_FAILURE);
  if (pid > 0) exit(EXIT_SUCCESS);

  umask(0);
  setlogmask(LOG_UPTO(LOG_INFO));
  openlog("Server system messages:",LOG_CONS|LOG_PID|LOG_NDELAY,LOG_LOCAL3);
  syslog(LOG_ERR,"Initiating new server.");
  if (setsid()<0) {
    syslog(LOG_ERR,"Error creating a new SID for the child pro cess.");
    exit(EXIT_FAILURE);
  }

  /* Se cambia el directorio de trabajo */
  if((chdir("/"))<0){
    syslog(LOG_ERR,"Error changing the current working directory = \"/\"");
    exit(EXIT_FAILURE);
  }

  syslog(LOG_INFO,"Closing standard file descriptors");
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  return;
}


int main(int argc, char const *argv[]) {
  daemon_ini();
  return 0;
}
