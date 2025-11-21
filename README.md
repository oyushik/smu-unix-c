# 멀티프로세스 파일 암호화/복호화 시스템

UNIX 시스템 프로그래밍 프로젝트 - 병렬 처리를 활용한 고성능 파일 암호화 도구

## 📌 프로젝트 개요

대용량 파일을 여러 워커 프로세스가 분산 처리하여 암호화/복호화하는 시스템입니다. fork(), pipe, mmap, pthread 등 UNIX 프로그래밍의 핵심 개념을 실제로 구현한 실용적인 프로젝트입니다.

## ✨ 주요 기능

- **병렬 처리**: 파일을 N개 청크로 분할하여 N개 워커 프로세스가 동시 처리
- **프로세스 간 통신**: 파이프(pipe)로 작업 할당 및 진행 상황 보고
- **메모리 매핑**: mmap을 사용한 효율적인 파일 데이터 공유
- **시그널 처리**: SIGINT, SIGUSR1/2로 프로세스 제어
- **성능 최적화**: 작은 파일은 자동으로 단일 프로세스 모드 사용

## 🚀 성능

**50MB 파일 암호화 벤치마크:**

- 1 worker: 221 MB/s
- 2 workers: 312 MB/s (1.41배)
- 4 workers: 393 MB/s (1.78배)
- 8 workers: 429 MB/s (1.94배)

## 🛠️ 빌드 및 실행

### 요구사항

- GCC compiler
- Linux/Unix 환경
- pthread 라이브러리

### 빌드

```bash
make
```

### 기본 사용법

```bash
# 파일 암호화
./crypto_system -e input.dat -k "mypassword"

# 파일 복호화
./crypto_system -d input.dat.encrypted -k "mypassword"

# 4개 워커로 암호화
./crypto_system -e largefile.dat -k "password" -w 4

# Verbose 모드 (시스템 정보 출력)
./crypto_system -e file.dat -k "key" -w 4 -v
```

### 옵션

- `-e <file>`: 파일 암호화
- `-d <file>`: 파일 복호화
- `-o <file>`: 출력 파일 (기본: 자동 생성)
- `-k <key>`: 암호화 키 (필수)
- `-w <num>`: 워커 프로세스 수 (기본: 4, 범위: 1-16)
- `-v`: Verbose 모드 (시스템 정보 출력)
- `-h`: 도움말 표시

## 🧪 테스트

### 기본 테스트

```bash
make test
```

### 성능 테스트

```bash
cd tests
./performance_test.sh
```

## 🎬 실행 및 테스트 가이드

### 1️⃣ 설치 및 빌드

```bash
# 프로젝트 클론 (또는 다운로드)
cd /path/to/project

# 빌드
make

# 빌드 확인
ls -lh crypto_system
```

**예상 출력:**
```
-rwxrwxr-x 1 user user 75K Nov 21 20:03 crypto_system
```

### 2️⃣ 기본 동작 확인

#### 도움말 확인
```bash
./crypto_system -h
```

#### 간단한 텍스트 파일 암호화/복호화
```bash
# 1. 테스트 파일 생성
echo "Hello, UNIX Programming!" > test.txt

# 2. 암호화 (단일 워커)
./crypto_system -e test.txt -k "mypassword" -w 1

# 3. 암호화된 파일 확인
ls -lh test.txt*
# test.txt
# test.txt.encrypted

# 4. 복호화
./crypto_system -d test.txt.encrypted -k "mypassword" -w 1

# 5. 결과 확인
cat test.txt.decrypted
# 출력: Hello, UNIX Programming!

# 6. 원본과 복호화 파일 비교
diff test.txt test.txt.decrypted && echo "✓ 성공!"
```

### 3️⃣ 멀티프로세스 성능 비교

#### 대용량 파일 생성 및 테스트
```bash
# 1. 10MB 테스트 파일 생성
dd if=/dev/urandom of=largefile.dat bs=1M count=10

# 2. 단일 워커로 암호화
time ./crypto_system -e largefile.dat -o output1.dat -k "test" -w 1
# 출력 예: Throughput: 206 MB/s

# 3. 4개 워커로 암호화
time ./crypto_system -e largefile.dat -o output4.dat -k "test" -w 4
# 출력 예: Throughput: 305 MB/s (약 1.5배 빠름!)

# 4. 8개 워커로 암호화
time ./crypto_system -e largefile.dat -o output8.dat -k "test" -w 8
# 출력 예: Throughput: 312 MB/s

# 5. 정리
rm -f largefile.dat output*.dat
```

### 4️⃣ Verbose 모드로 시스템 정보 확인

```bash
# 시스템 정보와 함께 실행
./crypto_system -e test.txt -k "password" -w 4 -v
```

**예상 출력:**
```
=== System Information ===
Process ID: 12345
Parent PID: 12344
Total RAM: 16384 MB
Free RAM: 8192 MB
Available CPUs: 8
==========================

=== Crypto System (Multi-Process Mode) ===
Input file: test.txt
Output file: test.txt.encrypted
Mode: Encryption
Master PID: 12345
Workers: 4
...
```

### 5️⃣ 자동화된 성능 테스트

```bash
# tests 디렉토리로 이동
cd tests

# 성능 테스트 스크립트 실행
./performance_test.sh

# 결과 파일 확인
cat performance_results.txt
```

