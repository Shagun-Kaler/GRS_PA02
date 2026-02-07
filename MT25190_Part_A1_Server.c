/*
 * TCP server using send()/recv() - demonstrates TWO-COPY model:
 * Copy 1: User space → Kernel space
 * Copy 2: Kernel space → NIC
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

#define DEFAULT_PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 8192

/* Message structure with 8 dynamically allocated string fields */
typedef struct {
    char *field1;  // Each field will be allocated via malloc()
    char *field2;
    char *field3;
    char *field4;
    char *field5;
    char *field6;
    char *field7;
    char *field8;
} Message;

/* Global configuration */
int message_size = 1024;        // Size of each message field
int num_threads = 4;            // Number of client threads to expect
volatile sig_atomic_t running = 1;  // Server running flag (sig_atomic_t for signal safety)

/* Signal handler for graceful shutdown */
void signal_handler(int signum) {
    (void)signum;  // Suppress unused warning
    printf("\nReceived shutdown signal. Stopping server...\n");
    running = 0;
}

/*
 * allocate_message: Dynamically allocates message fields
 * Each field is allocated using malloc() to demonstrate
 * user-space memory allocation before sending
 */
Message* allocate_message(int field_size) {
    Message *msg = (Message*)malloc(sizeof(Message));
    if (!msg) {
        perror("Failed to allocate message structure");
        return NULL;
    }
    
    // Allocate each field separately using malloc()
    msg->field1 = (char*)malloc(field_size);
    msg->field2 = (char*)malloc(field_size);
    msg->field3 = (char*)malloc(field_size);
    msg->field4 = (char*)malloc(field_size);
    msg->field5 = (char*)malloc(field_size);
    msg->field6 = (char*)malloc(field_size);
    msg->field7 = (char*)malloc(field_size);
    msg->field8 = (char*)malloc(field_size);
    
    // Check if all allocations succeeded
    if (!msg->field1 || !msg->field2 || !msg->field3 || !msg->field4 ||
        !msg->field5 || !msg->field6 || !msg->field7 || !msg->field8) {
        perror("Failed to allocate message fields");
        // Cleanup any successfully allocated fields
        free(msg->field1);
        free(msg->field2);
        free(msg->field3);
        free(msg->field4);
        free(msg->field5);
        free(msg->field6);
        free(msg->field7);
        free(msg->field8);
        free(msg);
        return NULL;
    }
    
    // Initialize with sample data
    memset(msg->field1, 'A', field_size - 1);
    memset(msg->field2, 'B', field_size - 1);
    memset(msg->field3, 'C', field_size - 1);
    memset(msg->field4, 'D', field_size - 1);
    memset(msg->field5, 'E', field_size - 1);
    memset(msg->field6, 'F', field_size - 1);
    memset(msg->field7, 'G', field_size - 1);
    memset(msg->field8, 'H', field_size - 1);
    
    // Null-terminate each field
    msg->field1[field_size - 1] = '\0';
    msg->field2[field_size - 1] = '\0';
    msg->field3[field_size - 1] = '\0';
    msg->field4[field_size - 1] = '\0';
    msg->field5[field_size - 1] = '\0';
    msg->field6[field_size - 1] = '\0';
    msg->field7[field_size - 1] = '\0';
    msg->field8[field_size - 1] = '\0';
    
    return msg;
}

/*
 * free_message: Deallocates all message fields
 */
void free_message(Message *msg) {
    if (msg) {
        free(msg->field1);
        free(msg->field2);
        free(msg->field3);
        free(msg->field4);
        free(msg->field5);
        free(msg->field6);
        free(msg->field7);
        free(msg->field8);
        free(msg);
    }
}

/*
 * send_message_twocopy: Sends message using TWO-COPY model
 * 
 * TWO-COPY ARCHITECTURE:
 * 1. COPY 1: User → Kernel
 *    - send() copies data from user buffer (msg->field*) to kernel socket buffer
 *    - Kernel allocates sk_buff structure
 *    - Data is copied into kernel memory space
 * 
 * 2. COPY 2: Kernel → NIC
 *    - DMA controller copies data from kernel buffer to NIC transmit ring
 *    - Network card buffers data before transmission
 */
