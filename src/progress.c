#include "crypto_system.h"

// 진행률 스레드 함수 (교안 ch11 기반)
void* progress_thread_func(void *arg) {
    SharedData *shared = (SharedData*)arg;

    printf("\n[Progress] Monitoring started\n");

    while (1) {
        pthread_mutex_lock(&shared->mutex);

        int completed = shared->completed_chunks;
        int total = shared->total_chunks;
        int shutdown = shared->shutdown_flag;

        // 각 워커의 진행률 확인
        double total_progress = 0.0;
        for (int i = 0; i < total && i < MAX_WORKERS; i++) {
            total_progress += shared->worker_progress[i];
        }

        pthread_mutex_unlock(&shared->mutex);

        // 종료 확인
        if (shutdown || completed >= total) {
            break;
        }

        // 진행률 계산
        float percentage = total > 0 ? (total_progress / total * 100.0) : 0.0;

        // 진행률 바 출력
        printf("\r[Progress] ");
        int bar_width = 40;
        int pos = total > 0 ? (int)(bar_width * total_progress / total) : 0;

        printf("[");
        for (int i = 0; i < bar_width; i++) {
            if (i < pos) printf("=");
            else if (i == pos) printf(">");
            else printf(" ");
        }

        printf("] %.1f%% (%d/%d chunks)", percentage, completed, total);
        fflush(stdout);

        usleep(100000);  // 100ms 대기 (0.1초)
    }

    printf("\n[Progress] Monitoring complete\n\n");
    return NULL;
}
