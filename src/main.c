#include "main.h"
#include <sys/stat.h>

static ftp_server_t server;
static volatile int running = 1;

void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    running = 0;
    ftp_stop(&server);
    exit(0);
}

void print_usage(const char *program_name) {
    printf("Usage: %s [-p port] [-d directory] [-a]\n", program_name);
    printf("Options:\n");
    printf("  -p port       Port to listen on (default: %d)\n", DEFAULT_PORT);
    printf("  -d directory  Root directory for FTP server (default: %s)\n", DEFAULT_ROOT_DIR);
    printf("  -a            Enable anonymous login\n");
}

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    char root_dir[MAX_PATH_LEN] = DEFAULT_ROOT_DIR;
    int anonymous = 0;
    int opt;
    
    /* Parse command line arguments */
    while ((opt = getopt(argc, argv, "p:d:a")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'd':
                strncpy(root_dir, optarg, MAX_PATH_LEN - 1);
                root_dir[MAX_PATH_LEN - 1] = '\0';
                break;
            case 'a':
                anonymous = 1;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    /* Create root directory if it doesn't exist */
    struct stat st = {0};
    if (stat(root_dir, &st) == -1) {
        printf("Creating root directory: %s\n", root_dir);
        if (mkdir(root_dir, 0755) == -1) {
            perror("Error creating root directory");
            return 1;
        }
    }
    
    /* Set up signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("Starting FTP server on port %d\n", port);
    printf("Root directory: %s\n", root_dir);
    if (anonymous) {
        printf("Anonymous login enabled\n");
    }
    
    /* Initialize and start the server */
    if (!ftp_init(&server, port, root_dir, anonymous)) {
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }
    
    /* Run the server */
    ftp_start(&server);
    
    /* Clean up */
    ftp_stop(&server);
    
    return 0;
}
