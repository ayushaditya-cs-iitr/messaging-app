#include<stdlib.h>
#include<stdio.h>
#include"helper.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>

ssize_t rio_readn(int fd, void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
        if ((nread = read(fd, bufp, nleft)) < 0) {
            if (errno != EINTR) {
                return -1; 
            }

          
            nread = 0;
        } else if (nread == 0) {
            break; /* EOF */
        }
        nleft -= (size_t)nread;
        bufp += nread;
    }
    return (ssize_t)(n - nleft);  
}

 
ssize_t rio_writen(int fd, const void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nwritten;
    const char *bufp = usrbuf;

    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
            if (errno != EINTR) {
                return -1;  
            }

            
            nwritten = 0;
        }
        nleft -= (size_t)nwritten;
        bufp += nwritten;
    }
    return (ssize_t)n;
}

 
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n) {
    size_t cnt;

    while (rp->rio_cnt <= 0) {  
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0) {
            if (errno != EINTR) {
                return -1; 
            }

            
        } else if (rp->rio_cnt == 0) {
            return 0; 
        } else {
            rp->rio_bufptr = rp->rio_buf;  
        }
    }

     
    cnt = n;
    if ((size_t)rp->rio_cnt < n) {
        cnt = (size_t)rp->rio_cnt;
    }
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return (ssize_t)cnt;
}

 
void rio_readinitb(rio_t *rp, int fd) {
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}


ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
        if ((nread = rio_read(rp, bufp, nleft)) < 0) {
            return -1;  
        } else if (nread == 0) {
            break;  
        }
        nleft -= (size_t)nread;
        bufp += nread;
    }
    return (ssize_t)(n - nleft); 
}

 
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) {
    size_t n;
    ssize_t rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) {
        if ((rc = rio_read(rp, &c, 1)) == 1) {
            *bufp++ = c;
            if (c == '\n') {
                n++;
                break;
            }
        } else if (rc == 0) {
            if (n == 1) {
                return 0;  
            } else {
                break;  
            }
        } else {
            return -1; 
        }
    }
    *bufp = 0;
    return (ssize_t)(n - 1);
}