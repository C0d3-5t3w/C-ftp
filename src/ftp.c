#include "ftp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX_CMD_SIZE 512

/* Simple user authentication - for a production system, use secure storage */
static bool authenticate_user(const char *username, const char *password, bool allow_anonymous) {
    if (allow_anonymous && strcmp(username, "anonymous") == 0) {
        return true;
    }
    
    /* Replace with actual authentication */
    if (strcmp(username, "admin") == 0 && strcmp(password, "password") == 0) {
        return true;
    }
    
    return false;
}

/* Client handler thread */
static void *client_handler(void *arg) {
    ftp_client_t *client = (ftp_client_t *)arg;
    char buffer[MAX_BUFFER_SIZE];
    
    /* Send welcome message */
    ftp_send_response(client, FTP_READY_FOR_NEW_USER, "Welcome to Simple FTP Server");
    
    while (1) {
        ssize_t n = read(client->control_fd, buffer, MAX_BUFFER_SIZE - 1);
        if (n <= 0) {
            break; /* Connection closed or error */
        }
        
        buffer[n] = '\0';
        /* Remove newline characters */
        char *newline = strchr(buffer, '\r');
        if (newline) *newline = '\0';
        newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
        
        printf("Received command: %s\n", buffer);
        
        if (!ftp_process_command(client, buffer)) {
            break;
        }
    }
    
    ftp_close_client(client);
    free(client);
    return NULL;
}

bool ftp_init(ftp_server_t *server, int port, const char *root_dir, bool anonymous) {
    server->port = port;
    strncpy(server->root_dir, root_dir, MAX_PATH_LEN - 1);
    server->root_dir[MAX_PATH_LEN - 1] = '\0';
    strncpy(server->current_dir, root_dir, MAX_PATH_LEN - 1);
    server->current_dir[MAX_PATH_LEN - 1] = '\0';
    server->anonymous_enabled = anonymous;
    server->passive_mode = false;
    server->passive_port = FTP_DATA_PORT;
    
    /* Create control socket */
    server->control_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->control_socket < 0) {
        perror("Error creating socket");
        return false;
    }
    
    /* Set socket options */
    int opt = 1;
    if (setsockopt(server->control_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Error setting socket options");
        close(server->control_socket);
        return false;
    }
    
    /* Bind the socket */
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server->port);
    
    if (bind(server->control_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        close(server->control_socket);
        return false;
    }
    
    /* Listen for connections */
    if (listen(server->control_socket, MAX_CLIENTS) < 0) {
        perror("Error listening on socket");
        close(server->control_socket);
        return false;
    }
    
    return true;
}

bool ftp_start(ftp_server_t *server) {
    printf("FTP server started on port %d\n", server->port);
    printf("Root directory: %s\n", server->root_dir);
    
    while (1) {
        ftp_client_t *client = ftp_accept_client(server);
        if (client) {
            pthread_t thread_id;
            if (pthread_create(&thread_id, NULL, client_handler, client) != 0) {
                perror("Error creating thread");
                ftp_close_client(client);
                free(client);
            } else {
                pthread_detach(thread_id);
            }
        }
    }
    
    return true;
}

void ftp_stop(ftp_server_t *server) {
    if (server->control_socket >= 0) {
        close(server->control_socket);
        server->control_socket = -1;
    }
}

ftp_client_t *ftp_accept_client(ftp_server_t *server) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = accept(server->control_socket, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("Error accepting connection");
        return NULL;
    }
    
    printf("New client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    ftp_client_t *client = (ftp_client_t *)malloc(sizeof(ftp_client_t));
    if (!client) {
        close(client_fd);
        return NULL;
    }
    
    client->control_fd = client_fd;
    client->data_fd = -1;
    client->logged_in = false;
    memset(client->username, 0, MAX_USERNAME_LEN);
    strncpy(client->current_dir, server->root_dir, MAX_PATH_LEN - 1);
    client->current_dir[MAX_PATH_LEN - 1] = '\0';
    client->passive_mode = false;
    client->passive_port = FTP_DATA_PORT;
    
    return client;
}

void ftp_close_client(ftp_client_t *client) {
    if (client->control_fd >= 0) {
        close(client->control_fd);
        client->control_fd = -1;
    }
    
    if (client->data_fd >= 0) {
        close(client->data_fd);
        client->data_fd = -1;
    }
}

bool ftp_send_response(ftp_client_t *client, int code, const char *message) {
    char response[MAX_BUFFER_SIZE];
    snprintf(response, sizeof(response), "%d %s\r\n", code, message);
    
    ssize_t n = write(client->control_fd, response, strlen(response));
    return n > 0;
}

