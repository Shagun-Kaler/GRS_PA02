#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#define DEFAULT_PORT 8082
#define DEFAULT_SERVER "127.0.0.1"
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

void* client_thread(void *arg) {
    int thread_id = *(int*)arg;
    free(arg);
    
    int sock;
    struct sockaddr_in server_addr;
    char *buffer;
    ThreadStats stats = {0, 0, 0.0};
    struct timespec start_time, end_time;
    
    buffer = aligned_alloc(4096, message_size * 8);
    if (!buffer) {
        perror("Failed to allocate buffer");
        return NULL;
    }
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        free(buffer);
        return NULL;
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    
    printf("[Thread %d] Connecting...\\n", thread_id);
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return NULL;
    }
    
    printf("[Thread %d] Connected\\n", thread_id);
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    while (running) {
        ssize_t received = recv(sock, buffer, message_size * 8, 0);
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
    
    printf("[Thread %d] Msgs: %ld, Throughput: %.2f MB/s\\n",
           thread_id, stats.messages_received,
           (stats.bytes_received / (1024.0 * 1024.0)) / stats.elapsed_time);
    
    close(sock);
    free(buffer);
    
    ThreadStats *result = malloc(sizeof(ThreadStats));
    *result = stats;
    return result;
}

int main(int argc, char *argv[]) {
    // Parse command line arguments: <server_ip> <port> <message_size> <num_threads> <duration>
    // PA02 requirement: All parameters must be passed explicitly for automation
    if (argc > 1) strncpy(server_ip, argv[1], sizeof(server_ip) - 1);
    if (argc > 2) server_port = atoi(argv[2]);
    if (argc > 3) message_size = atoi(argv[3]);
    if (argc > 4) num_threads = atoi(argv[4]);
    if (argc > 5) run_duration = atoi(argv[5]);
    
    printf("=== PA02 Part A3: Zero-Copy Client ===\n");
    printf("Roll Number: MT25190\n");
    printf("Server: %s:%d, Duration: %d sec\n\n", server_ip, server_port, run_duration);
    
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    ThreadStats aggregate = {0, 0, 0.0};
    
    for (int i = 0; i < num_threads; i++) {
        int *id = malloc(sizeof(int));
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
    
    printf("\n=== Aggregate ===\n");
    printf("Messages: %ld, Bytes: %.2f MB\n",
           aggregate.messages_received,
           aggregate.bytes_received / (1024.0 * 1024.0));
    printf("Throughput: %.2f MB/s\n",
           (aggregate.bytes_received / (1024.0 * 1024.0)) / aggregate.elapsed_time);    
    // PA02 requirement: Output parseable metrics for script collection
    double throughput_gbps = (aggregate.bytes_received * 8.0) / (aggregate.elapsed_time * 1e9);
    double latency_us = (aggregate.elapsed_time * 1e6) / aggregate.messages_received;
    printf("METRICS throughput_gbps=%.6f latency_us=%.2f bytes=%ld\n",
           throughput_gbps, latency_us, aggregate.bytes_received);    
    free(threads);
    return 0;
}
