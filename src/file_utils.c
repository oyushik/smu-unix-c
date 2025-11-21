#include "crypto_system.h"

// 파일 유효성 검증 (교안 ch03, ch04 기반)
int validate_file(const char *filename) {
    // access() 함수로 파일 존재 확인
    if (access(filename, F_OK) == -1) {
        fprintf(stderr, "Error: File '%s' does not exist\n", filename);
        return -1;
    }

    // access() 함수로 읽기 권한 확인
    if (access(filename, R_OK) == -1) {
        fprintf(stderr, "Error: Cannot read file '%s'\n", filename);
        return -1;
    }

    return 0;
}

// 파일 크기 확인 (교안 ch03 예제 기반)
size_t get_file_size(const char *filename) {
    struct stat statbuf;

    if (stat(filename, &statbuf) == -1) {
        perror("stat");
        return 0;
    }

    // 일반 파일인지 확인
    if (!S_ISREG(statbuf.st_mode)) {
        fprintf(stderr, "Error: '%s' is not a regular file\n", filename);
        return 0;
    }

    return statbuf.st_size;
}

// 출력 파일 생성 (교안 ch04 기반)
int create_output_file(const char *filename, size_t size) {
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return -1;
    }

    // 파일 크기 확장 (교안 ch09 예제 9-2 기반)
    if (ftruncate(fd, size) == -1) {
        perror("ftruncate");
        close(fd);
        return -1;
    }

    return fd;
}

// 파일 직접 복사 (system("cp") 대신 구현)
// mmap을 사용한 효율적인 파일 복사
int copy_file_direct(const char *src, const char *dst) {
    int src_fd = -1, dst_fd = -1;
    struct stat statbuf;
    void *src_map = NULL, *dst_map = NULL;
    int ret = -1;

    // 소스 파일 열기
    src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        perror("open source file");
        goto cleanup;
    }

    // 소스 파일 크기 확인
    if (fstat(src_fd, &statbuf) == -1) {
        perror("fstat");
        goto cleanup;
    }

    size_t file_size = statbuf.st_size;

    // 빈 파일 처리
    if (file_size == 0) {
        dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (dst_fd == -1) {
            perror("open destination file");
            goto cleanup;
        }
        ret = 0;
        goto cleanup;
    }

    // 목적지 파일 생성
    dst_fd = create_output_file(dst, file_size);
    if (dst_fd == -1) {
        goto cleanup;
    }

    // 소스 파일 메모리 매핑 (읽기 전용)
    src_map = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    if (src_map == MAP_FAILED) {
        perror("mmap source file");
        goto cleanup;
    }

    // 목적지 파일 메모리 매핑 (읽기/쓰기)
    dst_map = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, dst_fd, 0);
    if (dst_map == MAP_FAILED) {
        perror("mmap destination file");
        goto cleanup;
    }

    // 메모리 복사 (매우 빠름)
    memcpy(dst_map, src_map, file_size);

    // 디스크에 동기화
    if (msync(dst_map, file_size, MS_SYNC) == -1) {
        perror("msync");
        goto cleanup;
    }

    ret = 0;  // 성공

cleanup:
    // 메모리 매핑 해제
    if (src_map && src_map != MAP_FAILED) {
        munmap(src_map, file_size);
    }
    if (dst_map && dst_map != MAP_FAILED) {
        munmap(dst_map, file_size);
    }

    // 파일 디스크립터 닫기
    if (src_fd != -1) close(src_fd);
    if (dst_fd != -1) close(dst_fd);

    return ret;
}

// 디렉터리 처리 (교안 ch02 기반)
void process_directory(const char *dir_path, int num_workers,
                       char mode, const char *key) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    int file_count = 0;

    printf("\n=== Processing Directory: %s ===\n", dir_path);

    while ((entry = readdir(dir)) != NULL) {
        // "."과 ".." 건너뛰기
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char filepath[MAX_PATH_LEN];
        snprintf(filepath, sizeof(filepath), "%s/%s", dir_path, entry->d_name);

        // 일반 파일만 처리
        struct stat statbuf;
        if (stat(filepath, &statbuf) == 0 && S_ISREG(statbuf.st_mode)) {
            printf("\n[%d] Processing: %s\n", ++file_count, entry->d_name);

            // 출력 파일명 생성
            char output_file[MAX_PATH_LEN];
            if (mode == 'e') {
                snprintf(output_file, sizeof(output_file), "%s.encrypted", filepath);
            } else {
                // .encrypted 확장자 제거
                if (strstr(entry->d_name, ".encrypted")) {
                    snprintf(output_file, sizeof(output_file), "%s/%s",
                             dir_path, entry->d_name);
                    char *ext = strstr(output_file, ".encrypted");
                    if (ext) *ext = '\0';
                    strcat(output_file, ".decrypted");
                } else {
                    snprintf(output_file, sizeof(output_file), "%s.decrypted", filepath);
                }
            }

            // TODO: process_single_file() 호출 (main.c에서 구현 예정)
            // process_single_file(filepath, output_file, num_workers, mode, key);
            printf("    Output: %s\n", output_file);
        }
    }

    closedir(dir);

    if (file_count == 0) {
        printf("No regular files found in directory.\n");
    } else {
        printf("\n=== Total %d files processed ===\n", file_count);
    }
}
