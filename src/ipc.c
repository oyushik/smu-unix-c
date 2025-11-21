#include "crypto_system.h"

// 공유 메모리 초기화 (교안 ch09 기반)
SharedData* init_shared_memory(void) {
    // MAP_ANONYMOUS | MAP_SHARED로 프로세스 간 공유 메모리 생성
    SharedData *shared = mmap(NULL, sizeof(SharedData),
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED | MAP_ANONYMOUS,
                              -1, 0);

    if (shared == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }

    // 공유 데이터 초기화
    shared->total_chunks = 0;
    shared->completed_chunks = 0;
    shared->shutdown_flag = 0;
    memset(shared->worker_status, 0, sizeof(shared->worker_status));
    memset(shared->worker_progress, 0, sizeof(shared->worker_progress));

    // 프로세스 간 공유 뮤텍스 초기화 (교안 ch11 기반)
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shared->mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    return shared;
}

// 공유 메모리 해제
void cleanup_shared_memory(SharedData *shared) {
    if (shared) {
        pthread_mutex_destroy(&shared->mutex);
        munmap(shared, sizeof(SharedData));
    }
}

// 파일을 메모리에 매핑 (교안 ch09 예제 9-1 기반)
void* map_file_to_memory(const char *filename, size_t *file_size, int writable) {
    int flags = writable ? O_RDWR : O_RDONLY;
    int fd = open(filename, flags);
    if (fd == -1) {
        perror("open");
        return NULL;
    }

    struct stat statbuf;
    if (fstat(fd, &statbuf) == -1) {
        perror("fstat");
        close(fd);
        return NULL;
    }

    *file_size = statbuf.st_size;

    // 빈 파일 처리
    if (*file_size == 0) {
        close(fd);
        fprintf(stderr, "Error: Cannot map empty file\n");
        return NULL;
    }

    int prot = PROT_READ;
    if (writable) prot |= PROT_WRITE;

    void *addr = mmap(NULL, *file_size, prot, MAP_SHARED, fd, 0);
    close(fd);  // 매핑 후 파일 디스크립터는 닫아도 됨

    if (addr == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }

    return addr;
}

// 메모리 매핑 해제
void unmap_file(void *addr, size_t size) {
    if (addr && munmap(addr, size) == -1) {
        perror("munmap");
    }
}

// 파이프 생성 (교안 ch10 기반)
// Phase 2에서 구현
int create_pipes(int pipes_to[][2], int pipes_from[][2], int num_workers) {
    for (int i = 0; i < num_workers; i++) {
        if (pipe(pipes_to[i]) == -1) {
            perror("pipe (to worker)");
            // 이미 생성된 파이프 정리
            for (int j = 0; j < i; j++) {
                close(pipes_to[j][0]);
                close(pipes_to[j][1]);
                close(pipes_from[j][0]);
                close(pipes_from[j][1]);
            }
            return -1;
        }
        if (pipe(pipes_from[i]) == -1) {
            perror("pipe (from worker)");
            // 이미 생성된 파이프 정리
            close(pipes_to[i][0]);
            close(pipes_to[i][1]);
            for (int j = 0; j < i; j++) {
                close(pipes_to[j][0]);
                close(pipes_to[j][1]);
                close(pipes_from[j][0]);
                close(pipes_from[j][1]);
            }
            return -1;
        }
    }
    return 0;
}

// 사용하지 않는 파이프 닫기 (워커 프로세스에서 호출)
void close_unused_pipes(int pipes_to[][2], int pipes_from[][2],
                        int num_workers, int current_worker_id) {
    for (int i = 0; i < num_workers; i++) {
        if (i != current_worker_id) {
            // 다른 워커의 파이프 모두 닫기
            close(pipes_to[i][0]);
            close(pipes_to[i][1]);
            close(pipes_from[i][0]);
            close(pipes_from[i][1]);
        }
    }

    // 현재 워커의 파이프에서 사용하지 않는 쪽 닫기
    close(pipes_to[current_worker_id][1]);      // 쓰기 끝 닫기 (읽기만 함)
    close(pipes_from[current_worker_id][0]);    // 읽기 끝 닫기 (쓰기만 함)
}
