#ifndef CRYPTO_SYSTEM_H
#define CRYPTO_SYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/sysinfo.h>
#include <sys/time.h>

// 상수 정의
#define MAX_WORKERS 16
#define MAX_PATH_LEN 4096
#define DEFAULT_WORKERS 4
#define CHUNK_MIN_SIZE (1024 * 1024)  // 1MB
#define SMALL_FILE_THRESHOLD (4 * 1024 * 1024)  // 4MB 이하는 단일 프로세스

// 작업 상태
#define STATUS_IDLE 0
#define STATUS_WORKING 1
#define STATUS_DONE 2
#define STATUS_ERROR 3

// 작업 정보 구조체
typedef struct {
    int chunk_id;           // 청크 ID
    off_t offset;           // 파일 오프셋
    size_t size;            // 청크 크기
    char operation;         // 'e' (encrypt) or 'd' (decrypt)
    char key[256];          // 암호화 키
} WorkTask;

// 진행 상황 보고 구조체
typedef struct {
    int chunk_id;           // 청크 ID
    int status;             // 작업 상태
    pid_t worker_pid;       // 워커 PID
    double progress;        // 진행률 (0.0 ~ 1.0)
} ProgressReport;

// 공유 메모리 구조체
typedef struct {
    int total_chunks;                   // 전체 청크 수
    int completed_chunks;               // 완료된 청크 수
    int worker_status[MAX_WORKERS];     // 각 워커 상태
    double worker_progress[MAX_WORKERS]; // 각 워커 진행률
    pthread_mutex_t mutex;              // 뮤텍스
    int shutdown_flag;                  // 종료 플래그
} SharedData;

// 함수 선언
// crypto.c
void xor_encrypt(unsigned char *data, size_t size, const char *key);
void xor_decrypt(unsigned char *data, size_t size, const char *key);

// file_utils.c
int validate_file(const char *filename);
size_t get_file_size(const char *filename);
int create_output_file(const char *filename, size_t size);
int copy_file_direct(const char *src, const char *dst);
void process_directory(const char *dir_path, int num_workers,
                       char mode, const char *key);

// ipc.c
SharedData* init_shared_memory(void);
void cleanup_shared_memory(SharedData *shared);
void* map_file_to_memory(const char *filename, size_t *file_size, int writable);
void unmap_file(void *addr, size_t size);
int create_pipes(int pipes_to[][2], int pipes_from[][2], int num_workers);
void close_unused_pipes(int pipes_to[][2], int pipes_from[][2],
                        int num_workers, int current_worker_id);

// worker.c
void worker_main(int worker_id, int read_fd, int write_fd,
                 SharedData *shared, const char *filename);

// signal_handler.c
void setup_signal_handlers(void);
void signal_handler(int signo);
void sigchld_handler(int signo);

// progress.c
void* progress_thread_func(void *arg);

// system_info.c
void print_system_info(void);
void print_performance_stats(struct timeval *start, struct timeval *end,
                             size_t file_size);

#endif // CRYPTO_SYSTEM_H
