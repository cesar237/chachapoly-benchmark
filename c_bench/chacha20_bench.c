#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <unistd.h>
#include <time.h>

#define MAX_QUEUE 10240

typedef struct {
    uint8_t *msg;
    size_t len;
} QueueItem;

typedef struct {
    QueueItem queue[MAX_QUEUE];
    int front, rear, count;
    pthread_spinlock_t lock;
    pthread_cond_t cond;
} Queue;

typedef struct {
    Queue *queue;
    uint8_t *shared_msg;
    size_t msg_len;
    time_t end_time;
    int mode;
    unsigned int sleep_time;  // New parameter for sleep duration in microseconds
} ThreadArgs;

typedef struct {
    double total_lock_wait_time;
    double total_processing_time;
    long message_count;
} TimingStats;

void queue_init(Queue *q) {
    q->front = q->rear = q->count = 0;
    pthread_spin_init(&q->lock, PTHREAD_PROCESS_PRIVATE);
    pthread_cond_init(&q->cond, NULL);
}

uint64_t get_microseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

void enqueue(Queue *q, uint8_t *msg, size_t len) {
    pthread_spin_lock(&q->lock);
    if (q->count < MAX_QUEUE) {
        q->queue[q->rear].msg = malloc(len);
        memcpy(q->queue[q->rear].msg, msg, len);
        q->queue[q->rear].len = len;
        q->rear = (q->rear + 1) % MAX_QUEUE;
        q->count++;
        pthread_cond_signal(&q->cond);
    }
    pthread_spin_unlock(&q->lock);
}

QueueItem dequeue_with_timing(Queue *q, double *wait_time) {
    QueueItem item = {NULL, 0};
    uint64_t start_wait = get_microseconds();
    
    pthread_spin_lock(&q->lock);
    while (q->count == 0) {
        pthread_spin_unlock(&q->lock);
        usleep(100);
        pthread_spin_lock(&q->lock);
        if (q->count > 0) break;
    }
    
    uint64_t end_wait = get_microseconds();
    *wait_time = (end_wait - start_wait) / 1000000.0;

    if (q->count > 0) {
        item = q->queue[q->front];
        q->front = (q->front + 1) % MAX_QUEUE;
        q->count--;
    }
    pthread_spin_unlock(&q->lock);
    return item;
}

void* encrypt_routine(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    TimingStats *stats = calloc(1, sizeof(TimingStats));
    
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    uint8_t key[32], nonce[12];
    RAND_bytes(key, sizeof(key));
    RAND_bytes(nonce, sizeof(nonce));

    if (args->mode == 0) {  // Direct mode
        uint8_t *ciphertext = malloc(args->msg_len + 16);
        int len;
        
        while (time(NULL) < args->end_time) {
            uint64_t start_process = get_microseconds();
            
            EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
            EVP_EncryptUpdate(ctx, ciphertext, &len, args->shared_msg, args->msg_len);
            
            // Add sleep after encryption
            if (args->sleep_time > 0) {
                usleep(args->sleep_time);
            }
            
            uint64_t end_process = get_microseconds();
            stats->total_processing_time += (end_process - start_process) / 1000000.0;
            stats->message_count++;
        }
        
        free(ciphertext);
    } else {  // Queue mode
        while (time(NULL) < args->end_time) {
            double wait_time;
            QueueItem item = dequeue_with_timing(args->queue, &wait_time);
            stats->total_lock_wait_time += wait_time;
            
            if (item.msg == NULL) break;

            uint8_t *ciphertext = malloc(item.len + 16);
            int len;
            
            uint64_t start_process = get_microseconds();
            
            EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
            EVP_EncryptUpdate(ctx, ciphertext, &len, item.msg, item.len);
            
            // Add sleep after encryption
            if (args->sleep_time > 0) {
                usleep(args->sleep_time);
            }
            
            uint64_t end_process = get_microseconds();
            stats->total_processing_time += (end_process - start_process) / 1000000.0;
            
            free(item.msg);
            free(ciphertext);
            stats->message_count++;
        }
    }
    
    EVP_CIPHER_CTX_free(ctx);
    return stats;
}

void* decrypt_routine(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    TimingStats *stats = calloc(1, sizeof(TimingStats));
    
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    uint8_t key[32], nonce[12];
    RAND_bytes(key, sizeof(key));
    RAND_bytes(nonce, sizeof(nonce));

    if (args->mode == 0) {  // Direct mode
        uint8_t *plaintext = malloc(args->msg_len);
        int len;
        
        while (time(NULL) < args->end_time) {
            uint64_t start_process = get_microseconds();
            
            EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
            EVP_DecryptUpdate(ctx, plaintext, &len, args->shared_msg, args->msg_len);
            
            // Add sleep after decryption
            if (args->sleep_time > 0) {
                usleep(args->sleep_time);
            }
            
            uint64_t end_process = get_microseconds();
            stats->total_processing_time += (end_process - start_process) / 1000000.0;
            stats->message_count++;
        }
        
        free(plaintext);
    } else {  // Queue mode
        while (time(NULL) < args->end_time) {
            double wait_time;
            QueueItem item = dequeue_with_timing(args->queue, &wait_time);
            stats->total_lock_wait_time += wait_time;
            
            if (item.msg == NULL) break;

            if (item.len < 12) {
                free(item.msg);
                continue;
            }

            uint8_t *plaintext = malloc(item.len);
            int len;
            
            uint64_t start_process = get_microseconds();
            
            EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, key, nonce);
            EVP_DecryptUpdate(ctx, plaintext, &len, item.msg, item.len);
            
            // Add sleep after decryption
            if (args->sleep_time > 0) {
                usleep(args->sleep_time);
            }
            
            uint64_t end_process = get_microseconds();
            stats->total_processing_time += (end_process - start_process) / 1000000.0;
            
            free(plaintext);
            free(item.msg);
            stats->message_count++;
        }
    }
    
    EVP_CIPHER_CTX_free(ctx);
    return stats;
}

