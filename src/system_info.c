#include "crypto_system.h"

// 시스템 정보 출력 (교안 ch05 기반)
void print_system_info(void) {
    struct sysinfo info;

    if (sysinfo(&info) == -1) {
        perror("sysinfo");
        return;
    }

    printf("\n=== System Information ===\n");
    printf("Process ID: %d\n", getpid());
    printf("Parent PID: %d\n", getppid());
    printf("Total RAM: %ld MB\n", info.totalram / 1024 / 1024);
    printf("Free RAM: %ld MB\n", info.freeram / 1024 / 1024);
    printf("Shared RAM: %ld MB\n", info.sharedram / 1024 / 1024);
    printf("Buffer RAM: %ld MB\n", info.bufferram / 1024 / 1024);
    printf("Number of processes: %d\n", info.procs);
    printf("System uptime: %ld seconds (%.1f hours)\n",
           info.uptime, info.uptime / 3600.0);

    // CPU 정보 (가능한 경우)
    long num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cpus > 0) {
        printf("Available CPUs: %ld\n", num_cpus);
    }

    printf("==========================\n\n");
}

// 성능 통계 출력
void print_performance_stats(struct timeval *start, struct timeval *end,
                             size_t file_size) {
    double elapsed = (end->tv_sec - start->tv_sec) +
                     (end->tv_usec - start->tv_usec) / 1000000.0;

    double mb_size = file_size / (1024.0 * 1024.0);
    double throughput = mb_size / elapsed;

    printf("\n=== Performance Statistics ===\n");
    printf("File size: %.2f MB (%.0f bytes)\n", mb_size, (double)file_size);
    printf("Processing time: %.3f seconds\n", elapsed);
    printf("Throughput: %.2f MB/s\n", throughput);
    printf("==============================\n");
}
