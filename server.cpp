#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <ctime>
#include <vector>
#include <map>
#include <sys/time.h>

#define KNOCK_PORT_ONE  9985
#define KNOCK_PORT_TWO  9986
#define MAIN_PORT       9987
#define MAXMSG          512
#define MAXPATH         120

//Global variables
std::string server_ID;

struct user
{
  int fd;
  std::string username;
};

std::vector<user> connected_users;

int make_socket (uint16_t port)
{
  int sock;
  struct sockaddr_in name;

  // Creating socket
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
  {
    perror ("ERROR creating socket!");
    exit (EXIT_FAILURE);
  }

  /* Give the socket a name. */
  name.sin_family = AF_INET;
  name.sin_port = htons (port);
  name.sin_addr.s_addr = htonl (INADDR_ANY);
  if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
  {
    perror ("ERROR binding socket!");
    exit (EXIT_FAILURE);
  }

  return sock;
}

void generate_id ()
{
  FILE *fp;
  int status;
  char path[MAXPATH];
  time_t now;
  char timestamp[20];

  /* Reset ID */
  server_ID = "";

  fp = popen("fortune -s", "r");
  if (fp == NULL)
  {
    perror ("ERROR running fortune -s!");
    exit (EXIT_FAILURE);
  }

  while (fgets(path, MAXPATH, fp) != NULL)
  {
    /* Removing newline from the string */
    path[std::string(path).length() - 1] = 0;
    server_ID.append(path);
  }

  /* Adding group initials and timestamp to ID */
  server_ID.append("AAB");

  now = time(0);
  strftime(timestamp, 20, "%X %x", localtime(&now));
    
  server_ID.append(timestamp);

  if (pclose(fp) < 0) {
    perror ("ERROR running pclose()!");
    exit (EXIT_FAILURE);
  }
}

// Returns index of user with given filedescriptor
int find_user_w_fd (int filedes)
{
  for (int i = 0; i < connected_users.size(); i++)
  {
    if (connected_users[i].fd == filedes)
    {
      return i;
    }
  }
  return -1;
}

// Returns filedescriptor of user with given username
int find_fd_w_username (std::string username)
{
  for (int i = 0; i < connected_users.size(); i++)
  {
    if (connected_users[i].username == username)
    {
      return connected_users[i].fd;
    }
  }
  return -1;
}

int connect_user (std::string username, int filedes)
{
  user newUser;
  if (find_user_w_fd (filedes) >= 0)
  {
    // user already connected
    return -1;
  }
  else
  {
    newUser.fd = filedes;
    newUser.username = username;
    connected_users.push_back(newUser);
    return 0;
  }
}

void send_msg_to_fd (std::string answer, int filedes)
{
  int writeN;
  char buffer[answer.length()+1];  
  strcpy(buffer, answer.c_str());
  writeN = write(filedes,buffer,strlen(buffer));
  if (writeN < 0)
  {
    fprintf (stderr, "SERVER: ERROR writing socket");
  }
}

void send_msg_to_all (std::string msg, int filedes)
{
  int n;
  int user_index = find_user_w_fd (filedes);
  std::string full_msg;
  if (user_index < 0)
  {
    fprintf (stderr, "you are not connected. Use CONNECT <username>\n");
    return;
  }

  full_msg = connected_users[user_index].username + " -> ALL: " + msg;

  for (int i = 0; i < connected_users.size(); i++)
  {
    send_msg_to_fd(full_msg, connected_users[i].fd);
  }
}

void send_private_msg (std::string userAndMsg, int filedes)
{
  std::string username, msg, full_msg;
  int receiver_fd, sender_index;

  sender_index = find_user_w_fd(filedes);
  if (sender_index < 0)
  {
    send_msg_to_fd("You are not connected. ==> CONNECT <username>", filedes);
    return;
  }

  std::size_t pos = userAndMsg.find(" ");
  if (pos <= 0)
  {
    send_msg_to_fd("Invalid command. ==> MSG <username> <message>", filedes);
    return;
  }

  username = userAndMsg.substr(0, pos);
  msg = userAndMsg.substr(pos + 1, userAndMsg.length() - (pos + 1));
  if(username.length() <= 0)
  {
    send_msg_to_fd("Missing username. ==> MSG <username> <message>", filedes);
    return;
  }
  else if (msg.length() <= 0)
  {
    send_msg_to_fd("Missing message. ==> MSG <username> <message>", filedes);
    return;
  }

  receiver_fd = find_fd_w_username(username);
  if (receiver_fd < 0)
  {
    send_msg_to_fd("The user with given username is not connected.", filedes);
    return;
  }

  full_msg = connected_users[sender_index].username + " -> " + username + ": " + msg;

  // Sending message to receiver
  send_msg_to_fd(full_msg, receiver_fd);

  // Showing message sent by sender
  send_msg_to_fd(full_msg, filedes);
}

