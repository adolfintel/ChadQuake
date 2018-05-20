
#ifndef __BASE_64_H__
#define __BASE_64_H__

char *base64_encode_a (const void *data, size_t in_len, reply size_t *numbytes);
void *base64_decode_a (const char *encoded_string, reply size_t *numbytes);

#endif // ! __BASE_64_H__