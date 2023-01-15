//********************************************************************************************************
// Program name: Dining Philosopher with signal control
// Programmer: Chao He 
// g++ dp.cpp -o dp -lpthread
// You need to open 6 terminals:
// terminal 1: ./dp 0 
// terminal 2: ./dp 1 
// terminal 3: ./dp 2 
// terminal 4: ./dp 3 
// terminal 5: ./dp 4 
// terminal 6: send the start sinal using kill -n 30 [PID], send the stop sinal using kill -n 31 [PID]
//********************************************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <cstring>
#include <string.h>
#include <time.h>
#include <iostream>
#include <sys/socket.h> //for socket(), connect()
#include <unistd.h>     //for close()
#include <netinet/in.h> //for internet address family
#include <arpa/inet.h>  //for sockaddr_in, inet_addr()
#include <errno.h>      //for system error numbers
#include <netdb.h>
#include <signal.h>
#include <sys/types.h> //for signal

using namespace std;

#define SERVER_PORT_ID 8000
#define MESSAGESIZE 80

pthread_mutex_t myFork;

int pid[5];
int sigFlag = -1;
char const *ipAddress[] = {
    "127.0.0.1", "127.0.0.1",
    "127.0.0.1", "127.0.0.1",
    "127.0.0.1"};

//Function for waiting time
void waitFor(unsigned int secs)
{
    unsigned int retTime = time(0) + secs; // Get finishing time.
    while (time(0) < retTime)
        ; // Loop until it arrives.
}

void sigusr1_handler(int sig)
{
    printf("\n\n****************************************************\n");
    printf("\n        o(^_^)o System: Receive start signal          \n");
    printf("\n****************************************************\n\n");
    sigFlag = sig;
    //sleep(2);
}

void sigusr2_handler(int sig)
{
    printf("\n\n****************************************************\n");
    printf("\n        o(>_<)o System: Receive terminate signal      \n");
    printf("\n****************************************************\n\n");
    sigFlag = sig;
    //sleep(2);
}

//client thread
void *client(void *number)
{
    //wait untill all the other servers are running
    //waitFor(5);
    printf("\n\n");

    int philosopher = atoi((char *)number);
    //for signal
    pid[philosopher] = getpid();

    /* Set up SIGUSR1 handler for informing others to start*/
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = sigusr1_handler;
    sigaddset(&act.sa_mask, SIGUSR1);
    sigaction(SIGUSR1, &act, NULL);

    /* Set up SIGUSR2 handler for informing others to stop*/
    struct sigaction act2;
    sigemptyset(&act2.sa_mask);
    act2.sa_flags = 0;
    act2.sa_handler = sigusr2_handler;
    sigaddset(&act2.sa_mask, SIGUSR2);
    sigaction(SIGUSR2, &act2, NULL);

    //for socket
    int sockfd, retcode, nread, addrlen;
    char const *serv_host_addr = "127.0.0.1";
    struct sockaddr_in my_addr;
    struct sockaddr_in server_addr;
    char msg[MESSAGESIZE];
    unsigned int waitingTime = 24;
    int server_port_id;
    srand(time(NULL));

    // Finding the adjacent philosopher
    serv_host_addr = ipAddress[philosopher];

    if (philosopher < 4)
        server_port_id = SERVER_PORT_ID + philosopher + 1; // neighbor's port (right port)
    else
        server_port_id = SERVER_PORT_ID;

    // Socket Initialization:
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("Client: socket failed!\n");
        exit(1);
    }

    // initialize neighbor's address and port info
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port_id);
    server_addr.sin_addr.s_addr = inet_addr(serv_host_addr);

    // kill -l
    // (1)HUP INT QUIT ILL TRAP ABRT EMT FPE KILL BUS SEGV SYS PIPE ALRM TERM URG STOP
    // TSTP CONT CHLD TTIN TTOU IO XCPU XFSZ VTALRM PROF WINCH INFO USR1 USR2 (31)
    while (1)
    {

        if (sigFlag == 30)
            break;
        printf("\n\n########## Philosopher %d PID( %d )is waiting...", philosopher, pid[philosopher]);
        sleep(3);
    }

    // change port number and multicast
    for (int i = 0; i < 4; i++)
    {
        server_addr.sin_port = htons(8000 + (server_port_id + i) % 5);
        sendto(sockfd, "start", 5, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    }

    //restore neighbor's port number
    server_addr.sin_port = htons(server_port_id);

    // start simulation
    while (sigFlag != 31)
    {
        printf("\n\nPhilosopher %d PID( %d )is thinking...", philosopher, pid[philosopher]);

        if (pthread_mutex_lock(&myFork) != 0) //unsuccessful lock, client did not hold the fork
        {
            if (waitingTime > 6)
            {
                waitingTime -= 3; //Reduce the waiting time
            }
            waitFor(waitingTime);
        }
        else // client side hold the fork
        {
            strcpy(msg, "lock");
            //cout << "Client: sending signal lock...\n";
            //sending the successful locked signal to the neighbor
            retcode = sendto(sockfd, msg, strlen(msg) + 1, 0,
                             (struct sockaddr *)&server_addr, sizeof(server_addr));
            //printf("Client: send to %s\n",inet_ntoa(server_addr.sin_addr));

            // Receive message from server
            addrlen = sizeof(server_addr); // specify the size of server_addr, this is important !!!!!

            nread = recvfrom(sockfd, msg, MESSAGESIZE, 0,
                             (struct sockaddr *)&server_addr, (socklen_t *)&addrlen);

            if (nread > 0)
            {
                if (strncmp(msg, "locked", 6) == 0) //successfully get neighbor's fork
                {
                    waitingTime = rand() % (24) + 6; //making the waiting time to 0-29 sec again
                    printf("\n\n>>>>>>>>>> Philosopher %d PID( %d )is eating...", philosopher, pid[philosopher]);
                    waitFor(rand() % 10 + 1);      //Eat for random time
                    pthread_mutex_unlock(&myFork); //unlock the fork after eating
                    strcpy(msg, "unlock");         //send unlock msg to neighbor's server after eating random time
                    //cout << "\nClient: sending signal unlock...\n";
                    retcode = sendto(sockfd, msg, strlen(msg) + 1, 0,
                                     (struct sockaddr *)&server_addr, sizeof(server_addr));
                }
                else if (strncmp(msg, "unsec", 5) == 0) //unable to get the fork form neighbor
                {
                    waitingTime -= 3;              //reduce waiting time
                    pthread_mutex_unlock(&myFork); //release left fork on client side
                    //printf("\n --Client: philosopher %d releases left fork \n", philosopher);
                }
                waitFor(waitingTime);
            }
        }
    }

    // receive stop signal and multicast
    for (int i = 0; i < 4; i++)
    {
        if (i == 0)
            printf("\n\nxxxxxxxxxxxxxxxxxxxx Philosopher %d PID( %d ) is exit...\n\n", philosopher, pid[philosopher]);
        server_addr.sin_port = htons(8000 + (server_port_id + i) % 5);
        sendto(sockfd, "stop", 4, 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr));
    }

    //kill( (pid_t)pid[philosopher], SIGKILL );
    close(sockfd);
    exit(0);
    return 0;
}