int send_message_twocopy(int sockfd, Message *msg, int field_size) {
    ssize_t total_sent = 0;
    ssize_t bytes_sent;
    
    // Send each field separately - each send() triggers COPY 1 (User → Kernel)
    // Note: Each send() call results in:
    //   - Syscall transition (user mode → kernel mode)
    //   - Memory copy from user space to kernel socket buffer
    //   - Eventual DMA transfer to NIC (COPY 2: Kernel → NIC)
    // FIX: Send full field_size bytes (not strlen) to match client expectation
    
    bytes_sent = send(sockfd, msg->field1, field_size, 0);  // USER → KERNEL copy
    if (bytes_sent < 0) return -1;
    total_sent += bytes_sent;
    
    bytes_sent = send(sockfd, msg->field2, field_size, 0);  // USER → KERNEL copy
    if (bytes_sent < 0) return -1;
    total_sent += bytes_sent;
    
    bytes_sent = send(sockfd, msg->field3, field_size, 0);  // USER → KERNEL copy
    if (bytes_sent < 0) return -1;
    total_sent += bytes_sent;
    
    bytes_sent = send(sockfd, msg->field4, field_size, 0);  // USER → KERNEL copy
    if (bytes_sent < 0) return -1;
    total_sent += bytes_sent;
    
    bytes_sent = send(sockfd, msg->field5, field_size, 0);  // USER → KERNEL copy
    if (bytes_sent < 0) return -1;
    total_sent += bytes_sent;
    
    bytes_sent = send(sockfd, msg->field6, field_size, 0);  // USER → KERNEL copy
    if (bytes_sent < 0) return -1;
    total_sent += bytes_sent;
    
    bytes_sent = send(sockfd, msg->field7, field_size, 0);  // USER → KERNEL copy
    if (bytes_sent < 0) return -1;
    total_sent += bytes_sent;
    
    bytes_sent = send(sockfd, msg->field8, field_size, 0);  // USER → KERNEL copy
    if (bytes_sent < 0) return -1;
    total_sent += bytes_sent;
    
    return total_sent;
}

/*
 * client_handler: Thread function to handle each client connection
 * Each client is handled by a separate pthread
 */
void* client_handler(void *arg) {
    int client_sock = *(int*)arg;
    free(arg);  // Free the allocated socket descriptor
    
    printf("[Thread %lu] Client connected\n", pthread_self());
    
    // Allocate message structure
    Message *msg = allocate_message(message_size);
    if (!msg) {
        close(client_sock);
        return NULL;
    }
    
    // Send messages repeatedly until connection closes or error
    int messages_sent = 0;
    while (running) {
        int result = send_message_twocopy(client_sock, msg, message_size);
        if (result < 0) {
            if (errno == EPIPE || errno == ECONNRESET) {
                printf("[Thread %lu] Client disconnected\n", pthread_self());
                break;
            }
            perror("send error");
            break;
        }
        
        messages_sent++;
    }
    
    printf("[Thread %lu] Total messages sent: %d\n", pthread_self(), messages_sent);
    
    // Cleanup
    free_message(msg);
    close(client_sock);
    
    return NULL;
}

/*
 * Main server function
 */
int main(int argc, char *argv[]) {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t thread_id;
    
    // Register signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Parse command line arguments: <port> <message_size> <num_threads>
    // PA02 requirement: Port must be passed explicitly for automation
    int port = DEFAULT_PORT;
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    if (argc > 2) {
        message_size = atoi(argv[2]);
    }
    if (argc > 3) {
        num_threads = atoi(argv[3]);
    }
    
    printf("=== PA02 Part A1: Two-Copy Server ===\n");
    printf("Roll Number: MT25190\n");
    printf("Port: %d\n", port);
    printf("Message size: %d bytes per field\n", message_size);
    printf("Expected threads: %d\n\n", num_threads);
    
    // Create TCP socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    
    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // Bind socket to address
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen for connections
    if (listen(server_sock, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d...\n", port);
    printf("Waiting for %d client connections...\n\n", num_threads);
    
    // Accept client connections and spawn threads
    int connected_clients = 0;
    while (connected_clients < num_threads) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }
        
        printf("Accepted connection %d from %s:%d\n", 
               connected_clients + 1,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        
        // Allocate memory for socket descriptor to pass to thread
        int *sock_ptr = (int*)malloc(sizeof(int));
        *sock_ptr = client_sock;
        
        // Create thread to handle client
        if (pthread_create(&thread_id, NULL, client_handler, sock_ptr) != 0) {
            perror("Thread creation failed");
            free(sock_ptr);
            close(client_sock);
            continue;
        }
        
        // Detach thread so resources are freed automatically
        pthread_detach(thread_id);
        
        connected_clients++;
    }
    
    printf("\nAll %d clients connected. Press Ctrl+C to stop.\n", num_threads);
    
    // Keep server running
    while (running) {
        sleep(1);
    }
    
    close(server_sock);
    return 0;
}