int generate_message(Queue *queue, size_t msg_len, int pps, int total_time) {
    if (pps == 0) return 0;
    
    uint8_t *msg = malloc(msg_len);
    RAND_bytes(msg, msg_len);
    time_t end_time = time(NULL) + total_time;
    int total_generated = 0;
    int delay_ms = 1000 / (pps / 1000);
    int batch_size = 1000;
    
    while (time(NULL) < end_time) {
        for (int i = 0; i < batch_size; i++) {
            enqueue(queue, msg, msg_len);
            total_generated++;
        }
        usleep(delay_ms * 1000);
    }
    
    free(msg);
    printf("Total messages generated: %d\n", total_generated);
    return total_generated;
}

int main(int argc, char *argv[]) {
    if (argc != 7) {
        fprintf(stderr, "Usage: %s encryption|decryption msg-len n-routines pps duration sleep-time\n", argv[0]);
        return 1;
    }
    const char *mode = argv[1];
    size_t msg_len = atoi(argv[2]);
    int n_routines = atoi(argv[3]);
    int pps = atoi(argv[4]);
    int total_time = atoi(argv[5]);
    unsigned int sleep_time = atoi(argv[6]);  // New sleep time parameter in microseconds

    pthread_t threads[n_routines];
    ThreadArgs *thread_args = malloc(sizeof(ThreadArgs) * n_routines);
    time_t end_time = time(NULL) + total_time;

    Queue queue;
    uint8_t *shared_msg = NULL;
    
    if (pps == 0) {
        shared_msg = malloc(msg_len);
        RAND_bytes(shared_msg, msg_len);
    } else {
        queue_init(&queue);
    }

    for (int i = 0; i < n_routines; i++) {
        thread_args[i].end_time = end_time;
        thread_args[i].msg_len = msg_len;
        thread_args[i].mode = (pps == 0) ? 0 : 1;
        thread_args[i].shared_msg = shared_msg;
        thread_args[i].queue = (pps == 0) ? NULL : &queue;
        thread_args[i].sleep_time = sleep_time;  // Initialize sleep time for each thread
    }

    if (strcmp(mode, "encryption") == 0) {
        for (int i = 0; i < n_routines; i++) {
            pthread_create(&threads[i], NULL, encrypt_routine, &thread_args[i]);
        }
    } else if (strcmp(mode, "decryption") == 0) {
        for (int i = 0; i < n_routines; i++) {
            pthread_create(&threads[i], NULL, decrypt_routine, &thread_args[i]);
        }
    } else {
        fprintf(stderr, "Invalid mode. Use 'encryption' or 'decryption'.\n");
        goto cleanup;
    }

    int generated = 0;
    if (pps > 0) {
        generated = generate_message(&queue, msg_len, pps, total_time);
    }

    double total_lock_wait = 0;
    double total_processing = 0;
    long total_messages = 0;

    for (int i = 0; i < n_routines; i++) {
        if (pps > 0) {
            enqueue(&queue, NULL, 0);
        }
        TimingStats *stats;
        pthread_join(threads[i], (void**)&stats);
        
        total_lock_wait += stats->total_lock_wait_time;
        total_processing += stats->total_processing_time;
        total_messages += stats->message_count;
        
        free(stats);
    }

    printf("\nTiming Statistics:\n");
    printf("Total messages processed: %ld\n", total_messages);
    printf("Total lock wait time: %.6f seconds\n", total_lock_wait);
    printf("Total processing time: %.6f seconds\n", total_processing);
    
    if (total_messages > 0) {
        printf("\nPer Message Statistics:\n");
        printf("Average lock wait time: %.6f microseconds\n", (total_lock_wait / total_messages) * 1000000);
        printf("Average processing time: %.6f microseconds\n", (total_processing / total_messages) * 1000000);
    }
    
    printf("\nThread Statistics:\n");
    printf("Average lock wait time per thread: %.6f seconds\n", total_lock_wait / n_routines);
    printf("Average processing time per thread: %.6f seconds\n", total_processing / n_routines);
    
    if (pps > 0) {
        printf("\nQueue Statistics:\n");
        printf("Messages generated: %d\n", generated);
        printf("Messages dropped: %d\n", generated - total_messages);
    }

cleanup:
    if (pps > 0) {
        pthread_spin_destroy(&queue.lock);
        pthread_cond_destroy(&queue.cond);
    }
    free(thread_args);
    free(shared_msg);
    
    return 0;
}