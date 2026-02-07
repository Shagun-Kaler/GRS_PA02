/*
 * TCP client using recv() - demonstrates TWO-COPY model on receive path:
 * Copy 1: NIC → Kernel space (DMA)
 * Copy 2: Kernel space → User space (via recv())
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <errno.h>

#define DEFAULT_PORT 8080
#define DEFAULT_SERVER "127.0.0.1"
#define BUFFER_SIZE 8192
#define RUN_DURATION 30  // Run for 30 seconds

/* Statistics structure for each thread */
typedef struct {
    long bytes_received;
    long messages_received;
    double elapsed_time;
} ThreadStats;

/* Global configuration */
char server_ip[32] = DEFAULT_SERVER;
int server_port = DEFAULT_PORT;
int message_size = 1024;
int num_threads = 4;
int run_duration = RUN_DURATION;  
volatile int running = 1;

/*
 * receive_data: Receives data using TWO-COPY model
 * 
 * RECEIVE PATH - TWO COPIES:
 * 1. COPY 1: NIC → Kernel
 *    - NIC receives packet from network
 *    - DMA controller copies packet data to kernel ring buffer (sk_buff)
 *    - Interrupt notifies kernel of new data
 * 
 * 2. COPY 2: Kernel → User
 *    - recv() syscall copies data from kernel socket buffer to user-space buffer
 *    - This requires CPU involvement and context switch
 *    - Data is copied from kernel memory to user-provided buffer
 */
ssize_t receive_data(int sockfd, char *buffer, size_t size) {
    size_t total_received = 0;  // FIX: Changed to size_t to match 'size' parameter
    ssize_t bytes_received;
    
    while (total_received < size) {
        // recv() triggers COPY 2: Kernel socket buffer → User buffer
        // The kernel has already received data from NIC (COPY 1: NIC → Kernel via DMA)
        bytes_received = recv(sockfd, buffer + total_received, 
                            size - total_received, 0);
        
        if (bytes_received < 0) {
            if (errno == EINTR) continue;  // Interrupted, retry
            return -1;  // Error
        }
        if (bytes_received == 0) {
            return total_received;  // Connection closed
        }
        
        total_received += bytes_received;
    }
    
    return total_received;
}

/*
 * client_thread: Each thread establishes connection and receives data
 */
void* client_thread(void *arg) {
    int thread_id = *(int*)arg;
    free(arg);
    
    int sock;
    struct sockaddr_in server_addr;
    char *buffer;
    ThreadStats stats = {0, 0, 0.0};
    struct timespec start_time, end_time;
    
    // Allocate receive buffer in user space
    buffer = (char*)malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("Buffer allocation failed");
        return NULL;
    }
    
    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        free(buffer);
        return NULL;
    }
    
    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        free(buffer);
        return NULL;
    }
    
    // Connect to server
    printf("[Thread %d] Connecting to server...\n", thread_id);
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        free(buffer);
        return NULL;
    }
    
    printf("[Thread %d] Connected to server\n", thread_id);
    
    // Start timing
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    // Receive data continuously
    while (running) {
        // Receive message fields (8 fields per message)
        for (int i = 0; i < 8; i++) {
            ssize_t received = receive_data(sock, buffer, message_size);
            if (received < 0) {
                perror("Receive error");
                goto cleanup;
            }
            if (received == 0) {
                printf("[Thread %d] Server closed connection\n", thread_id);
                goto cleanup;
            }
            
            stats.bytes_received += received;
        }
        
        stats.messages_received++;
        
        // Check if run duration exceeded
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        double elapsed = (end_time.tv_sec - start_time.tv_sec) +
                        (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
        if (elapsed >= run_duration) {
            running = 0;
        }
    }
    
cleanup:
    // Calculate final statistics
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    stats.elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                        (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    
    // Print thread statistics
    printf("\n[Thread %d] Statistics:\n", thread_id);
    printf("  Messages received: %ld\n", stats.messages_received);
    printf("  Bytes received: %ld\n", stats.bytes_received);
    printf("  Duration: %.2f seconds\n", stats.elapsed_time);
    printf("  Throughput: %.2f MB/s\n", 
           (stats.bytes_received / (1024.0 * 1024.0)) / stats.elapsed_time);
    
    close(sock);
    free(buffer);
    
    // Return statistics
    ThreadStats *result = (ThreadStats*)malloc(sizeof(ThreadStats));
    *result = stats;
    return result;
}

int main(int argc, char *argv[]) {
    pthread_t *threads;
    ThreadStats aggregate = {0, 0, 0.0};
    
    // Parse command line arguments: <server_ip> <port> <message_size> <num_threads> <duration>
    // PA02 requirement: All parameters must be passed explicitly for automation
    if (argc > 1) {
        strncpy(server_ip, argv[1], sizeof(server_ip) - 1);
    }
    if (argc > 2) {
        server_port = atoi(argv[2]);
    }
    if (argc > 3) {
        message_size = atoi(argv[3]);
    }
    if (argc > 4) {
        num_threads = atoi(argv[4]);
    }
    if (argc > 5) {
        run_duration = atoi(argv[5]);
    }
    
    printf("=== PA02 Part A1: Two-Copy Client ===\n");
    printf("Roll Number: MT25190\n");
    printf("Server: %s:%d\n", server_ip, server_port);
    printf("Message size: %d bytes per field\n", message_size);
    printf("Number of threads: %d\n", num_threads);
    printf("Run duration: %d seconds\n\n", run_duration);
    
    // Allocate thread array
    threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    if (!threads) {
        perror("Thread array allocation failed");
        exit(EXIT_FAILURE);
    }
    
    // Create client threads
    for (int i = 0; i < num_threads; i++) {
        int *thread_id = (int*)malloc(sizeof(int));
        *thread_id = i + 1;
        
        if (pthread_create(&threads[i], NULL, client_thread, thread_id) != 0) {
            perror("Thread creation failed");
            free(thread_id);
            continue;
        }
        
        // Small delay between thread creation
        usleep(10000);  // 10ms
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < num_threads; i++) {
        ThreadStats *stats;
        pthread_join(threads[i], (void**)&stats);
        
        if (stats) {
            aggregate.bytes_received += stats->bytes_received;
            aggregate.messages_received += stats->messages_received;
            if (stats->elapsed_time > aggregate.elapsed_time) {
                aggregate.elapsed_time = stats->elapsed_time;
            }
            free(stats);
        }
    }
    
    // Print aggregate statistics
    printf("\n=== Aggregate Statistics ===\n");
    printf("Total messages received: %ld\n", aggregate.messages_received);
    printf("Total bytes received: %ld (%.2f MB)\n", 
           aggregate.bytes_received,
           aggregate.bytes_received / (1024.0 * 1024.0));
    printf("Aggregate throughput: %.2f MB/s\n",
           (aggregate.bytes_received / (1024.0 * 1024.0)) / aggregate.elapsed_time);
    
    // PA02 requirement: Output parseable metrics for script collection
    double throughput_gbps = (aggregate.bytes_received * 8.0) / (aggregate.elapsed_time * 1e9);
    double latency_us = (aggregate.elapsed_time * 1e6) / aggregate.messages_received;
    printf("METRICS throughput_gbps=%.6f latency_us=%.2f bytes=%ld\n",
           throughput_gbps, latency_us, aggregate.bytes_received);
    
    free(threads);
    return 0;
}
