/*
 Copyright (c) 2026 ySpades Mobile developers.

 This file is part of ySpades Mobile.
 Provides minimal iconv compatibility for Android libc (bionic)
 where iconv is not built into the system library.
 */

#if defined(__ANDROID__) || defined(ANDROID)

#include <errno.h>
#include <stddef.h>
#include <string.h>

typedef void* iconv_t;

#ifdef __cplusplus
extern "C" {
#endif

iconv_t iconv_open(const char* tocode, const char* fromcode) {
	if (!tocode || !fromcode) {
		errno = EINVAL;
		return (iconv_t)(size_t)-1;
	}
	/* Return dummy valid descriptor */
	return (iconv_t)1;
}

size_t iconv(iconv_t cd, char** inbuf, size_t* inbytesleft, char** outbuf, size_t* outbytesleft) {
	(void)cd;
	if (!inbuf || !*inbuf)
		return 0;
	if (!inbytesleft || !outbuf || !*outbuf || !outbytesleft) {
		errno = EINVAL;
		return (size_t)-1;
	}
	size_t to_copy = *inbytesleft < *outbytesleft ? *inbytesleft : *outbytesleft;
	memcpy(*outbuf, *inbuf, to_copy);
	*inbuf += to_copy;
	*inbytesleft -= to_copy;
	*outbuf += to_copy;
	*outbytesleft -= to_copy;
	if (*inbytesleft > 0) {
		errno = E2BIG;
		return (size_t)-1;
	}
	return 0;
}

int iconv_close(iconv_t cd) {
	(void)cd;
	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* defined(__ANDROID__) || defined(ANDROID) */