bool ftp_open_data_connection(ftp_client_t *client) {
    if (client->passive_mode) {
        /* In passive mode, we've already set up the socket, so just accept */
        return true;
    } else {
        /* Active mode - connect to client's data port */
        /* This is simplified and would need more work for real implementation */
        client->data_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (client->data_fd < 0) {
            perror("Error creating data socket");
            return false;
        }
        
        /* Connect to data port (simplified) */
        struct sockaddr_in data_addr;
        memset(&data_addr, 0, sizeof(data_addr));
        data_addr.sin_family = AF_INET;
        data_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); /* Should use client's IP */
        data_addr.sin_port = htons(client->passive_port);
        
        if (connect(client->data_fd, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0) {
            perror("Error connecting to data port");
            close(client->data_fd);
            client->data_fd = -1;
            return false;
        }
        
        return true;
    }
}

void ftp_close_data_connection(ftp_client_t *client) {
    if (client->data_fd >= 0) {
        close(client->data_fd);
        client->data_fd = -1;
    }
}

bool ftp_process_command(ftp_client_t *client, const char *command) {
    char cmd[MAX_CMD_SIZE];
    char arg[MAX_BUFFER_SIZE];
    
    /* Parse command and argument */
    arg[0] = '\0';
    if (sscanf(command, "%s %[^\n]", cmd, arg) < 1) {
        return ftp_send_response(client, FTP_COMMAND_NOT_IMPLEMENTED, "Command not recognized");
    }
    
    /* Convert command to uppercase */
    for (char *p = cmd; *p; p++) {
        *p = toupper(*p);
    }
    
    /* Handle FTP commands */
    if (strcmp(cmd, "USER") == 0) {
        strncpy(client->username, arg, MAX_USERNAME_LEN - 1);
        client->username[MAX_USERNAME_LEN - 1] = '\0';
        return ftp_send_response(client, FTP_USER_NAME_OKAY, "User name okay, need password");
    }
    else if (strcmp(cmd, "PASS") == 0) {
        if (authenticate_user(client->username, arg, true)) {
            client->logged_in = true;
            return ftp_send_response(client, FTP_USER_LOGGED_IN, "User logged in, proceed");
        } else {
            return ftp_send_response(client, 530, "Login incorrect");
        }
    }
    else if (strcmp(cmd, "QUIT") == 0) {
        ftp_send_response(client, 221, "Goodbye");
        return false; /* End connection */
    }
    else if (!client->logged_in) {
        return ftp_send_response(client, 530, "Not logged in");
    }
    else if (strcmp(cmd, "PWD") == 0) {
        char response[MAX_BUFFER_SIZE];
        snprintf(response, sizeof(response), "\"%s\"", client->current_dir);
        return ftp_send_response(client, 257, response);
    }
    else if (strcmp(cmd, "CWD") == 0) {
        /* Simple CWD implementation - would need more validation in production */
        strncpy(client->current_dir, arg, MAX_PATH_LEN - 1);
        client->current_dir[MAX_PATH_LEN - 1] = '\0';
        return ftp_send_response(client, FTP_FILE_ACTION_COMPLETED, "Directory changed");
    }
    else if (strcmp(cmd, "TYPE") == 0) {
        /* We support ASCII and binary modes */
        return ftp_send_response(client, 200, "Type set to I");
    }
    else if (strcmp(cmd, "PASV") == 0) {
        /* Simplified PASV implementation */
        client->passive_mode = true;
        client->passive_port = 2121; /* Use fixed port for simplicity */
        return ftp_send_response(client, 227, "Entering Passive Mode (127,0,0,1,8,89)");
    }
    else if (strcmp(cmd, "LIST") == 0) {
        /* List directory contents */
        ftp_send_response(client, FTP_OPENING_DATA_CONNECTION, "File status okay; about to open data connection");
        
        if (ftp_open_data_connection(client)) {
            DIR *dir = opendir(client->current_dir);
            if (dir) {
                struct dirent *entry;
                while ((entry = readdir(dir)) != NULL) {
                    char file_info[MAX_BUFFER_SIZE];
                    snprintf(file_info, sizeof(file_info), "%s\r\n", entry->d_name);
                    write(client->data_fd, file_info, strlen(file_info));
                }
                closedir(dir);
                ftp_close_data_connection(client);
                return ftp_send_response(client, FTP_CLOSING_DATA_CONNECTION, "Directory send OK");
            } else {
                ftp_close_data_connection(client);
                return ftp_send_response(client, FTP_LOCAL_ERROR, "Error reading directory");
            }
        } else {
            return ftp_send_response(client, 425, "Can't open data connection");
        }
    }
    else {
        return ftp_send_response(client, FTP_COMMAND_NOT_IMPLEMENTED, "Command not implemented");
    }
    
    return true;
}
