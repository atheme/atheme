#ifndef ATHEME_LAC_EXPLICIT_BZERO_H
#define ATHEME_LAC_EXPLICIT_BZERO_H 1

extern void *(* volatile volatile_memset)(void *, int, size_t);

#endif /* !ATHEME_LAC_EXPLICIT_BZERO_H */
