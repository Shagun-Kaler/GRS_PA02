/*
 * TCP server using sendmsg() with iovec - demonstrates ONE-COPY model:
 * ELIMINATED: User space → Kernel space copy (using pre-registered buffers & scatter-gather)
 * REMAINING: Kernel space → NIC (DMA still required)
 * 
 * KEY OPTIMIZATION:
 * - Uses struct iovec for scatter-gather I/O
 * - Buffers are pre-registered and reused
 * - Kernel can directly DMA from these buffers without intermediate copy
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <errno.h>
#include <signal.h>

#define DEFAULT_PORT 8081
#define MAX_CLIENTS 100
#define NUM_FIELDS 8

/* Global configuration */
int message_size = 1024;
int num_threads = 4;
volatile sig_atomic_t running = 1;

/* Signal handler for graceful shutdown */
void signal_handler(int signum) {
    (void)signum;
    printf("\nReceived shutdown signal. Stopping server...\n");
    running = 0;
}

/*
 * Message structure using pre-registered buffers
 * These buffers are allocated once and reused, allowing
 * the kernel to reference them directly without copying
 */
typedef struct {
    char *fields[NUM_FIELDS];  // Pre-allocated buffers
    struct iovec iov[NUM_FIELDS];  // iovec array for scatter-gather I/O
} MessageOneCopy;

/*
 * allocate_message_onecopy: Allocates pre-registered buffers
 * 
 * WHY THIS ENABLES ONE-COPY:
 * - Buffers are allocated once and page-aligned (can be optimized further)
 * - Kernel can set up direct DMA descriptors pointing to these buffers
 * - No need to copy from user buffer to kernel buffer (eliminates COPY 1)
 * - Kernel directly DMAs from user-space buffers to NIC (only COPY 2 remains)
 */
MessageOneCopy* allocate_message_onecopy(int field_size) {
    MessageOneCopy *msg = (MessageOneCopy*)malloc(sizeof(MessageOneCopy));
    if (!msg) {
        perror("Failed to allocate message structure");
        return NULL;
    }
    
    // Allocate each field as a pre-registered buffer
    for (int i = 0; i < NUM_FIELDS; i++) {
        // Allocate page-aligned memory for better DMA performance
        // Note: For true zero-copy, these would need to be pinned pages
        msg->fields[i] = (char*)aligned_alloc(4096, field_size);
        if (!msg->fields[i]) {
            perror("Failed to allocate field buffer");
            // Cleanup previously allocated fields
            for (int j = 0; j < i; j++) {
                free(msg->fields[j]);
            }
            free(msg);
            return NULL;
        }
        
        // Initialize with test data
        memset(msg->fields[i], 'A' + i, field_size - 1);
        msg->fields[i][field_size - 1] = '\0';
        
        // Set up iovec structure for this field
        // iovec allows kernel to gather data from multiple buffers
        // without copying them into a single contiguous buffer
        msg->iov[i].iov_base = msg->fields[i];
        msg->iov[i].iov_len = field_size;
    }
    
    return msg;
}

/*
 * free_message_onecopy: Frees pre-registered buffers
 */
void free_message_onecopy(MessageOneCopy *msg) {
    if (msg) {
        for (int i = 0; i < NUM_FIELDS; i++) {
            free(msg->fields[i]);
        }
        free(msg);
    }
}

/*
 * send_message_onecopy: Sends message using ONE-COPY model via sendmsg()
 * 
 * ONE-COPY ARCHITECTURE:
 * ----------------------
 * Traditional send() (TWO-COPY):
 *   User Buffer → [COPY 1] → Kernel Socket Buffer → [COPY 2 via DMA] → NIC
 * 
 * sendmsg() with iovec (ONE-COPY):
 *   User Pre-registered Buffer → [ELIMINATED] → [COPY via DMA] → NIC
 * 
 * HOW IT WORKS:
 * 1. iovec array describes multiple non-contiguous buffers
 * 2. sendmsg() uses scatter-gather I/O: kernel builds a descriptor list
 * 3. NIC's DMA engine reads directly from user buffers using descriptor list
 * 4. No intermediate copy to kernel buffer needed (COPY 1 eliminated)
 * 5. Only DMA transfer to NIC remains (COPY 2: Kernel/User → NIC)
 * 
 * KERNEL BEHAVIOR:
 * - tcp_sendmsg() references user pages instead of copying
 * - sk_buff points directly to user memory (if pages are pinned)
 * - DMA descriptors set up to read from original user buffers
 */
int send_message_onecopy(int sockfd, MessageOneCopy *msg) {
    struct msghdr msgh;
    memset(&msgh, 0, sizeof(msgh));
    
    // Set up message header for sendmsg()
    // iov points to our pre-registered buffer array
    // iovlen indicates number of buffers to send
    msgh.msg_iov = msg->iov;
    msgh.msg_iovlen = NUM_FIELDS;
    msgh.msg_control = NULL;
    msgh.msg_controllen = 0;
    
    // sendmsg() with iovec - enables ONE-COPY transmission
    // Kernel sets up scatter-gather DMA without copying data
    ssize_t sent = sendmsg(sockfd, &msgh, 0);
    
    if (sent < 0) {
        return -1;
    }
    
    return sent;
}

/*
 * client_handler: Thread function for each client
 */
void* client_handler(void *arg) {
    int client_sock = *(int*)arg;
    free(arg);
    
    printf("[Thread %lu] Client connected\n", pthread_self());
    
    // Allocate pre-registered message buffers (one-time allocation)
    MessageOneCopy *msg = allocate_message_onecopy(message_size);
    if (!msg) {
        close(client_sock);
        return NULL;
    }
    
    int messages_sent = 0;
    while (running) {
        // Send using ONE-COPY model
        int result = send_message_onecopy(client_sock, msg);
        if (result < 0) {
            if (errno == EPIPE || errno == ECONNRESET) {
                printf("[Thread %lu] Client disconnected\n", pthread_self());
                break;
            }
            perror("sendmsg error");
            break;
        }
        
        messages_sent++;
    }
    
    printf("[Thread %lu] Total messages sent: %d\n", pthread_self(), messages_sent);
    
    free_message_onecopy(msg);
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
    
    printf("=== PA02 Part A2: One-Copy Server ===\n");
    printf("Roll Number: MT25190\n");
    printf("Port: %d\n", port);
    printf("Message size: %d bytes per field\n", message_size);
    printf("Expected threads: %d\n", num_threads);
    printf("\nONE-COPY OPTIMIZATION:\n");
    printf("- Using sendmsg() with struct iovec\n");
    printf("- Pre-registered buffers eliminate User→Kernel copy\n");
    printf("- Only Kernel→NIC DMA copy remains\n\n");
    
    // Create TCP socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options
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
    
    // Bind and listen
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_sock, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d...\n", port);
    
    // Accept connections
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
        
        int *sock_ptr = (int*)malloc(sizeof(int));
        *sock_ptr = client_sock;
        
        if (pthread_create(&thread_id, NULL, client_handler, sock_ptr) != 0) {
            perror("Thread creation failed");
            free(sock_ptr);
            close(client_sock);
            continue;
        }
        
        pthread_detach(thread_id);
        connected_clients++;
    }
    
    printf("\nAll %d clients connected. Press Ctrl+C to stop.\n", num_threads);
    
    while (running) {
        sleep(1);
    }
    
    close(server_sock);
    return 0;
}
