#include "crypto_system.h"

// 워커 프로세스 메인 함수 (교안 ch07, ch10 기반)
void worker_main(int worker_id, int read_fd, int write_fd,
                 SharedData *shared, const char *filename) {
    printf("[Worker %d] Started (PID: %d, PPID: %d)\n",
           worker_id, getpid(), getppid());

    // 작업 수신 대기
    WorkTask task;
    ssize_t n;

    // EINTR 처리 (시그널로 인한 중단 복구)
    while ((n = read(read_fd, &task, sizeof(WorkTask))) == -1) {
        if (errno == EINTR) {
            continue;  // 시그널로 중단되었으면 재시도
        }
        perror("[Worker] read failed");
        exit(1);
    }

    if (n != sizeof(WorkTask)) {
        fprintf(stderr, "[Worker %d] Failed to read task (got %zd bytes, expected %zu)\n",
                worker_id, n, sizeof(WorkTask));
        exit(1);
    }

    printf("[Worker %d] Received task: chunk_id=%d, offset=%ld, size=%zu, operation=%c\n",
           worker_id, task.chunk_id, task.offset, task.size, task.operation);

    // 파일 메모리 매핑
    size_t file_size;
    void *mapped_data = map_file_to_memory(filename, &file_size, 1);
    if (!mapped_data) {
        fprintf(stderr, "[Worker %d] Failed to map file\n", worker_id);

        // 에러 상태 보고
        ProgressReport error_report;
        error_report.chunk_id = task.chunk_id;
        error_report.status = STATUS_ERROR;
        error_report.worker_pid = getpid();
        error_report.progress = 0.0;
        write(write_fd, &error_report, sizeof(ProgressReport));

        exit(1);
    }

    // 공유 메모리 업데이트: 작업 시작
    pthread_mutex_lock(&shared->mutex);
    shared->worker_status[worker_id] = STATUS_WORKING;
    pthread_mutex_unlock(&shared->mutex);

    // 자신의 청크 암호화/복호화
    unsigned char *chunk_start = (unsigned char*)mapped_data + task.offset;

    // 진행률 표시를 위한 중간 보고 (큰 파일의 경우)
    size_t chunk_size = task.size;
    size_t progress_interval = chunk_size / 10;  // 10% 단위로 보고
    if (progress_interval < 1024 * 1024) {
        progress_interval = chunk_size;  // 작은 청크는 한 번에
    }

    printf("[Worker %d] Processing chunk %d (%zu bytes)...\n",
           worker_id, task.chunk_id, chunk_size);

    if (task.operation == 'e') {
        // 암호화
        for (size_t processed = 0; processed < chunk_size; processed += progress_interval) {
            size_t block_size = (processed + progress_interval > chunk_size) ?
                                (chunk_size - processed) : progress_interval;

            xor_encrypt(chunk_start + processed, block_size, task.key);

            // 진행률 업데이트
            pthread_mutex_lock(&shared->mutex);
            shared->worker_progress[worker_id] = (double)(processed + block_size) / chunk_size;
            pthread_mutex_unlock(&shared->mutex);
        }
    } else {
        // 복호화
        for (size_t processed = 0; processed < chunk_size; processed += progress_interval) {
            size_t block_size = (processed + progress_interval > chunk_size) ?
                                (chunk_size - processed) : progress_interval;

            xor_decrypt(chunk_start + processed, block_size, task.key);

            // 진행률 업데이트
            pthread_mutex_lock(&shared->mutex);
            shared->worker_progress[worker_id] = (double)(processed + block_size) / chunk_size;
            pthread_mutex_unlock(&shared->mutex);
        }
    }

    // 메모리 동기화 (디스크에 기록) (교안 ch09 기반)
    printf("[Worker %d] Syncing chunk %d to disk...\n", worker_id, task.chunk_id);
    if (msync(chunk_start, chunk_size, MS_SYNC) == -1) {
        perror("[Worker] msync");
    }

    // 진행 상황 보고
    ProgressReport report;
    report.chunk_id = task.chunk_id;
    report.status = STATUS_DONE;
    report.worker_pid = getpid();
    report.progress = 1.0;

    // EINTR 처리
    ssize_t written;
    while ((written = write(write_fd, &report, sizeof(ProgressReport))) == -1) {
        if (errno == EINTR) {
            continue;
        }
        perror("[Worker] write failed");
        break;
    }

    // 공유 메모리 업데이트: 작업 완료
    pthread_mutex_lock(&shared->mutex);
    shared->completed_chunks++;
    shared->worker_status[worker_id] = STATUS_DONE;
    shared->worker_progress[worker_id] = 1.0;
    pthread_mutex_unlock(&shared->mutex);

    // 메모리 매핑 해제
    unmap_file(mapped_data, file_size);

    printf("[Worker %d] Completed chunk %d\n", worker_id, task.chunk_id);
    exit(0);
}
