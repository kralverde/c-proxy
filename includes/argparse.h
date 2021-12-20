#ifndef __ARGPARSE__H_
#define __ARGPARSE__H_

#include <argp.h>
#include <stdint.h>
#include <netinet/in.h>

#include "sha256.h"

typedef struct s_args {
    uint16_t service_port;
    uint16_t clients_port;
    BYTE password_hash[SHA256_BLOCK_SIZE];
    uint8_t max_clients;
} s_args;

typedef struct c_args {
    struct sockaddr_in proxy_addr;
    struct sockaddr_in service_addr;
    BYTE password_hash[SHA256_BLOCK_SIZE];
} c_args;

uint8_t parse_server_arguments(int, char**, s_args*);
uint8_t parse_client_arguments(int, char**, c_args*);

#endif //__ARGPARSE_H