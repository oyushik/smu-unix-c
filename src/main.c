#include "crypto_system.h"
#include <getopt.h>

// 전역 변수 (다음 단계에서 멀티프로세스용으로 사용)
pid_t worker_pids[MAX_WORKERS];
int pipes_to_workers[MAX_WORKERS][2];
int pipes_from_workers[MAX_WORKERS][2];
SharedData *shared_data = NULL;

// 사용법 출력
void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("\nOptions:\n");
    printf("  -e <file>    Encrypt file\n");
    printf("  -d <file>    Decrypt file\n");
    printf("  -o <file>    Output file (default: <input>.encrypted or <input>.decrypted)\n");
    printf("  -k <key>     Encryption key (required)\n");
    printf("  -w <num>     Number of worker processes (default: 4, range: 1-%d)\n", MAX_WORKERS);
    printf("  -D <dir>     Process entire directory\n");
    printf("  -v           Verbose mode (show system info)\n");
    printf("  -h           Show this help message\n");
    printf("\nExamples:\n");
    printf("  %s -e input.dat -k \"mypassword\"                    # Single process encryption\n", program_name);
    printf("  %s -e input.dat -o output.dat -k \"pass\" -w 4       # 4 workers encryption\n", program_name);
    printf("  %s -d encrypted.dat -k \"mypassword\"                # Decryption\n", program_name);
    printf("  %s -D /path/to/dir -k \"pass\" -e                    # Encrypt directory\n", program_name);
}

// 단일 프로세스 파일 처리 (1단계: 기본 구현)
int process_single_file_simple(const char *input_file, const char *output_file,
                                char mode, const char *key) {
    struct timeval start, end;
    gettimeofday(&start, NULL);

    printf("\n=== Crypto System (Single Process Mode) ===\n");
    printf("Input file: %s\n", input_file);
    printf("Output file: %s\n", output_file);
    printf("Mode: %s\n", mode == 'e' ? "Encryption" : "Decryption");
    printf("Process ID: %d\n", getpid());

    // 파일 검증
    if (validate_file(input_file) == -1) {
        return -1;
    }

    size_t file_size = get_file_size(input_file);
    if (file_size == 0) {
        fprintf(stderr, "Error: File is empty or invalid\n");
        return -1;
    }

    printf("File size: %.2f MB\n", file_size / 1024.0 / 1024.0);

    // 파일 복사 (입력 -> 출력)
    printf("\nCopying file...\n");
    if (copy_file_direct(input_file, output_file) == -1) {
        fprintf(stderr, "Error: Failed to copy file\n");
        return -1;
    }

    // 출력 파일을 메모리에 매핑
    printf("Mapping file to memory...\n");
    size_t mapped_size;
    void *mapped_data = map_file_to_memory(output_file, &mapped_size, 1);
    if (!mapped_data) {
        fprintf(stderr, "Error: Failed to map file to memory\n");
        return -1;
    }

    // 암호화/복호화 수행
    printf("Processing...\n");
    if (mode == 'e') {
        xor_encrypt((unsigned char*)mapped_data, mapped_size, key);
    } else {
        xor_decrypt((unsigned char*)mapped_data, mapped_size, key);
    }

    // 메모리 동기화 (디스크에 기록)
    printf("Syncing to disk...\n");
    if (msync(mapped_data, mapped_size, MS_SYNC) == -1) {
        perror("msync");
        unmap_file(mapped_data, mapped_size);
        return -1;
    }

    // 메모리 매핑 해제
    unmap_file(mapped_data, mapped_size);

    gettimeofday(&end, NULL);

    printf("\n=== Processing Complete ===\n");
    printf("Output file: %s\n", output_file);

    // 성능 통계 출력
    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_usec - start.tv_usec) / 1000000.0;
    double mb_size = file_size / (1024.0 * 1024.0);
    double throughput = mb_size / elapsed;

    printf("\n=== Performance Statistics ===\n");
    printf("File size: %.2f MB\n", mb_size);
    printf("Processing time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", throughput);
    printf("==============================\n");

    return 0;
}

