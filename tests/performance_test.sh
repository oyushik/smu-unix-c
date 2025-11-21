#!/bin/bash

# 멀티프로세스 파일 암호화/복호화 시스템 성능 테스트 스크립트

echo "================================================"
echo "Multi-Process Crypto System - Performance Test"
echo "================================================"
echo ""

# 프로그램 경로
CRYPTO_SYSTEM="../crypto_system"

if [ ! -f "$CRYPTO_SYSTEM" ]; then
    echo "Error: crypto_system not found!"
    echo "Please run 'make' first."
    exit 1
fi

# 테스트 파일 크기 (MB)
FILE_SIZES=(1 5 10 50)

# 워커 수
WORKER_COUNTS=(1 2 4 8)

# 결과 저장
RESULTS_FILE="performance_results.txt"
echo "Performance Test Results - $(date)" > $RESULTS_FILE
echo "========================================" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE

# 각 파일 크기별 테스트
for SIZE in "${FILE_SIZES[@]}"; do
    TEST_FILE="test_${SIZE}mb.dat"

    echo "=== Testing with ${SIZE}MB file ==="
    echo "" | tee -a $RESULTS_FILE
    echo "File Size: ${SIZE}MB" | tee -a $RESULTS_FILE
    echo "-------------------" | tee -a $RESULTS_FILE

    # 테스트 파일 생성
    echo "Creating ${SIZE}MB test file..."
    dd if=/dev/urandom of=$TEST_FILE bs=1M count=$SIZE 2>/dev/null

    if [ ! -f "$TEST_FILE" ]; then
        echo "Failed to create test file!"
        continue
    fi

    # 각 워커 수별 테스트
    for WORKERS in "${WORKER_COUNTS[@]}"; do
        echo "Testing with $WORKERS worker(s)..."

        # 암호화 테스트
        ENCRYPTED_FILE="${TEST_FILE}.encrypted"
        rm -f $ENCRYPTED_FILE

        # 시간 측정
        START_TIME=$(date +%s.%N)
        $CRYPTO_SYSTEM -e $TEST_FILE -o $ENCRYPTED_FILE -k "testpassword" -w $WORKERS > /dev/null 2>&1
        EXIT_CODE=$?
        END_TIME=$(date +%s.%N)

        if [ $EXIT_CODE -eq 0 ]; then
            ELAPSED=$(echo "$END_TIME - $START_TIME" | bc)
            THROUGHPUT=$(echo "scale=2; $SIZE / $ELAPSED" | bc)

            echo "  Workers: $WORKERS | Time: ${ELAPSED}s | Throughput: ${THROUGHPUT} MB/s" | tee -a $RESULTS_FILE
        else
            echo "  Workers: $WORKERS | FAILED" | tee -a $RESULTS_FILE
        fi

        # 정리
        rm -f $ENCRYPTED_FILE
    done

    echo "" | tee -a $RESULTS_FILE

    # 테스트 파일 삭제
    rm -f $TEST_FILE
done

echo "========================================" | tee -a $RESULTS_FILE
echo "Performance test complete!" | tee -a $RESULTS_FILE
echo "Results saved to: $RESULTS_FILE" | tee -a $RESULTS_FILE
echo ""

# 간단한 정확성 테스트
echo "=== Correctness Test ===" | tee -a $RESULTS_FILE
echo "Testing encryption/decryption accuracy..." | tee -a $RESULTS_FILE

TEST_FILE="correctness_test.dat"
dd if=/dev/urandom of=$TEST_FILE bs=1M count=2 2>/dev/null

$CRYPTO_SYSTEM -e $TEST_FILE -k "test123" -w 4 > /dev/null 2>&1
$CRYPTO_SYSTEM -d ${TEST_FILE}.encrypted -k "test123" -w 4 > /dev/null 2>&1

if cmp -s $TEST_FILE ${TEST_FILE}.decrypted; then
    echo "✓ Correctness test PASSED" | tee -a $RESULTS_FILE
else
    echo "✗ Correctness test FAILED" | tee -a $RESULTS_FILE
fi

# 정리
rm -f $TEST_FILE ${TEST_FILE}.encrypted ${TEST_FILE}.decrypted

echo "" | tee -a $RESULTS_FILE
echo "All tests complete!"