// server thread
void *server(void *number)
{
    int sockfd;
    struct sockaddr_in my_addr, client_addr;
    int nread, retcode, addrlen;
    char msg[MESSAGESIZE];
    char startBuf[MESSAGESIZE];
    int waitingTime = rand() % (24) + 6;
    int philosopher = atoi((char *)number);
    int my_port_id = SERVER_PORT_ID + philosopher; //my own left port

    // Initialization:
    //cout << "Server: create socket...\n";
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("Server: socket failed!\n");
        exit(1);
    }

    //cout << "Server: binding my local socket...\n";
    memset(&my_addr, 0, sizeof(my_addr));        // Zero out structure
    my_addr.sin_family = AF_INET;                // Internet address family
    my_addr.sin_port = htons(my_port_id);        //My port
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface

    // binding:
    if (::bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
    {
        printf("Server: bind failed!\n");
        exit(2);
    }

    if (recvfrom(sockfd, startBuf, MESSAGESIZE, 0,
                 (struct sockaddr *)&client_addr, (socklen_t *)&addrlen) > 0 ||
        sigFlag == 30)
    {
        //if receive "start" command
        if (strncmp(startBuf, "start", 5) == 0)
        {
            sigFlag = 30;

            while (sigFlag != 31)
            {
                // Wait for client's connection
                printf("\n\nPhilosopher %d PID( %d )is thinking...", philosopher, pid[philosopher]);
                addrlen = sizeof(client_addr); // need to give it the buffer size for sender's address

                nread = recvfrom(sockfd, msg, MESSAGESIZE, 0,
                                 (struct sockaddr *)&client_addr, (socklen_t *)&addrlen);

                if (strncmp(msg, "stop", 4) == 0)
                {
                    //sigFlag = 31;
                    printf("\n\nxxxxxxxxxxxxxxxxxxxx Philosopher %d PID( %d ) is exit...\n\n", philosopher, pid[philosopher]);
                    exit(0);
                    //kill( (pid_t)pid[philosopher], SIGKILL );
                    //break;
                }
                if (nread > 0)
                {
                    if (strncmp(msg, "lock", 4) == 0) //client send request to get right fork
                    {
                        //cout << "Server: client sends request to lock the resource...\n";
                        if (pthread_mutex_lock(&myFork) != 0) //unsucessful lock
                        {
                            strcpy(msg, "unsuc"); //send unsuccessful lock code to client

                            retcode = sendto(sockfd, msg, strlen(msg) + 1, 0,
                                             (struct sockaddr *)&client_addr, sizeof(client_addr));
                            //cout << "Server: send unsuccessful lock code to client\n";
                            if (waitingTime > 6)
                            {
                                waitingTime -= 3; //reducing waiting time
                            }
                            waitFor(waitingTime);
                        }
                        else
                        {
                            strcpy(msg, "locked"); //lock acquired on the fork

                            retcode = sendto(sockfd, msg, strlen(msg) + 1, 0,
                                             (struct sockaddr *)&client_addr, sizeof(client_addr));
                            //cout << "Server: lock acquired on the fork\n";
                        }
                    }
                    else if (strncmp(msg, "unlock", 6) == 0) //client send a request to unlock the fork
                    {
                        pthread_mutex_unlock(&myFork); //unlocked fork
                        //cout << "Server: client send a request to unlock the fork\n";
                    }
                }
            }
            //kill( (pid_t)pid[philosopher], SIGKILL );
        }
    }
    close(sockfd);
    return 0;
}

int main(int argc, char *argv[])
{
    cout << "\n************************************************************\n";
    pthread_t client_thread, server_thread;

    //initiating the mutex resource
    if (pthread_mutex_init(&myFork, NULL) != 0)
    {
        printf("\n mutex fork init failed\n");
        return 1;
    }

    pthread_create(&client_thread, NULL, client, (void *)argv[1]);
    //cout << "Client thread created\n";
    pthread_create(&server_thread, NULL, server, (void *)argv[1]);
    //cout << "Server thread created\n";

    // wait for them to finish
    pthread_join(client_thread, NULL);
    pthread_join(server_thread, NULL);
    pthread_mutex_destroy(&myFork);

    cout << "\n*************************************************************\n";
    return 0;
}
