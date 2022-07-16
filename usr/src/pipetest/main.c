#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

char *cmd1[] = {"/usr/bin/cat", "test.txt", NULL};
char *cmd2[] = {"/usr/bin/head", "-5", NULL};
char *cmd3[] = {"/usr/bin/wc", "-l", NULL};
char **cmds[] = {cmd1, cmd2, cmd3};
int cmd_n = 3;

void dopipes(i) {
  pid_t ret;
  int pp[2] = {};
  fprintf(stderr, "dopipes(%d)\n", i);
  if (i == cmd_n - 1) {
    // 左端なら単にexecvp
	fprintf(stderr, "[%d] execvp %s\n", i, cmds[0][0]);
    execvp(cmds[0][0], cmds[0]);
  }
  else {
    // 左端以外ならpipeしてforkして親が実行、子が再帰
    pipe(pp);
	fprintf(stderr, "pipe: pp=[%d, %d]\n", pp[0], pp[1]);
    ret = fork();

    if (ret == 0) {
      // 子プロセスならパイプをstdoutにdup2してdopipes(i+1)で再帰し、
      // 次のforkで親になった側が右からi+1番目のコマンドを実行
	  fprintf(stderr, "[c] close pp[0] = %d\n", pp[0]);
      close(pp[0]);
	  fprintf(stderr, "[c] dup(pp[1] %d to 1\n", pp[1]);
      dup2(pp[1], 1);
	  fprintf(stderr, "[c] close pp[1] = %d\n", pp[1]);
      close(pp[1]);

      dopipes(i+1);
    }
    else {
      // 親プロセスならパイプをstdinにdup2して、
      // 右からi番目のコマンドを実行
	  fprintf(stderr, "[p] close pp[1] = %d\n", pp[1]);
      close(pp[1]);
	  fprintf(stderr, "[p] dup(pp[0] %d to 0\n", pp[0]);
      dup2(pp[0], 0);
	  fprintf(stderr, "[p] close pp[0] = %d\n", pp[0]);
      close(pp[0]);
      fprintf(stderr, "[%d] execvp: %s\n", i, cmds[cmd_n-i-1][0]);
      execvp(cmds[cmd_n-i-1][0], cmds[cmd_n-i-1]);
    }
  }
}

int main(void) {
  pid_t ret;

  ret = fork();
  if (ret == 0)
    dopipes(0);
  else
    wait(NULL);

  return 0;
}
