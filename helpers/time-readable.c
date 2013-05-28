#include <stdio.h>
#include <sys/time.h>
#include <time.h>

// gcc time-readable.c -o time-readable 

int main(){
    struct timeval tv;
    time_t nowtime;
    struct tm *nowtm;
    char tmbuf[64], buf[64];

    gettimeofday(&tv, NULL);
    nowtime = tv.tv_sec;
    nowtm = localtime(&nowtime);
    strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
    snprintf(buf, sizeof buf, "%s.%06d", tmbuf, tv.tv_usec);
    printf("%s\n", buf); 
    return 0; 
}
