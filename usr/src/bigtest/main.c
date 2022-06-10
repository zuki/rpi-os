#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

int main(void)
{
  char buf[512];
  int fd, i, sectors;

  fd = open("big.file", O_CREAT | O_WRONLY);
  if(fd < 0){
    printf("big: cannot open big.file for writing\n");
    exit(1);
  }

  sectors = 0;
  while(1){
    *(int*)buf = sectors;
    int cc = write(fd, buf, sizeof(buf));
    if(cc <= 0)
      break;
    sectors++;
	if (sectors % 100 == 0)
		printf(".");
  }

  printf("\nwrote %d sectors\n", sectors);

  close(fd);
  fd = open("big.file", O_RDONLY);
  if(fd < 0){
    printf("big: cannot re-open big.file for reading\n");
    exit(1);
  }
  for(i = 0; i < sectors; i++){
    int cc = read(fd, buf, sizeof(buf));
    if(cc <= 0){
      printf("big: read error at sector %d\n", i);
      exit(1);
    }
    if(*(int*)buf != i){
      printf("big: read the wrong data (%d) for sector %d\n",
             *(int*)buf, i);
      exit(1);
    }
  }
  close(fd);

  printf("read; ok\n");

  if(unlink("big.file") < 0){
    printf("big: unlink error\n");
    exit(1);
  }

  printf("done; ok\n");

  return 0;
}
