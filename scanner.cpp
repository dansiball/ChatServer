#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <algorithm>
#include <vector>
#include <ctime>

void error(const char* msg) 
{
  perror(msg);
  exit(0);
}

int openSock(int sockfd) 
{
  sockfd = socket(AF_INET, SOCK_STREAM, 0); // Open Socket
  if (sockfd < 0) 
      error("ERROR opening socket");
  return sockfd;
}

int main(int argc, char* argv[]) 
{
  std::vector<int> ports;  // Vonerable ports
  int sockfd, n, low, top;
  struct sockaddr_in serv_addr;  // Socket address structure
  struct hostent* server;

  char buffer[256];
  if (argc < 4) {
    fprintf(stderr, "missing hostname\n");
    exit(0);
  }

  low = atoi(argv[2]);
  top = atoi(argv[3]);

  for(int i = low; i < top; i++ )
  {
    ports.push_back(i);
  }

  sockfd = openSock(sockfd);

  sockfd = socket(AF_INET, SOCK_STREAM, 0);  // Open Socket

  if (sockfd < 0) error("ERROR opening socket");

  server = gethostbyname(argv[1]);  // Get host from IP

  if (server == NULL)
  {
    fprintf(stderr, "ERROR, no such host\n");
    exit(0);
  }

  bzero((char*)&serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;  // This is always set to AF_INET

  // Host address is stored in network byte order
  bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr,
        server->h_length);

  for (int i = 0; i < ports.size(); i++) 
  {
    serv_addr.sin_port = htons(ports[i]);

    //Loop that only prints out if it find 3 consecutive open ports
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
      close(sockfd);
      sockfd = openSock(sockfd);
    } 
    else 
    {
      //found open prort
      close(sockfd);
      sockfd = openSock(sockfd);
      serv_addr.sin_port = htons(ports[i+1]);
      if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
      {
        close(sockfd);
        sockfd = openSock(sockfd);
        i++;
      }
      else 
      {
        //found open prort
        close(sockfd);
        sockfd = openSock(sockfd);
        serv_addr.sin_port = htons(ports[i+2]);
        if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
        {
          close(sockfd);
          sockfd = openSock(sockfd);
          i+=2;
        }
        else
        {
          //found open prort
          printf("Port: %d - OPEN\n", ports[i]);
          printf("Port: %d - OPEN\n", ports[i+1]);
          printf("Port: %d - OPEN\n", ports[i+2]);
          close(sockfd);
          sockfd = openSock(sockfd);
          i+=2;
        }
      }
    }
  }

  return 0;
}

