#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define STDIN 0

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n, i, s; // i and s are iterators for for loops
    struct sockaddr_in serv_addr;           // Socket address structure
    struct hostent *server;
    fd_set active_fd_set, read_fd_set;
    bool shouldLoop;

    char buffer[256];
    if (argc < 5)
    {
       fprintf(stderr,"usage %s hostname knockPort1 knockPort2 mainPort\n", argv[0]);
       exit(0);
    }

    portno = atoi(argv[4]);     // Read Port No from command line

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // Open Socket

    if (sockfd < 0)
    {
      error("ERROR opening socket");
    }

    server = gethostbyname(argv[1]);        // Get host from IP

    if (server == NULL)
    {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET; // This is always set to AF_INET

    // Host address is stored in network byte order
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);

    for (s = 2; s < 4; s++)
    {
      serv_addr.sin_port = htons(atoi(argv[s]));
      if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
          error("ERROR connecting");
      }
      close(sockfd);
      sockfd = socket(AF_INET, SOCK_STREAM, 0); // Open Socket
      usleep(500);
    }

    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
    {
      error("ERROR connecting");
    }

    FD_ZERO (&active_fd_set);
    FD_SET (sockfd, &active_fd_set);
    FD_SET (STDIN, &active_fd_set);
    printf("You have connection with server\n");
    printf("-: ");
    fflush(stdout);
    // Read and write to socket
    shouldLoop = true;
    while (shouldLoop)
    {
      read_fd_set = active_fd_set;
      if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
      {
        perror ("select ERROR");
        exit (EXIT_FAILURE);
      }

      for (i = 0; i < FD_SETSIZE; ++i)
      {
        if (FD_ISSET (i, &read_fd_set))
        {
          if (i == STDIN)
          {
            bzero(buffer,256);
            fgets(buffer,255,stdin);
            n = write(sockfd,buffer,strlen(buffer));
            if (n < 0) 
                error("ERROR writing to socket");
                bzero(buffer,256);
          }
          else if (i == sockfd)
          {
            bzero(buffer,256);
            n = read(sockfd,buffer,255);
            // n = recv(sockfd,buffer,255,0);
            if (n < 0)
            {
              error("ERROR reading from socket");
            }
            else if (n == 0)
            {
              shouldLoop = false;
            }
            else
            {
              printf("\nSERVER: %s\n",buffer);
            }
            printf("-: ");
            fflush(stdout);
          }
        }
      }


     

    }

    close(sockfd);
    return 0;
}

