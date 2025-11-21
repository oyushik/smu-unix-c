#include "crypto_system.h"

// XOR 기반 암호화
// 교육 목적의 간단한 암호화 알고리즘
// 실제 프로덕션 환경에서는 AES 등의 강력한 알고리즘 사용 권장
void xor_encrypt(unsigned char *data, size_t size, const char *key) {
    size_t key_len = strlen(key);

    if (key_len == 0) {
        fprintf(stderr, "Error: Encryption key is empty\n");
        return;
    }

    // XOR 연산으로 각 바이트 암호화
    for (size_t i = 0; i < size; i++) {
        data[i] ^= key[i % key_len];
    }
}

// XOR 기반 복호화
// XOR은 대칭 연산이므로 암호화와 동일한 함수 사용
void xor_decrypt(unsigned char *data, size_t size, const char *key) {
    xor_encrypt(data, size, key);  // XOR은 자기 역함수
}
