/*
 * TCP client using recvmsg() with iovec for ONE-COPY receive
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <time.h>
#include <errno.h>

#define DEFAULT_PORT 8081
#define DEFAULT_SERVER "127.0.0.1"
#define NUM_FIELDS 8
#define RUN_DURATION 30

typedef struct {
    long bytes_received;
    long messages_received;
    double elapsed_time;
} ThreadStats;

char server_ip[32] = DEFAULT_SERVER;
int server_port = DEFAULT_PORT;
int message_size = 1024;
int num_threads = 4;
int run_duration = RUN_DURATION;  
volatile int running = 1;

/*
 * receive_message_onecopy: Uses recvmsg() with iovec for ONE-COPY receive
 * Kernel DMAs directly into pre-registered user buffers
 */
ssize_t receive_message_onecopy(int sockfd, struct iovec *iov, int iovcnt) {
    struct msghdr msgh;
    memset(&msgh, 0, sizeof(msgh));
    
    msgh.msg_iov = iov;
    msgh.msg_iovlen = iovcnt;
    
    ssize_t received = recvmsg(sockfd, &msgh, 0);
    return received;
}

void* client_thread(void *arg) {
    int thread_id = *(int*)arg;
    free(arg);
    
    int sock;
    struct sockaddr_in server_addr;
    struct iovec iov[NUM_FIELDS];
    char *buffers[NUM_FIELDS];
    ThreadStats stats = {0, 0, 0.0};
    struct timespec start_time, end_time;
    
    // Allocate pre-registered buffers for ONE-COPY receive
    for (int i = 0; i < NUM_FIELDS; i++) {
        buffers[i] = (char*)aligned_alloc(4096, message_size);
        if (!buffers[i]) {
            perror("Failed to allocate buffer");
            // Cleanup previously allocated buffers
            for (int j = 0; j < i; j++) {
                free(buffers[j]);
            }
            return NULL;
        }
        iov[i].iov_base = buffers[i];
        iov[i].iov_len = message_size;
    }
    
    // Create and connect socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return NULL;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    
    printf("[Thread %d] Connecting to server...\n", thread_id);
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return NULL;
    }
    
    printf("[Thread %d] Connected\n", thread_id);
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    // Receive messages
    while (running) {
        ssize_t received = receive_message_onecopy(sock, iov, NUM_FIELDS);
        if (received <= 0) break;
        
        stats.bytes_received += received;
        stats.messages_received++;
        
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        double elapsed = (end_time.tv_sec - start_time.tv_sec) +
                        (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
        if (elapsed >= run_duration) running = 0;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    stats.elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                        (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    
    printf("\n[Thread %d] Statistics:\n", thread_id);
    printf("  Messages: %ld, Bytes: %ld\n", stats.messages_received, stats.bytes_received);
    printf("  Throughput: %.2f MB/s\n",
           (stats.bytes_received / (1024.0 * 1024.0)) / stats.elapsed_time);
    
    // Cleanup
    close(sock);
    for (int i = 0; i < NUM_FIELDS; i++) {
        free(buffers[i]);
    }
    
    ThreadStats *result = (ThreadStats*)malloc(sizeof(ThreadStats));
    *result = stats;
    return result;
}

int main(int argc, char *argv[]) {
    pthread_t *threads;
    ThreadStats aggregate = {0, 0, 0.0};
    
    // Parse command line arguments: <server_ip> <port> <message_size> <num_threads> <duration>
    // PA02 requirement: All parameters must be passed explicitly for automation
    if (argc > 1) strncpy(server_ip, argv[1], sizeof(server_ip) - 1);
    if (argc > 2) server_port = atoi(argv[2]);
    if (argc > 3) message_size = atoi(argv[3]);
    if (argc > 4) num_threads = atoi(argv[4]);
    if (argc > 5) run_duration = atoi(argv[5]);
    
    printf("=== PA02 Part A2: One-Copy Client ===\n");
    printf("Roll Number: MT25190\n");
    printf("Server: %s:%d\n", server_ip, server_port);
    printf("Message size: %d bytes, Threads: %d, Duration: %d sec\n\n", 
           message_size, num_threads, run_duration);
    
    threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    
    for (int i = 0; i < num_threads; i++) {
        int *id = (int*)malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&threads[i], NULL, client_thread, id);
        usleep(10000);  // 10ms
    }
    
    for (int i = 0; i < num_threads; i++) {
        ThreadStats *stats;
        pthread_join(threads[i], (void**)&stats);
        if (stats) {
            aggregate.bytes_received += stats->bytes_received;
            aggregate.messages_received += stats->messages_received;
            if (stats->elapsed_time > aggregate.elapsed_time)
                aggregate.elapsed_time = stats->elapsed_time;
            free(stats);
        }
    }
    
    printf("\n=== Aggregate Statistics ===\n");
    printf("Total messages: %ld\n", aggregate.messages_received);
    printf("Total bytes: %ld (%.2f MB)\n",
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