// 멀티프로세스 파일 처리 (2단계: 병렬 처리)
int process_single_file_multiprocess(const char *input_file, const char *output_file,
                                      int num_workers, char mode, const char *key) {
    struct timeval start, end;
    gettimeofday(&start, NULL);

    printf("\n=== Crypto System (Multi-Process Mode) ===\n");
    printf("Input file: %s\n", input_file);
    printf("Output file: %s\n", output_file);
    printf("Mode: %s\n", mode == 'e' ? "Encryption" : "Decryption");
    printf("Master PID: %d\n", getpid());
    printf("Workers: %d\n", num_workers);

    // 파일 검증
    if (validate_file(input_file) == -1) {
        return -1;
    }

    size_t file_size = get_file_size(input_file);
    if (file_size == 0) {
        fprintf(stderr, "Error: File is empty or invalid\n");
        return -1;
    }

    printf("File size: %.2f MB\n\n", file_size / 1024.0 / 1024.0);

    // 파일 복사
    printf("Copying file...\n");
    if (copy_file_direct(input_file, output_file) == -1) {
        fprintf(stderr, "Error: Failed to copy file\n");
        return -1;
    }

    // 공유 메모리 초기화
    shared_data = init_shared_memory();
    if (!shared_data) {
        return -1;
    }

    // 청크 계산
    size_t chunk_size = file_size / num_workers;
    if (chunk_size < CHUNK_MIN_SIZE && file_size > CHUNK_MIN_SIZE) {
        num_workers = file_size / CHUNK_MIN_SIZE;
        if (num_workers == 0) num_workers = 1;
        chunk_size = file_size / num_workers;
        printf("Adjusted workers to %d (chunk size: %.2f MB)\n",
               num_workers, chunk_size / 1024.0 / 1024.0);
    }

    shared_data->total_chunks = num_workers;

    // 파이프 생성 (교안 ch10 기반)
    printf("Creating pipes...\n");
    if (create_pipes(pipes_to_workers, pipes_from_workers, num_workers) == -1) {
        cleanup_shared_memory(shared_data);
        return -1;
    }

    // 시그널 핸들러 설정
    setup_signal_handlers();

    // 워커 프로세스 생성 (교안 ch07 예제 7-2 기반)
    printf("Creating %d worker processes...\n", num_workers);
    for (int i = 0; i < num_workers; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            // 이미 생성된 워커들 정리
            for (int j = 0; j < i; j++) {
                kill(worker_pids[j], SIGTERM);
                waitpid(worker_pids[j], NULL, 0);
            }
            cleanup_shared_memory(shared_data);
            return -1;
        }

        if (pid == 0) {  // 자식 프로세스 (워커)
            // 사용하지 않는 파이프 닫기
            close_unused_pipes(pipes_to_workers, pipes_from_workers,
                               num_workers, i);

            worker_main(i, pipes_to_workers[i][0], pipes_from_workers[i][1],
                       shared_data, output_file);
            exit(0);  // worker_main에서 exit하지만 명시적으로 추가
        }

        // 부모 프로세스
        worker_pids[i] = pid;
        close(pipes_to_workers[i][0]);      // 읽기 끝 닫기
        close(pipes_from_workers[i][1]);    // 쓰기 끝 닫기
    }

    printf("[Master] All workers created\n\n");

    // 작업 할당
    printf("=== Assigning tasks to workers ===\n");
    for (int i = 0; i < num_workers; i++) {
        WorkTask task;
        task.chunk_id = i;
        task.offset = i * chunk_size;
        task.size = (i == num_workers - 1) ?
                    (file_size - task.offset) : chunk_size;
        task.operation = mode;
        strncpy(task.key, key, sizeof(task.key) - 1);
        task.key[sizeof(task.key) - 1] = '\0';

        write(pipes_to_workers[i][1], &task, sizeof(WorkTask));
        printf("[Master] Worker %d (PID %d): chunk %d (offset=%ld, size=%zu)\n",
               i, worker_pids[i], i, task.offset, task.size);
    }
    printf("\n");

    // 워커들로부터 진행 상황 수신
    printf("=== Collecting results ===\n");
    int completed = 0;
    while (completed < num_workers) {
        for (int i = 0; i < num_workers; i++) {
            ProgressReport report;
            ssize_t n = read(pipes_from_workers[i][0], &report, sizeof(ProgressReport));

            if (n == sizeof(ProgressReport)) {
                if (report.status == STATUS_DONE) {
                    printf("[Master] Worker %d completed chunk %d\n",
                           report.worker_pid, report.chunk_id);
                    completed++;
                } else if (report.status == STATUS_ERROR) {
                    fprintf(stderr, "[Master] Worker %d reported error on chunk %d\n",
                            report.worker_pid, report.chunk_id);
                    completed++;
                }
            } else if (n == -1 && errno != EAGAIN && errno != EINTR) {
                break;  // 에러 또는 EOF
            }
        }
    }

    // 모든 워커 종료 대기 (교안 ch07 기반)
    printf("\n=== Waiting for workers to exit ===\n");
    for (int i = 0; i < num_workers; i++) {
        int status;
        waitpid(worker_pids[i], &status, 0);

        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            if (exit_code == 0) {
                printf("[Master] Worker %d (PID %d) exited successfully\n",
                       i, worker_pids[i]);
            } else {
                printf("[Master] Worker %d (PID %d) exited with error code %d\n",
                       i, worker_pids[i], exit_code);
            }
        } else if (WIFSIGNALED(status)) {
            printf("[Master] Worker %d (PID %d) killed by signal %d\n",
                   i, worker_pids[i], WTERMSIG(status));
        }
    }

    // 파이프 닫기
    for (int i = 0; i < num_workers; i++) {
        close(pipes_to_workers[i][1]);
        close(pipes_from_workers[i][0]);
    }

    // 정리
    cleanup_shared_memory(shared_data);
    shared_data = NULL;

    gettimeofday(&end, NULL);

    printf("\n=== Processing Complete ===\n");
    printf("Output file: %s\n", output_file);

    // 성능 통계 출력
    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_usec - start.tv_usec) / 1000000.0;
    double mb_size = file_size / (1024.0 * 1024.0);
    double throughput = mb_size / elapsed;

    printf("\n=== Performance Statistics ===\n");
    printf("File size: %.2f MB\n", mb_size);
    printf("Processing time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", throughput);
    printf("Workers: %d\n", num_workers);
    printf("==============================\n");

    return 0;
}

