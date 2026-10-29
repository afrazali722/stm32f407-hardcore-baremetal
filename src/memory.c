#include <stddef.h>
/* Minimal freestanding memory operations for standalone -nostdlib links.
 * No C standard library or HAL required. */
void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d=(unsigned char *)dst;
    const unsigned char *s=(const unsigned char *)src;
    while(n--) *d++=*s++;
    return dst;
}
void *memset(void *dst, int byte, size_t n) {
    unsigned char *d=(unsigned char *)dst;
    while(n--) *d++=(unsigned char)byte;
    return dst;
}
void __aeabi_memcpy(void *d, const void *s, size_t n) { (void)memcpy(d,s,n); }
void __aeabi_memcpy4(void *d, const void *s, size_t n) { (void)memcpy(d,s,n); }
void __aeabi_memcpy8(void *d, const void *s, size_t n) { (void)memcpy(d,s,n); }
void __aeabi_memclr(void *d, size_t n) { (void)memset(d,0,n); }
void __aeabi_memclr4(void *d, size_t n) { (void)memset(d,0,n); }
void __aeabi_memclr8(void *d, size_t n) { (void)memset(d,0,n); }
