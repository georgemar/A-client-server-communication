/*
 * George Maroulis 03/2017 TCP Client UPatras CEID
 * Programming Language = C
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

ssize_t writen(int fd, const void *vptr, size_t n);

int main(int argc, char **argv) {
  char com[2048]; // the message to the server
  int comlen = 0;
  int curlen = 0;
  int i;
  int j = 0;
  int z = 0;
  struct sockaddr_in saddr; // UNIX network address rep
  char *host;               // hostname
  int socketfd, port;       // socket file descriptor, portnumber
  struct hostent *sdet;     // server details
  struct in_addr *addr;
  int ce = 0;  // connection error handler
  int wre = 0; // write read error handler
  // check for correct input
  if (argc < 4)
    return -1;
  host = argv[1];
  port = atoi(argv[2]);
  // create the socket
  socketfd = socket(AF_INET, SOCK_STREAM, 0);
  if (socketfd < 0)
    return -1;
  // searching for the server with the hostname
  sdet = gethostbyname(host);
  if (sdet == NULL)
    return -1;
  // initialize the servers details
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(port);
  addr = (struct in_addr *)sdet->h_addr_list[0];
  saddr.sin_addr = *addr;
  // connecting
  ce = connect(socketfd, &saddr, sizeof(saddr));
  if (ce < 0) {
    return -1;
  }
  int gcount = 0;
  // creates the message for the server
  for (i = 3; i < argc; i++) {
    if (strcmp(argv[i], "put") == 0) {
      com[comlen] = 'p';
      comlen++;
    } else if (strcmp(argv[i], "get") == 0) {
      gcount++;
      com[comlen] = 'g';
      comlen++;
    } else {
      curlen = strlen(argv[i]);
      for (j = 0; j < curlen; j++) {
        com[comlen] = argv[i][j];
        comlen++;
      }
      com[comlen] = '\0';
      comlen++;
      wre = writen(socketfd, &com, (size_t)comlen);
      // wre = write(socketfd, com, comlen);
      if (wre == -1) {
        printf("Error writen\n");
        return -1;
      }
      comlen = 0;
    }
  }
  shutdown(socketfd, SHUT_WR);
  // read from the client
  int len = 0;
  while (1) {
    int ntcount = 0;
    int nc = 0;
    wre = read(socketfd, com, 2048);
    if (wre < 0) {
      printf("read\n");
      return -1;
    } else if (wre == 0) {
      return 0;
    }
    /*for (i = 0; i < wre; i++) {
      if (com[i] == 'f') {
        i++;
        while (i < wre && com[i] != '\0') {
          printf("%c", com[i]);
          i++;
        }
        printf("\n");
      }
      if (com[i] == 'n') {
        printf("\n");
      }
    }*/
    for (i = 0; i < len + wre; i++) {
      ntcount++;
    }
  start:;
    nc = 0;
    if (com[0] == 'f' && ntcount >= 1) {
      nc++;
      for (i = 1; i < len + wre; i++) {
        if (com[i] != '\0') {
          printf("%c", com[i]);
          nc++;
        } else {
          nc++;
          printf("\n");
          break;
        }
      }
      len = wre - nc + len;
      for (i = 0; i < len; i++) {
        com[i] = com[i + nc];
      }
    } else if (com[0] == 'n') {
      printf("\n");
      nc = 1;
      len = wre - nc + len;
      for (i = 0; i < len; i++) {
        com[i] = com[i + nc];
      }
    }
    if (((com[0] == 'n') || (com[0] == 'f' && ntcount >= 1)) && len > 0) {
      wre = 0;
      goto start;
    }
    close(socketfd);
    return 0;
  }
}

ssize_t writen(int fd, const void *vptr, size_t n) {
  size_t nleft;
  ssize_t nwritten;
  const char *ptr;
  ptr = vptr;
  nleft = n;
  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) <= 0) {
      if (errno == EINTR)
        nwritten = 0; /* and call write() again */
      else
        return -1; /* error */
    }
    nleft -= nwritten;
    ptr += nwritten;
  }
  return n;
}