int read_from_client (int filedes)
{
  char buffer[MAXMSG];
  int nbytes;
  std::string answer = "";

  nbytes = read (filedes, buffer, MAXMSG);
  if (nbytes < 0)
  {
    return -1;
  }
  else if (nbytes == 0)
    return -1;
  else
  {    
    buffer[nbytes -1 ] = 0;
    std::string command = buffer;
    // scanf("%[^\]s", buffer); TODO, split buffer between character

    if (command.substr(0,2) == "ID")
    {
      answer = "Server ID: " + server_ID;
      send_msg_to_fd(answer, filedes);
      return 0;
    }
    else if (command.substr(0,9) == "CHANGE ID")
    {
      fprintf (stderr, "SERVER: Changing server ID!\n");
      answer = "Changing server ID...";
      send_msg_to_fd(answer, filedes);
      generate_id();
      return 0;
    }
    else if (command.substr(0,8) == "CONNECT " && command.substr(8,1) != "")
    {
      char username[MAXMSG];
      strcpy (username, command.substr(8, MAXMSG).c_str());
      fprintf (stderr, "SERVER: Connecting to chatserver as %s\n", username);
      if (connect_user(username, filedes) < 0)
      {
        fprintf (stderr, "SERVER: User already connected with another username\n");
        answer = "You are already connected with another username!";
      }
      else
      {
        answer = "Connected as ==> " + command.substr(8, MAXMSG);
      }
      send_msg_to_fd(answer, filedes);
      return 0;
    }
    else if (command.substr(0,3) == "WHO")
    {
      answer = "ONLINE USERS:";
      for (int i = 0; i < connected_users.size(); i++)
      {
        answer += "\n -> " + connected_users[i].username;
      }
      send_msg_to_fd(answer, filedes);
      return 0;
    }
    else if (command.substr(0,8) == "MSG ALL ")
    {
      fprintf (stderr, "Server: MSG to all\n");
      send_msg_to_all(command.substr(8, command.length() - 8), filedes);
      return 0;
    }
    else if (command.substr(0,4) == "MSG ")
    {
      fprintf (stderr, "Server: Trying to send private MSG\n");
      send_private_msg(command.substr(4, command.length() - 4), filedes);
      return 0;
    }
    else if (command.substr(0,5) == "LEAVE")
    {
      answer = "GOOD BYE!";
      send_msg_to_fd(answer, filedes);
      return -1;
    }
    else if (command.substr(0,4) == "HELP")
    {
      answer = "\n- ID\n";
      answer += "- CHANGE ID\n";
      answer += "- WHO\n";
      answer += "- CONNECT <username>\n";
      answer += "- MSG ALL <message>\n";
      answer += "- MSG <username>\n";
      answer += "- LEAVE";
      send_msg_to_fd(answer, filedes);
      return 0;
    }
    else
    {
      answer = "Not a valid command! Type HELP for list of valid commands.";
      send_msg_to_fd(answer, filedes);
      return 0;
    }

    return 0;
  }
}

int setup_port_listen (int sock, int port)
{
  sock = make_socket (port);
  if (listen (sock, 1) < 0)
  {
    perror ("Error listening to port!");
    exit (EXIT_FAILURE);
  }
  return sock;
}

void disconnect_user (int filedes)
{
  int user_index = find_user_w_fd (filedes);
  if (user_index >= 0)
  {
    fprintf (stderr, "SERVER: %s disconnected\n", connected_users[user_index].username.c_str());
    connected_users.erase(connected_users.begin() + user_index);
  }
  else
  {
    fprintf (stderr, "SERVER: User with no username disconnected\n");
  }
}

