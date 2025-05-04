#ifndef FTP_H
#define FTP_H

#include <stdint.h>
#include <stdbool.h>

#define FTP_PORT 21
#define FTP_DATA_PORT 20
#define MAX_BUFFER_SIZE 8192
#define MAX_CLIENTS 10
#define MAX_USERNAME_LEN 32
#define MAX_PASSWORD_LEN 32
#define MAX_PATH_LEN 1024

/* FTP Response Codes */
#define FTP_READY_FOR_NEW_USER 220
#define FTP_USER_LOGGED_IN 230
#define FTP_USER_NAME_OKAY 331
#define FTP_FILE_ACTION_COMPLETED 250
#define FTP_FILE_TRANSFER_OK 226
#define FTP_OPENING_DATA_CONNECTION 150
#define FTP_CLOSING_DATA_CONNECTION 226
#define FTP_COMMAND_NOT_IMPLEMENTED 502
#define FTP_FILE_NOT_FOUND 550
#define FTP_LOCAL_ERROR 451

/* FTP Server Context */
typedef struct {
    int control_socket;
    int data_socket;
    int port;
    char root_dir[MAX_PATH_LEN];
    char current_dir[MAX_PATH_LEN];
    bool anonymous_enabled;
    bool passive_mode;
    int passive_port;
} ftp_server_t;

/* Client Session */
typedef struct {
    int control_fd;
    int data_fd;
    bool logged_in;
    char username[MAX_USERNAME_LEN];
    char current_dir[MAX_PATH_LEN];
    bool passive_mode;
    int passive_port;
} ftp_client_t;

/* Initialize FTP server with given port and root directory */
bool ftp_init(ftp_server_t *server, int port, const char *root_dir, bool anonymous);

/* Start the FTP server */
bool ftp_start(ftp_server_t *server);

/* Stop the FTP server */
void ftp_stop(ftp_server_t *server);

/* Process FTP command */
bool ftp_process_command(ftp_client_t *client, const char *command);

/* Handle new client connection */
ftp_client_t *ftp_accept_client(ftp_server_t *server);

/* Close client connection */
void ftp_close_client(ftp_client_t *client);

/* Send response to client */
bool ftp_send_response(ftp_client_t *client, int code, const char *message);

/* Open data connection */
bool ftp_open_data_connection(ftp_client_t *client);

/* Close data connection */
void ftp_close_data_connection(ftp_client_t *client);

#endif /* FTP_H */
