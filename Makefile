CC = gcc
CFLAGS = -Wall -Wextra -g -I./include
LDFLAGS = -lpthread

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = .
TEST_DIR = tests

# 소스 파일 목록
SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/crypto.c \
          $(SRC_DIR)/file_utils.c \
          $(SRC_DIR)/ipc.c

# 추가 소스 (2단계 이후)
SOURCES_PHASE2 = $(SRC_DIR)/worker.c \
                 $(SRC_DIR)/signal_handler.c \
                 $(SRC_DIR)/progress.c \
                 $(SRC_DIR)/system_info.c

# 오브젝트 파일
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
OBJECTS_PHASE2 = $(SOURCES_PHASE2:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# 최종 실행 파일
TARGET = $(BIN_DIR)/crypto_system

# 테스트 실행 파일
TEST_CRYPTO = $(BIN_DIR)/test_crypto

.PHONY: all clean test phase1 phase2 help

# 기본 타겟
all: $(TARGET)

# Phase 1: 단일 프로세스 버전
phase1: $(TARGET)

# Phase 2 이후: 전체 기능
phase2: SOURCES += $(SOURCES_PHASE2)
phase2: $(TARGET)

# 메인 프로그램 빌드
$(TARGET): $(OBJECTS)
	@echo "Linking $@..."
	@# Phase 2 오브젝트 파일들이 존재하는지 확인하고 추가
	@PHASE2_OBJS=""; \
	for file in $(SOURCES_PHASE2); do \
		objfile=$(OBJ_DIR)/$$(basename $$file .c).o; \
		if [ -f $$file ]; then \
			$(MAKE) $$objfile 2>/dev/null || true; \
		fi; \
		if [ -f $$objfile ]; then \
			PHASE2_OBJS="$$PHASE2_OBJS $$objfile"; \
		fi; \
	done; \
	$(CC) $(OBJECTS) $$PHASE2_OBJS -o $@ $(LDFLAGS)
	@echo "Build complete: $@"

# 오브젝트 파일 생성
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# obj 디렉토리 생성
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# 암호화 테스트 프로그램
$(TEST_CRYPTO): $(TEST_DIR)/test_crypto.c $(OBJ_DIR)/crypto.o
	@echo "Building test_crypto..."
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# 클린
clean:
	@echo "Cleaning..."
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET) $(TEST_CRYPTO)
	rm -f *.encrypted *.decrypted
	rm -f $(TEST_DIR)/*.encrypted $(TEST_DIR)/*.decrypted
	@echo "Clean complete"

# 테스트 실행
test: $(TARGET)
	@echo "Running basic functionality tests..."
	@echo ""
	@echo "=== Test 1: Help message ==="
	./$(TARGET) -h
	@echo ""
	@echo "=== Test 2: Creating test file (1MB) ==="
	dd if=/dev/urandom of=test_1mb.dat bs=1M count=1 2>/dev/null
	@echo ""
	@echo "=== Test 3: Encryption ==="
	./$(TARGET) -e test_1mb.dat -k "testpassword123" -v
	@echo ""
	@echo "=== Test 4: Decryption ==="
	./$(TARGET) -d test_1mb.dat.encrypted -k "testpassword123" -v
	@echo ""
	@echo "=== Test 5: Verify original vs decrypted ==="
	cmp test_1mb.dat test_1mb.dat.decrypted && echo "✓ Files match! Encryption/Decryption works correctly." || echo "✗ Files don't match! There's a problem."
	@echo ""
	@echo "=== Cleaning up test files ==="
	rm -f test_1mb.dat test_1mb.dat.encrypted test_1mb.dat.decrypted

# 성능 테스트 (대용량 파일)
perftest: $(TARGET)
	@echo "Creating 10MB test file..."
	dd if=/dev/urandom of=test_10mb.dat bs=1M count=10 2>/dev/null
	@echo ""
	@echo "=== Performance Test: 10MB file ==="
	time ./$(TARGET) -e test_10mb.dat -k "password" -w 1
	@echo ""
	@echo "Cleaning up..."
	rm -f test_10mb.dat test_10mb.dat.encrypted

# 도움말
help:
	@echo "Makefile for Crypto System"
	@echo ""
	@echo "Targets:"
	@echo "  make              - Build the crypto_system program (all available features)"
	@echo "  make phase1       - Build Phase 1 (single process version)"
	@echo "  make phase2       - Build Phase 2+ (multiprocess version)"
	@echo "  make test         - Run basic functionality tests"
	@echo "  make perftest     - Run performance test with 10MB file"
	@echo "  make clean        - Remove all build artifacts and test files"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Example usage:"
	@echo "  make && ./crypto_system -e myfile.txt -k \"password\""