int main(int argc, char *argv[]) {
    char *input_file = NULL;
    char *output_file = NULL;
    char *key = NULL;
    char *directory = NULL;
    char mode = 0;  // 'e' or 'd'
    int num_workers = DEFAULT_WORKERS;
    int verbose = 0;

    // 명령행 인자 파싱
    int opt;
    while ((opt = getopt(argc, argv, "e:d:o:k:w:D:vh")) != -1) {
        switch (opt) {
            case 'e':
                mode = 'e';
                input_file = optarg;
                break;
            case 'd':
                mode = 'd';
                input_file = optarg;
                break;
            case 'o':
                output_file = optarg;
                break;
            case 'k':
                key = optarg;
                break;
            case 'w':
                num_workers = atoi(optarg);
                if (num_workers < 1 || num_workers > MAX_WORKERS) {
                    fprintf(stderr, "Error: Number of workers must be between 1 and %d\n",
                            MAX_WORKERS);
                    exit(1);
                }
                break;
            case 'D':
                directory = optarg;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                exit(0);
            default:
                print_usage(argv[0]);
                exit(1);
        }
    }

    // 입력 검증
    if (!key) {
        fprintf(stderr, "Error: Encryption key is required (-k option)\n\n");
        print_usage(argv[0]);
        exit(1);
    }

    if (!mode) {
        fprintf(stderr, "Error: Must specify encryption (-e) or decryption (-d)\n\n");
        print_usage(argv[0]);
        exit(1);
    }

    if (!input_file && !directory) {
        fprintf(stderr, "Error: Must specify input file or directory\n\n");
        print_usage(argv[0]);
        exit(1);
    }

    // 시스템 정보 출력 (verbose 모드)
    if (verbose) {
        print_system_info();
    }

    // 디렉터리 처리
    if (directory) {
        printf("Directory processing not yet implemented in this phase.\n");
        printf("Please specify a single file with -e or -d option.\n");
        return 1;
    }

    // 출력 파일명 자동 생성
    if (!output_file) {
        static char auto_output[MAX_PATH_LEN];
        if (mode == 'e') {
            snprintf(auto_output, sizeof(auto_output), "%s.encrypted", input_file);
        } else {
            // 복호화: .encrypted 확장자 제거 후 .decrypted 추가 (안전)
            if (strstr(input_file, ".encrypted")) {
                strncpy(auto_output, input_file, sizeof(auto_output) - 1);
                char *ext = strstr(auto_output, ".encrypted");
                if (ext) *ext = '\0';
                strncat(auto_output, ".decrypted",
                        sizeof(auto_output) - strlen(auto_output) - 1);
            } else {
                snprintf(auto_output, sizeof(auto_output), "%s.decrypted", input_file);
            }
        }
        output_file = auto_output;
    }

    // 단일 파일 처리
    if (num_workers == 1 || get_file_size(input_file) < SMALL_FILE_THRESHOLD) {
        // 단일 프로세스 모드
        if (num_workers > 1) {
            printf("Note: File is small (< 4MB), using single process mode for efficiency.\n");
        }
        return process_single_file_simple(input_file, output_file, mode, key);
    } else {
        // 멀티프로세스 모드 (2단계)
        return process_single_file_multiprocess(input_file, output_file,
                                                 num_workers, mode, key);
    }
}