int main (void)
{
  extern int make_socket (uint16_t port);
  int sock, first_knock_sock, second_knock_sock;
  fd_set active_fd_set, read_fd_set;
  int i;
  struct sockaddr_in clientname, clientname2, clientname3, clientname4;
  socklen_t size;
  int client_command_ID;
  time_t now_time;
  struct tm first_tm, second_tm, third_tm;

  struct timeval timer_microsec;
  long long int timestamp_microsec_first, timestamp_microsec_second, timestamp_microsec_third;


  // Generate server ID
  generate_id();

  // Setting up port listening.
  sock = setup_port_listen (sock, MAIN_PORT);

  first_knock_sock = setup_port_listen (first_knock_sock, KNOCK_PORT_ONE);

  second_knock_sock = setup_port_listen (second_knock_sock, KNOCK_PORT_TWO);

  // Initialize the set of active sockets.
  FD_ZERO (&active_fd_set);
  FD_SET (sock, &active_fd_set);
  FD_SET (first_knock_sock, &active_fd_set);
  FD_SET (second_knock_sock, &active_fd_set);

  while (1)
  {
    // Block until input arrives on one or more active sockets.
    read_fd_set = active_fd_set;
    if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
    {
      perror ("select ERROR");
      exit (EXIT_FAILURE);
    }

    // Service all the sockets with input pending.
    for (i = 0; i < FD_SETSIZE; ++i)
      if (FD_ISSET (i, &read_fd_set))
      {
        if (i == first_knock_sock)
        {
          printf("1 Knock\n");

          // Getting first knock timestamp
          if (!gettimeofday(&timer_microsec, NULL))
          {
            timestamp_microsec_first = ((long long int) timer_microsec.tv_sec) * 1000000ll + (long long int) timer_microsec.tv_usec;
          }
          else {
            timestamp_microsec_first = -1;
          }


          int newi;
          size = sizeof (clientname);
          newi = accept (first_knock_sock,
                        (struct sockaddr *) &clientname,
                        &size);
          if (newi < 0)
          {
            perror ("ERROR accepting client!");
            exit (EXIT_FAILURE);
          }
          close(newi);
        }
        else if (i == second_knock_sock)
        {
          printf("2 Knock\n");

          // Getting second knock timestamp
          if (!gettimeofday(&timer_microsec, NULL))
          {
            timestamp_microsec_second = ((long long int) timer_microsec.tv_sec) * 1000000ll + (long long int) timer_microsec.tv_usec;
          }
          else {
            timestamp_microsec_second = -1;
          }

          int newi;
          size = sizeof (clientname);
          newi = accept (second_knock_sock,
                        (struct sockaddr *) &clientname,
                        &size);
          if (newi < 0)
          {
            perror ("ERROR accepting client!");
            exit (EXIT_FAILURE);
          }
          close(newi);
        }
        else if (i == sock)
        {
          // Getting third knock timestamp
          if (!gettimeofday(&timer_microsec, NULL))
          {
            timestamp_microsec_third = ((long long int) timer_microsec.tv_sec) * 1000000ll + (long long int) timer_microsec.tv_usec;
          }
          else {
            timestamp_microsec_third = -1;
          }

          // Checking if the port knocking was done in the correct order
          if (((timestamp_microsec_third - timestamp_microsec_first) <= 120000000) && (timestamp_microsec_second > timestamp_microsec_first) && (timestamp_microsec_third > timestamp_microsec_second))
          {
            printf("3 ACCEPT\n");
            // Resetting timers
            timestamp_microsec_first = 0;
            timestamp_microsec_second = 0;
            timestamp_microsec_third = 0;

            // Connection request on main socket.
            int newi;
            size = sizeof (clientname);
            newi = accept (sock,
                          (struct sockaddr *) &clientname,
                          &size);
            if (newi < 0)
            {
              perror ("ERROR accepting client!");
              exit (EXIT_FAILURE);
            }
            fprintf (stderr,
                    "Server: connect from host %s, port %hd.\n",
                    inet_ntoa (clientname.sin_addr),
                    ntohs (clientname.sin_port));
            FD_SET (newi, &active_fd_set);
            fflush(stdout);
          }
          else
          {
            printf("REFUSE\n");

            int newi;
            size = sizeof (clientname);
            newi = accept (sock,
                          (struct sockaddr *) &clientname,
                          &size);
            if (newi < 0)
            {
              perror ("ERROR accepting client!");
              exit (EXIT_FAILURE);
            }
            close(newi);
          }
        }
        else
        {
          // read data from connected socket.
          if (read_from_client (i) < 0)
          {
            disconnect_user (i);
            close (i);
            FD_CLR (i, &active_fd_set);
          }
        }
      }
  }
}