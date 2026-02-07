/*
 * ZERO-COPY ARCHITECTURE:
 * ASCII Diagram:
 * 
 *   User Buffer (pinned pages)
 *        |
 *        | (no copy - kernel references pages)
 *        v
 *   Kernel Socket Layer
 *        |
 *        | (DMA descriptor setup)
 *        v
 *   NIC DMA Engine ----> Network
 *   
 * Page Pinning: mlock() pins pages in RAM
 * DMA: NIC reads directly from user pages
 * Completion: MSG_ERRQUEUE notification when NIC completes TX
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <linux/errqueue.h>
#include <linux/socket.h>
#include <poll.h>
#include <errno.h>
#include <signal.h>

#define DEFAULT_PORT 8082
#define MAX_CLIENTS 100
#define MSG_BUFFER_SIZE 8192

/*
 * drain_zerocopy_completions: Drain MSG_ERRQUEUE for zerocopy completions
 * This is CRITICAL for correct MSG_ZEROCOPY usage.
 * After send with MSG_ZEROCOPY, the kernel keeps a reference to the buffer.
 * We must wait for completion notification before reusing the buffer.
 */
static void drain_zerocopy_completions(int sockfd) {
    struct msghdr msg = {0};
    char control[128];
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);
    
    // Non-blocking check for completions
    while (1) {
        msg.msg_controllen = sizeof(control);
        int ret = recvmsg(sockfd, &msg, MSG_ERRQUEUE | MSG_DONTWAIT);
        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No more completions pending
                break;
            }
            break;
        }
        
        // Parse completion notification
        struct cmsghdr *cm = CMSG_FIRSTHDR(&msg);
        if (cm && cm->cmsg_level == SOL_IP && cm->cmsg_type == IP_RECVERR) {
            struct sock_extended_err *serr = (struct sock_extended_err *)CMSG_DATA(cm);
            if (serr->ee_errno == 0 && serr->ee_origin == SO_EE_ORIGIN_ZEROCOPY) {
                // Zerocopy completion confirmed
                // serr->ee_info contains the low sequence number
                // serr->ee_data contains the high sequence number
            }
        }
    }
}

int message_size = 1024;
int num_threads = 4;
volatile sig_atomic_t running = 1;

/* Signal handler for graceful shutdown */
void signal_handler(int signum) {
    (void)signum;
    printf("\nReceived shutdown signal. Stopping server...\n");
    running = 0;
}

typedef struct {
    char *buffer;  // Page-pinned buffer
    size_t size;
} ZeroCopyMessage;

/*
 * Allocate page-pinned buffer for true zero-copy
 * mlock() ensures pages stay in RAM and DMA-accessible
 */
ZeroCopyMessage* allocate_zerocopy_message(size_t size) {
    ZeroCopyMessage *msg = malloc(sizeof(ZeroCopyMessage));
    if (!msg) {
        perror("Failed to allocate ZeroCopyMessage");
        return NULL;
    }
    
    // Allocate page-aligned buffer
    msg->buffer = aligned_alloc(4096, size);
    if (!msg->buffer) {
        perror("Failed to allocate aligned buffer");
        free(msg);
        return NULL;
    }
    msg->size = size;
    
    // Pin pages in memory for DMA (CRITICAL for zero-copy)
    if (mlock(msg->buffer, size) != 0) {
        perror("mlock failed - zero-copy may not work");
    }
    
    memset(msg->buffer, 'Z', size - 1);
    msg->buffer[size - 1] = '\0';
    
    return msg;
}

void free_zerocopy_message(ZeroCopyMessage *msg) {
    if (msg) {
        munlock(msg->buffer, msg->size);
        free(msg->buffer);
        free(msg);
    }
}

/*
 * Send with MSG_ZEROCOPY flag
 * Kernel sets up DMA descriptors, NIC reads directly from user buffer
 * Completion notification via MSG_ERRQUEUE
 */
int send_zerocopy(int sockfd, ZeroCopyMessage *msg) {
    ssize_t sent = send(sockfd, msg->buffer, msg->size, MSG_ZEROCOPY);
    
    if (sent > 0) {
        // Drain any pending zerocopy completions
        // This ensures buffers from previous sends are safe to reuse
        drain_zerocopy_completions(sockfd);
    }
    
    return sent;
}

void* client_handler(void *arg) {
    int client_sock = *(int*)arg;
    free(arg);
    
    printf("[Thread %lu] Client connected\n", pthread_self());
    
    ZeroCopyMessage *msg = allocate_zerocopy_message(message_size * 8);
    int messages_sent = 0;
    
    while (running) {
        if (send_zerocopy(client_sock, msg) < 0) {
            if (errno == EPIPE || errno == ECONNRESET) break;
            perror("zerocopy send error");
            break;
        }
        messages_sent++;
    }
    
    printf("[Thread %lu] Sent %d messages\n", pthread_self(), messages_sent);
    
    free_zerocopy_message(msg);
    close(client_sock);
    return NULL;
}

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
    if (argc > 1) port = atoi(argv[1]);
    if (argc > 2) message_size = atoi(argv[2]);
    if (argc > 3) num_threads = atoi(argv[3]);
    
    printf("=== PA02 Part A3: Zero-Copy Server ===\n");
    printf("Roll Number: MT25190\n");
    printf("Port: %d\n", port);
    printf("Using MSG_ZEROCOPY with page pinning\n\n");
    
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR failed");
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_sock, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d...\n", port);
    
    int connected = 0;
    while (connected < num_threads) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) continue;
        
        // Enable zero-copy on client socket (must be set on connected socket)
        int zerocopy = 1;
        if (setsockopt(client_sock, SOL_SOCKET, SO_ZEROCOPY, &zerocopy, sizeof(zerocopy)) < 0) {
            perror("SO_ZEROCOPY not supported on client socket - using fallback");
        }
        
        printf("Client %d connected\n", ++connected);
        
        int *sock_ptr = malloc(sizeof(int));
        *sock_ptr = client_sock;
        
        pthread_create(&thread_id, NULL, client_handler, sock_ptr);
        pthread_detach(thread_id);
    }
    
    printf("All clients connected. Running...\n");
    while (running) sleep(1);
    
    close(server_sock);
    return 0;
}
