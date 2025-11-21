#include "crypto_system.h"

// 전역 변수 (extern으로 main.c에서 선언된 것 사용)
extern pid_t worker_pids[];
extern SharedData *shared_data;
extern int pipes_to_workers[][2];
extern int pipes_from_workers[][2];

// 시그널 핸들러 (교안 ch08 기반)
void signal_handler(int signo) {
    switch(signo) {
        case SIGINT:
            printf("\n[Signal] Received SIGINT (Ctrl+C). Shutting down gracefully...\n");

            // 공유 메모리에 종료 플래그 설정
            if (shared_data) {
                pthread_mutex_lock(&shared_data->mutex);
                shared_data->shutdown_flag = 1;
                pthread_mutex_unlock(&shared_data->mutex);
            }

            // 모든 워커에게 SIGTERM 전송
            for (int i = 0; i < MAX_WORKERS; i++) {
                if (worker_pids[i] > 0) {
                    printf("[Signal] Sending SIGTERM to worker %d (PID %d)\n",
                           i, worker_pids[i]);
                    kill(worker_pids[i], SIGTERM);
                }
            }

            // 정리 후 종료
            if (shared_data) {
                cleanup_shared_memory(shared_data);
            }

            printf("[Signal] Cleanup complete. Exiting.\n");
            exit(130);  // 128 + SIGINT(2)
            break;

        case SIGUSR1:
            printf("[Signal] Process %d received SIGUSR1 - Pausing\n", getpid());
            // 워커 프로세스가 이 시그널을 받으면 일시정지
            pause();
            break;

        case SIGUSR2:
            printf("[Signal] Process %d received SIGUSR2 - Resuming\n", getpid());
            // SIGUSR1의 pause()에서 깨어남
            break;

        case SIGTERM:
            printf("[Signal] Process %d received SIGTERM - Terminating\n", getpid());
            exit(0);
            break;
    }
}

// SIGCHLD 핸들러: 자식 프로세스 종료 감지
void sigchld_handler(int signo) {
    (void)signo;  // unused parameter

    pid_t pid;
    int status;

    // 종료된 모든 자식 프로세스 처리 (WNOHANG: 논블로킹)
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status != 0) {
                fprintf(stderr, "[Signal] Worker %d exited with error: %d\n",
                        pid, exit_status);
            }
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr, "[Signal] Worker %d killed by signal %d\n",
                    pid, WTERMSIG(status));
        }
    }
}

// 시그널 핸들러 설정 (교안 ch08 기반)
void setup_signal_handlers(void) {
    struct sigaction sa;

    // SIGINT 핸들러 (Ctrl+C)
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction SIGINT");
    }

    // SIGUSR1, SIGUSR2 핸들러 (프로세스 제어)
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1");
    }
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction SIGUSR2");
    }

    // SIGTERM 핸들러
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction SIGTERM");
    }

    // SIGCHLD 핸들러
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction SIGCHLD");
    }
}
