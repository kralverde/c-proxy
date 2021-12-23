#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <strings.h>
#include <string.h>

#include "argparse.h"

#define MAX_RECV 4096

static void print_bytes(void *buf, int size)
{
    int i;
    for (i = 0; i < size; i++)
    {
        if (i > 0) printf(":");
        printf("%02X", ((uint8_t *)buf)[i]);
    }
    printf("\n");
}

int main (int argc, char *argv[])
{
    int socket_fd;
    c_args args;
    uint8_t recv_buffer[MAX_RECV], preamble[4], send_buffer[4+MAX_RECV];
    struct pollfd *fds;
    uint8_t end_server = 0, close_connection;
    int nfds = 1, new_nfds, ret_val, i, j, max_fds, cnt, len, on = 1;
    uint16_t temp_16;

    if (parse_client_arguments(argc, argv, &args) != 0)
    {
        exit(EXIT_FAILURE);
    }

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("failed to create socket");
        exit(EXIT_FAILURE);
    }

    if(connect(socket_fd, (struct sockaddr *)&(args.proxy_addr), sizeof(args.proxy_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    send(socket_fd, args.password_hash, SHA256_BLOCK_SIZE, 0);
    read(socket_fd, recv_buffer, 1);
    
    ret_val = ioctl(socket_fd, FIONBIO, &on);
    if (ret_val < 0)
    {
        perror("ioctl(1) failed");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    max_fds = 1 + recv_buffer[0];
    fds = calloc(max_fds, sizeof (struct pollfd));
    if (fds == NULL)
    {
        printf("Failed to allocate memory for the fds.\n");
        exit(EXIT_FAILURE);
    }

    memset(fds, 0 , sizeof(*fds));

    fds[0].fd = socket_fd;
    fds[0].events = POLLIN;
   
    while (!end_server)
    {
        
        printf("State:\n");
        for (i = 0; i < max_fds; i++)
        {
            j = fds[i].fd;
            if (j > 0)
            {
                printf("%d: %d\n", i, j);
            }
        }
        printf("#FDS %d\n", nfds);
        printf("Waiting on poll()...\n");
        
        if (nfds < 1)
        {
            printf("Bad #fds\n");
            exit(EXIT_FAILURE);
        }

        ret_val = poll(fds, max_fds, -1);
        if (ret_val < 0)
        {
            perror("poll() failed");
            break;
        }
        if (ret_val == 0)
        {
            printf("Timeout\n");
            break;
        }
        cnt = 0;
        new_nfds = nfds;
        for (i = 0; i < max_fds; i++)
        {
            bzero(recv_buffer, sizeof(recv_buffer));
            bzero(send_buffer, sizeof(send_buffer));
            bzero(preamble, sizeof(preamble));

            if (cnt > new_nfds)
            {
                break;
            }
            if (fds[i].fd <= 0)
            {
                continue;
            }
            if (fds[i].revents == 0)
            {
                continue;
            }
            else if (fds[i].revents != POLLIN)
            {
                printf("Error! revents = %d (%d)\n", fds[i].revents, i);
                if (fds[i].fd != socket_fd && fds[i].fd > 0)
                {
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    nfds--;
                    printf("nfds--1\n");
                    preamble[0] = (uint8_t)(i-1);
                    preamble[1] = 1;
                    preamble[2] = 0;
                    preamble[3] = 0;
                    ret_val = send(socket_fd, preamble, 4, 0);
                }
                else
                {
                    end_server = 1;
                }
                break;
            }
            else if (fds[i].fd == socket_fd)
            {
                cnt++;
                close_connection = 0;
                do
                {
                    ret_val = recv(socket_fd, send_buffer, MAX_RECV + 4, 0);
                    if (ret_val < 0)
                    {
                        if (errno != EWOULDBLOCK)
                        {
                            perror("recv() failed");
                            close_connection = 1;
                        }
                        break;
                    }

                    if (ret_val == 0)
                    {
                        printf("Connection closed (1)\n");
                        close_connection = 1;
                        break;
                    }

                    printf("Received:\n");
                    print_bytes(send_buffer, ret_val);

                    cnt = 0;
                    if (send_buffer[1])
                    {
                        if (fds[send_buffer[0] + 1].fd > 0)
                        {
                            printf("Closing connection %d\n", send_buffer[0]);
                            close(fds[send_buffer[0] + 1].fd);
                            fds[send_buffer[0] + 1].fd = -1;
                            nfds--;
                            printf("nfds--2\n");
                        }
                    }
                    else
                    {
                        memcpy(&temp_16, &send_buffer[2], 2);
                        temp_16 = ntohs(temp_16);

                        if (temp_16 != (uint16_t)(ret_val - 4))
                        {
                            printf("Bad length expected %d, was %d\n", temp_16, (ret_val-4));
                            exit(EXIT_FAILURE);
                        }

                        j = fds[1+send_buffer[0]].fd;
                        if (j <= 0)
                        {
                            if ((j = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                                perror("failed to create socket");
                                send_buffer[1] = 1;
                                send_buffer[2] = 0;
                                send_buffer[3] = 0;
                                ret_val = send(socket_fd, send_buffer, 4, 0);
                                fds[1+send_buffer[0]].fd = -1;
                                printf("nfds--3\n");
                                if (ret_val < 0)
                                {
                                    perror("send() failed");
                                    close_connection = 1;
                                }
                                break;
                            }
                            if(connect(j, (struct sockaddr *)&(args.service_addr), sizeof(args.service_addr)) < 0) {
                                perror("Connection failed");
                                close(j);
                                send_buffer[1] = 1;
                                send_buffer[2] = 0;
                                send_buffer[3] = 0;
                                ret_val = send(socket_fd, send_buffer, 4, 0);
                                fds[1+send_buffer[0]].fd = -1;
                                printf("nfds--4\n");
                                if (ret_val < 0)
                                {
                                    perror("send() failed");
                                    close_connection = 1;
                                }
                                break;
                            }
                            if (ioctl(j, FIONBIO, &on) < 0)
                            {
                                perror("ioctl(2) failed");
                                send_buffer[1] = 1;
                                send_buffer[2] = 0;
                                send_buffer[3] = 0;
                                ret_val = send(socket_fd, send_buffer, 4, 0);
                                fds[1+send_buffer[0]].fd = -1;
                                close(j);
                                printf("nfds--5");
                                if (ret_val < 0)
                                {
                                    perror("send() failed");
                                    close_connection = 1;
                                }
                                break;
                            }

                            fds[1+send_buffer[0]].fd = j;
                            fds[1+send_buffer[0]].events = POLLIN;
                            nfds++;
                        }
                        printf("Sent\n");
                        print_bytes(&send_buffer[4], temp_16);
                        ret_val = send(j, &send_buffer[4], temp_16, 0);
                        if (ret_val < 0)
                        {
                            perror("send() failed");
                            send_buffer[1] = 1;
                            send_buffer[2] = 0;
                            send_buffer[3] = 0;
                            ret_val = send(socket_fd, send_buffer, 4, 0);
                            close(j);
                            fds[1+send_buffer[0]].fd = -1;
                            nfds--;
                            printf("nfds--6\n");
                            if (ret_val < 0)
                            {
                                perror("send() failed");
                                close_connection = 1;
                                break;
                            }
                        }
                    }
                } while (1);
                if (close_connection)
                {
                    printf("Main connection closed.\n");
                    end_server = 1;
                    break;
                }
            }
            else
            {
                cnt++;
                close_connection = 0;
                do
                {
                    ret_val = recv(fds[i].fd, recv_buffer, MAX_RECV, 0);
                    if (ret_val < 0)
                    {
                        if (errno != EWOULDBLOCK)
                        {
                            perror("recv() failed");
                            close_connection = 1;
                        }
                        break;
                    }

                    if (ret_val == 0)
                    {
                        printf("Connection closed (3)\n");
                        close_connection = 2;
                        break;
                    }

                    printf("Recieved:\n");
                    print_bytes(recv_buffer, ret_val);

                    preamble[0] = (uint8_t)(i-1);
                    preamble[1] = 0;
                    len = ret_val;
                    temp_16 = htons((uint16_t)ret_val);
                    memcpy(&preamble[2], &temp_16, 2);
                    memcpy(send_buffer, preamble, 4);
                    memcpy(&send_buffer[4], recv_buffer, len);

                    printf("Sending with size %d:\n", ret_val);
                    print_bytes(send_buffer, ret_val + 4);

                    ret_val = send(socket_fd, send_buffer, len + 4, 0);
                    if (ret_val < 0)
                    {
                        perror("send() failed");
                        close_connection = 1;
                        break;
                    }

                } while(1);
        
                if (close_connection)
                {
                    if (fds[i].fd > 0)
                    {
                        close(fds[i].fd);
                        fds[i].fd = -1;
                        nfds--;
                        printf("nfds--7\n");
                        if (close_connection < 2)
                        {
                            preamble[0] = (uint8_t)(i-1);
                            preamble[1] = 1;
                            preamble[2] = 0;
                            preamble[3] = 0;
                            ret_val = send(socket_fd, preamble, 4, 0);
                        }
                    }
                }
            }
        }
    }
    for (i = 0; i < max_fds; i++)
    {
        j = fds[i].fd;
        if (j > 0)
        {
            close(j);
        }
    }
}