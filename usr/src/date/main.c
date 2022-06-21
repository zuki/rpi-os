#include <stdio.h>
#include <time.h>

char * const wdays[] = { "日", "月", "火", "水", "木", "金", "土" };

int main(int argc, char *argv[])
{
    time_t t = time(NULL);
    struct tm *local = localtime(&t);

    printf("%04d年%2d日%2d日 %s曜日 %2d時%2d分%2d秒 JST\n",
        local->tm_year + 1900, local->tm_mon + 1, local->tm_mday,
        wdays[local->tm_wday], local->tm_hour, local->tm_min, local->tm_sec);

    return 0;
}
