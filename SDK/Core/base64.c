
#include "core.h"

static const char *base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";


static inline int is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

static int Base64encode_len (size_t len)
{
    return ((len + 2) / 3 * 4) + 1; // +1 for null
}


char *base64_encode_a (const void *data, size_t in_len, reply size_t *numbytes)
{
	const byte *src;
	int outlen = (in_len + 2) / 3 * 4;
	char *out = calloc (outlen + 1 /* for the null*/, 1);
	char *dst = out;
	int remaining;

	for (src = data, dst = out, remaining = in_len; remaining > 0; dst += 4, src +=3, remaining -= 3) {
		dst[0] = /* can't fail */      base64_chars[((src[0] & 0xfc) >> 2)];
		dst[1] = /* can't fail */      base64_chars[((src[0] & 0x03) << 4) + ((src[1] & 0xf0) >> 4)];
		dst[2] = remaining < 2 ? '=' : base64_chars[((src[1] & 0x0f) << 2) + ((src[2] & 0xc0) >> 6)];
		dst[3] = remaining < 3 ? '=' : base64_chars[((src[2] & 0x3f)     )];
	}

	if (dst - out != outlen)
		log_fatal ("base64_encode_a: dst - out != outlen");
	NOT_MISSING_ASSIGN(numbytes, dst-out);
	return out;
}


/* aaaack but it's fast and const should make it shared text page. */
static const unsigned char pr2six[256] =
{
    /* ASCII table */
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

static int Base64decode_len(const char *bufcoded)
{
    int nbytesdecoded;
    register const unsigned char *bufin;
    register int nprbytes;

    bufin = (const unsigned char *) bufcoded;
    while (pr2six[*(bufin++)] <= 63);

    nprbytes = (bufin - (const unsigned char *) bufcoded) - 1;
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;

    return nbytesdecoded + 1;
}

void *base64_decode_a (const char *encoded_string, reply size_t *numbytes)
{
	size_t outlen = Base64decode_len (encoded_string);
	unsigned char *_ret = calloc (outlen, 1);
	unsigned char *ret = _ret;
	size_t in_len = strlen(encoded_string);
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3] = {0}; // gcc says char_array_3 used uninitialized, but scenario looks impossible


	while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i ==4) {
			for (i = 0; i <4; i++)
				//char_array_4[i] = base64_chars.find(char_array_4[i]);
				char_array_4[i] = (int)(strchr (base64_chars, char_array_4[i]) - base64_chars);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				*ret++ = char_array_3[i];

			i = 0;
		}
	}

	if (i) {
		for (j = i; j <4; j++)
			char_array_4[j] = 0;

		for (j = 0; j <4; j++)
			//char_array_4[j] = base64_chars.find(char_array_4[j]);
			char_array_4[j] = (int)(strchr (base64_chars, char_array_4[j]) - base64_chars);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++)
			*ret++ = char_array_3[j];
	}


	NOT_MISSING_ASSIGN(numbytes, (ret - _ret));
	return (void *)_ret;
}



