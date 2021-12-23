#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <strings.h>
#include <endian.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
    uint8_t recv_buffer[MAX_RECV], preamble[4], send_buffer[MAX_RECV+4], main_cache[MAX_RECV];
    uint8_t close_connection, end_server = 0, conn_index;
    uint16_t temp_16, main_size = 0, main_wanted = 0;
    int ret_val, on = 1, cnt, i, j;
    unsigned int len;
    int service_listen_fd = -1, clients_listen_fd = -1, service_fd = -1, new_fd;
    int nfds = 2, new_nfds;
    struct sockaddr_in listen_addr, service_addr, client_addr;
    struct pollfd *fds;

    s_args args;
    if (parse_server_arguments(argc, argv, &args) != 0)
    {
        exit(EXIT_FAILURE);
    }

    fds = calloc(2 + args.max_clients, sizeof (struct pollfd));
    if (fds == NULL)
    {
        printf("Failed to allocate memory for the fds.\n");
        exit(EXIT_FAILURE);
    }

    //Create socket to listen for our external service
    service_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (service_listen_fd < 0)
    {
        perror("socket(1) failed");
        exit(EXIT_FAILURE);
    }

    //Allow reuse
    ret_val = setsockopt(service_listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (ret_val < 0)
    {
        perror("setsockopt(1) failed");
        close(service_listen_fd);
        exit(EXIT_FAILURE);
    }

    bzero(&listen_addr, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(args.service_port);

    //Bind
    ret_val = bind(service_listen_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr));
    if (ret_val < 0)
    {
        perror("bind(1) failed");
        close(service_listen_fd);
        exit(EXIT_FAILURE);
    }

    //Listen for service
    ret_val = listen(service_listen_fd, 1);
    if (ret_val < 0)
    {
        perror("listen(1) failed");
        close(service_listen_fd);
        exit(EXIT_FAILURE);
    }

    printf("Listening for service on %s:%d.\n", inet_ntoa(listen_addr.sin_addr), args.service_port);

    while (1)
    {
        len = sizeof(service_addr);
        service_fd = accept(service_listen_fd, (struct sockaddr *)&service_addr, &len);
        if (service_fd < 0)
        {
            perror("accept(1) failed");
            close(service_listen_fd);
            close(service_fd);
            exit(EXIT_FAILURE);
        }

        printf("Accepted new connection.\n");

        cnt = 0;
        fds[0].fd = service_fd;
        fds[0].events = POLLIN;
        while (cnt < SHA256_BLOCK_SIZE)
        {
            ret_val = poll(fds, 1, 100);
            if (ret_val <= 0)
            {
                break;
            }
            ret_val = read(service_fd, &recv_buffer[cnt], SHA256_BLOCK_SIZE);
            if (ret_val < 0) {
                break;
            } else {
                cnt += ret_val;
            }
        }
        if (cnt == SHA256_BLOCK_SIZE)
        {
            if(!memcmp(recv_buffer, args.password_hash, SHA256_BLOCK_SIZE))
            {
                break;
            } else {
                printf("Invalid password from %s.\n", inet_ntoa(service_addr.sin_addr));
                close(service_fd);
            }
        } else {
            printf("Invalid connection from %s.\n", inet_ntoa(service_addr.sin_addr));
            close(service_fd);
        }
    }

    //Dictate max clients to service
    printf("Got connection from service on %s.\n", inet_ntoa(service_addr.sin_addr));
    close(service_listen_fd);
    recv_buffer[0] = args.max_clients;
    send(service_fd, recv_buffer, 1, 0);

    printf("Listening on %s:%d.\n", inet_ntoa(listen_addr.sin_addr), args.clients_port);

    clients_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (clients_listen_fd < 0)
    {
        perror("socket(2) failed");
        exit(EXIT_FAILURE);
    }

    ret_val = setsockopt(clients_listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (ret_val < 0)
    {
        perror("setsockopt(2) failed");
        close(service_fd);
        close(clients_listen_fd);
        exit(EXIT_FAILURE);
    }

    ret_val = ioctl(clients_listen_fd, FIONBIO, &on);
    if (ret_val < 0)
    {
        perror("ioctl(1) failed");
        close(service_fd);
        close(clients_listen_fd);
        exit(EXIT_FAILURE);
    }

    ret_val = ioctl(service_fd, FIONBIO, &on);
    if (ret_val < 0)
    {
        perror("ioctl(1) failed");
        close(service_fd);
        close(clients_listen_fd);
        exit(EXIT_FAILURE);
    }

    listen_addr.sin_port = htons(args.clients_port);

    ret_val = bind(clients_listen_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr));
    if (ret_val < 0)
    {
        perror("bind(1) failed");
        close(service_fd);
        close(clients_listen_fd);
        exit(EXIT_FAILURE);
    }

    ret_val = listen(clients_listen_fd, 32);
    if (ret_val < 0)
    {
        perror("listen(2) failed");
        close(service_fd);
        close(clients_listen_fd);
        exit(EXIT_FAILURE);
    }

    memset(fds, 0 , sizeof(*fds));

    fds[0].fd = service_fd;
    fds[0].events = POLLIN;
    fds[1].fd = clients_listen_fd;
    fds[1].events = POLLIN;

    while (!end_server)
    {
        /*
        printf("State:\n");
        for (i = 0; i < 2+args.max_clients; i++)
        {
            j = fds[i].fd;
            if (j > 0)
            {
                printf("%d: %d\n", i, j);
            }
        }
        printf("#FDS %d\n", nfds);
        printf("Waiting on poll()...\n");
        */
        if (nfds < 1)
        {
            printf("Bad #fds\n");
            exit(EXIT_FAILURE);
        }

        ret_val = poll(fds, 2+args.max_clients, -1);
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
        for (i = 0; i < 2 + args.max_clients; i++)
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
                if (fds[i].fd != service_fd && fds[i].fd != clients_listen_fd)
                {
                    if (fds[i].fd > 0)
                    {
                        close(fds[i].fd);
                        fds[i].fd = -1;
                        nfds--;
                        printf("nfds--1\n");
                        preamble[0] = (uint8_t)(i-2);
                        preamble[1] = 1;
                        preamble[2] = 0;
                        preamble[3] = 0;
                        ret_val = send(service_fd, preamble, 4, 0);
                    }
                }
                else
                {
                    end_server = 1;
                }
                break;
            }
            else if (fds[i].fd == clients_listen_fd)
            {
                cnt++;
                do
                {
                    len = sizeof(client_addr);
                    new_fd = accept(clients_listen_fd, (struct sockaddr *)&client_addr, &len);
                    if (new_fd < 0)
                    {
                        if (errno != EWOULDBLOCK)
                        {
                            perror("accept() failed");
                        }
                        break;
                    }
                    printf("Got connection from client on %s:%d (%d).\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), new_fd);
                    if (nfds > 2 + args.max_clients)
                    {
                        printf("But we have the max number of clients, so we closed them!\n");
                        close(new_fd);
                    }
                    else
                    {
                        ret_val = ioctl(new_fd, FIONBIO, &on);
                        if (ret_val < 0)
                        {
                            continue;
                        }
                        for (j = 0; j < 2 + args.max_clients; j++)
                        {
                            if (fds[j].fd <= 0)
                            {
                                fds[j].fd = new_fd;
                                fds[j].events = POLLIN;
                                nfds++;
                                break;
                            }
                        }
                    }

                } while (new_fd != -1);
            }
            else if (fds[i].fd == service_fd)
            {
                cnt++;
                close_connection = 0;
                do
                {
                    if (!main_wanted)
                    {
                        ret_val = recv(service_fd, preamble, 4, 0);
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

                        if (ret_val < 4)
                        {
                            printf("Bad preamble size %d\n", ret_val);
                            close_connection = 1;
                            break;
                        }
                        conn_index = preamble[0];
                        memcpy(&temp_16, &preamble[2], 2);
                        main_wanted = ntohs(temp_16);

                        if (main_wanted > MAX_RECV)
                        {
                            printf("Invalid main wanted\n");
                            close_connection = 1;
                            break;
                        }

                        if (preamble[1])
                        {
                            main_size = 0;
                            main_wanted = 0;
                            if (fds[preamble[0] + 2].fd > 0)
                            {
                                printf("Closing connection %d\n", preamble[0]);
                                close(fds[preamble[0] + 2].fd);
                                fds[preamble[0] + 2].fd = -1;
                                nfds--;
                                printf("nfds--2\n");
                            }
                        }
                    }
                    else
                    {
                        if (!(main_wanted - main_size))
                        {
                            ret_val = recv(service_fd, &main_cache[main_size], main_wanted - main_size, 0);
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
                                printf("Connection closed (2)\n");
                                close_connection = 1;
                                break;
                            }

                            if (ret_val + main_size > MAX_RECV + 4)
                            {
                                printf("Invalid size %d\n", ret_val + main_size);
                                close_connection = 1;
                                break;
                            }

                            main_size += ret_val;
                        }
                        //printf("Received:\n");
                        //print_bytes(main_cache, main_size);

                        if (main_size == main_wanted)
                        {

                            j = fds[2+conn_index].fd;
                            if (j <= 0)
                            {
                                printf("Invalid client (%d)\n", conn_index);
                                main_cache[1] = 1;
                                main_cache[2] = 0;
                                main_cache[3] = 0;
                                ret_val = send(service_fd, main_cache, 4, 0);
                                if (ret_val < 0)
                                {
                                    perror("send() failed");
                                }
                                break;
                            }
                            ret_val = send(j, main_cache, main_size, 0);
                            //printf("Sent\n");
                            //print_bytes(main_cache, main_size);

                            if (ret_val < 0)
                            {
                                perror("send() failed");
                                main_cache[1] = 1;
                                main_cache[2] = 0;
                                main_cache[3] = 0;
                                ret_val = send(service_fd, main_cache, 4, 0);
                                close(j);
                                fds[2+conn_index].fd = -1;
                                nfds--;
                                printf("nfds--3\n");
                                if (ret_val < 0)
                                {
                                    perror("send() failed");
                                    close_connection = 1;
                                    break;
                                }
                            }
                            main_size = 0;
                            main_wanted = 0;
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
                        printf("Connection closed (to client)\n");
                        close_connection = 2;
                        break;
                    }

                    //printf("Recieved:\n");
                    //print_bytes(recv_buffer, ret_val);

                    preamble[0] = (uint8_t)(i-2);
                    preamble[1] = 0;
                    len = ret_val;
                    temp_16 = htons((uint16_t)ret_val);
                    memcpy(&preamble[2], &temp_16, 2);
                    memcpy(send_buffer, preamble, 4);
                    memcpy(&send_buffer[4], recv_buffer, len);

                    ret_val = send(service_fd, send_buffer, len + 4, 0);
                    //printf("Sent with size %d:\n", len);
                    //print_bytes(send_buffer, len + 4);

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
                        //printf("nfds--4\n");
                        preamble[0] = (uint8_t)(i-2);
                        preamble[1] = 1;
                        preamble[2] = 0;
                        preamble[3] = 0;
                        ret_val = send(service_fd, preamble, 4, 0);
                    }
                }
            }
        }
    }
    for (i = 0; i < 2 + args.max_clients; i++)
    {
        j = fds[i].fd;
        if (j > 0)
        {
            close(j);
        }
    }
}