#include <strings.h>
#include <string.h>
#include <stdlib.h>

#include <arpa/inet.h>

#include "argparse.h"

static int parse_opt_server (int key, char *arg, struct argp_state *state)
{
    s_args *server_args = state->input;
    switch (key)
    {
        case 1:
        {
            int ret = strtol(arg, NULL, 10);
            if (ret <= 1024 || ret >= UINT16_MAX)
                argp_failure(state, 1, 0, "Invalid port for port_service");
            server_args->service_port = ret;
            break;
        }
        case 2:
        {
            int ret = strtol(arg, NULL, 10);
            if (ret <= 1024 || ret >= UINT16_MAX)
                argp_failure(state, 1, 0, "Invalid port for port_incoming");
            server_args->clients_port = ret;
            break;
        }
        case 3:
        {
            SHA256_CTX ctx;
            sha256_init(&ctx);
            sha256_update(&ctx, (unsigned char *)arg, strlen(arg));
            sha256_final(&ctx, server_args->password_hash);
            break;
        }
        case 4:
        {
            server_args->max_clients = UINT8_MAX;
            if (arg)
            {
                int ret = strtol(arg, NULL, 10);
                if (ret < 1 || ret > UINT8_MAX)
                    argp_failure(state, 1, 0, "Invalid max clients");
                server_args->max_clients = ret;
            }
            break;
        }
    }
    return 0;
}

static int parse_opt_client (int key, char *arg, struct argp_state *state)
{
    c_args *client_args = state->input;
    switch (key)
    {
        case 1:
        {
            int ret = strtol(arg, NULL, 10);
            if (ret <= 1024 || ret >= UINT16_MAX)
                argp_failure(state, 1, 0, "Invalid port for proxy");
            client_args->proxy_addr.sin_port = htons(ret);
            break;
        }
        case 2:
        {
            int ret = inet_pton(AF_INET, arg, &(client_args->proxy_addr.sin_addr.s_addr));
            if (ret <= 0)
            {
                argp_failure(state, 1, 0, "Invalid IP address for proxy");
            }
            break;
        }
        case 3:
        {
            SHA256_CTX ctx;
            sha256_init(&ctx);
            sha256_update(&ctx, (unsigned char *)arg, strlen(arg));
            sha256_final(&ctx, client_args->password_hash);
            break;
        }
        case 4:
        {
            int ret = strtol(arg, NULL, 10);
            if (ret <= 1024 || ret >= UINT16_MAX)
                argp_failure(state, 1, 0, "Invalid port for service");
            client_args->service_addr.sin_port = htons(ret);
            break;
        }
        case 5:
        {
            int ret = inet_pton(AF_INET, arg, &(client_args->service_addr.sin_addr.s_addr));
            if (ret <= 0)
            {
                argp_failure(state, 1, 0, "Invalid IP address for service");
            }
            break;
        }
    }
    return 0;
}

uint8_t parse_server_arguments(int argc, char** argv, s_args* server_args)
{
    
    bzero(server_args, sizeof(s_args));
    server_args->max_clients = UINT8_MAX;

    struct argp_option options[] = {
        { "port_service", 1, "number", 0, "What port to listen for the service on.", 0 },
        { "port_incoming", 2, "number", 0, "What port to listen for clients on.", 0 },
        { "password", 3, "string", 0, "Used to verify the incoming service.", 0 },
        { "clients", 4, "number", OPTION_ARG_OPTIONAL, "The maximum number of clients to maintain", 0 },
        { 0 }
    };
    
    struct argp argp = { options, parse_opt_server, 0, 0, 0, 0, 0 };
    
    if (argp_parse(&argp, argc, argv, 0, 0, server_args))
    {
        printf("Failed to parse arguments.\n");
        return 1;
    }

    if (!server_args->clients_port)
    {
        printf("No port_incoming set.\n");
        return 1;
    }

    if (!server_args->service_port)
    {
        printf("No port_service set.\n");
        return 1;
    }

    if (!server_args->password_hash)
    {
        printf("No password set.\n");
        return 1;
    }

    return 0;
}

uint8_t parse_client_arguments(int argc, char** argv, c_args* client_args)
{
    
    bzero(client_args, sizeof(s_args));
    client_args->proxy_addr.sin_family = AF_INET;
    client_args->service_addr.sin_family = AF_INET;

    struct argp_option options[] = {
        { "port_proxy", 1, "number", 0, "The port of the proxy.", 0 },
        { "ip_proxy", 2, "string", 0, "The IP of the proxy.", 0 },
        { "password", 3, "string", 0, "Used to verify the incoming service.", 0 },
        { "port_service", 4, "number", 0, "The port of the service.", 0 },
        { "ip_service", 5, "string", 0, "The IP of the service.", 0 },
        { 0 }
    };
    
    struct argp argp = { options, parse_opt_client, 0, 0, 0, 0, 0 };
    
    if (argp_parse(&argp, argc, argv, 0, 0, client_args))
    {
        printf("Failed to parse arguments.\n");
        return 1;
    }

    if (!client_args->proxy_addr.sin_addr.s_addr)
    {
        printf("No proxy ip set.\n");
        return 1;
    }

    if (!client_args->proxy_addr.sin_port)
    {
        printf("No proxy port set.\n");
        return 1;
    }

    if (!client_args->service_addr.sin_addr.s_addr)
    {
        printf("No service ip set.\n");
        return 1;
    }

    if (!client_args->service_addr.sin_port)
    {
        printf("No service port set.\n");
        return 1;
    }

    if (!client_args->password_hash)
    {
        printf("No password set.\n");
        return 1;
    }

    return 0;
}