**예상 출력:**
```
Performance Test Results - Wed Nov 21 20:00:00 KST 2024
========================================

File Size: 1MB
-------------------
  Workers: 1 | Time: 0.011s | Throughput: 85.47 MB/s
  Workers: 2 | Time: 0.011s | Throughput: 88.94 MB/s
  Workers: 4 | Time: 0.010s | Throughput: 91.43 MB/s
  Workers: 8 | Time: 0.010s | Throughput: 95.82 MB/s

File Size: 50MB
-------------------
  Workers: 1 | Time: 0.226s | Throughput: 221.12 MB/s
  Workers: 2 | Time: 0.160s | Throughput: 312.10 MB/s
  Workers: 4 | Time: 0.127s | Throughput: 393.61 MB/s
  Workers: 8 | Time: 0.116s | Throughput: 429.33 MB/s

✓ Correctness test PASSED
```

### 6️⃣ 실제 사용 시나리오

#### 시나리오 1: 중요 문서 암호화
```bash
# 문서 암호화
./crypto_system -e important_doc.pdf -k "SecurePassword123!" -w 4

# 암호화된 파일 전송/저장
# important_doc.pdf.encrypted 파일 사용

# 필요시 복호화
./crypto_system -d important_doc.pdf.encrypted -k "SecurePassword123!" -w 4
```

#### 시나리오 2: 대용량 비디오 파일 보호
```bash
# 500MB 비디오 파일 암호화 (8개 워커로 빠르게 처리)
./crypto_system -e video.mp4 -k "MySecretKey" -w 8 -v

# 처리 시간 확인
# 예상: 약 1-2초 내 완료
```

#### 시나리오 3: 성능 비교 실험
```bash
# 동일 파일을 다양한 워커 수로 테스트
for workers in 1 2 4 8; do
    echo "=== Testing with $workers workers ==="
    time ./crypto_system -e testfile.dat -o "test_w${workers}.dat" -k "key" -w $workers
    echo ""
done
```

### 7️⃣ 트러블슈팅

#### 문제: 빌드 에러
```bash
# 해결: 필요한 라이브러리 설치
sudo apt-get update
sudo apt-get install build-essential

# 재빌드
make clean
make
```

#### 문제: Permission denied
```bash
# 해결: 실행 권한 추가
chmod +x crypto_system
chmod +x tests/performance_test.sh
```

#### 문제: "File is small, using single process mode" 메시지
```
# 이것은 에러가 아닙니다!
# 4MB 이하 파일은 자동으로 단일 프로세스 모드를 사용합니다.
# 오버헤드를 줄여 더 빠른 성능을 제공합니다.
```

#### 문제: 암호화/복호화 실패
```bash
# 1. 키가 일치하는지 확인
./crypto_system -e file.txt -k "key1"
./crypto_system -d file.txt.encrypted -k "key1"  # 동일한 키 사용!

# 2. 파일 존재 여부 확인
ls -l file.txt

# 3. 디스크 공간 확인
df -h .
```

### 8️⃣ 정리 (Clean Up)

```bash
# 빌드 파일 정리
make clean

# 테스트 파일 정리
rm -f *.encrypted *.decrypted *.dat
rm -f tests/*.encrypted tests/*.decrypted

# 전체 재빌드
make
```

## 📂 프로젝트 구조

```
crypto_system/
├── src/
│   ├── main.c              # 메인 프로세스
│   ├── worker.c            # 워커 프로세스 로직
│   ├── crypto.c            # 암호화/복호화 알고리즘
│   ├── ipc.c               # 프로세스 간 통신
│   ├── progress.c          # 진행률 표시 스레드
│   ├── file_utils.c        # 파일 처리
│   ├── signal_handler.c    # 시그널 처리
│   └── system_info.c       # 시스템 정보
├── include/
│   └── crypto_system.h     # 공통 헤더
├── tests/
│   └── performance_test.sh # 성능 테스트 스크립트
├── Makefile
├── PROJECT_GUIDE.md        # 상세 개발 가이드
└── README.md
```

## 🔧 구현 기술

### UNIX 시스템 프로그래밍 개념

- **프로세스 생성/제어**: `fork()`, `exec()`, `wait()`, `waitpid()`
- **프로세스 간 통신**: `pipe()` (양방향 통신)
- **메모리 매핑**: `mmap()`, `munmap()`, `msync()`
- **파일 I/O**: `open()`, `read()`, `write()`, `close()`
- **시그널**: `signal()`, `sigaction()`, `kill()`
- **스레드**: `pthread_create()`, `pthread_mutex_t`
- **디렉터리**: `opendir()`, `readdir()`, `closedir()`
- **시스템 정보**: `stat()`, `sysinfo()`, `getpid()`, `getppid()`

## 📖 알고리즘

현재 XOR 기반 암호화를 사용합니다 (교육 목적). 실제 프로덕션 환경에서는 AES 등 강력한 암호화 알고리즘 사용을 권장합니다.

## 🎓 학습 목표

이 프로젝트를 통해 다음을 학습할 수 있습니다:

1. 멀티프로세스 프로그래밍의 이해
2. 프로세스 간 통신 (IPC) 구현
3. 메모리 매핑을 통한 효율적인 파일 처리
4. 시그널을 이용한 프로세스 제어
5. 병렬 처리를 통한 성능 최적화

---
