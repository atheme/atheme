#include "atheme.h"

#ifdef HAVE_LIBSODIUM
#  include <sodium/core.h>
#endif /* HAVE_LIBSODIUM */

int
main(int argc, const char *argv)
{
#ifdef HAVE_LIBSODIUM
	if (sodium_init() == -1)
	{
		(void) fprintf(stderr, "Error: sodium_init() failed!\n");
		return EXIT_FAILURE;
	}
#endif /* HAVE_LIBSODIUM */

	char b64[] = "Q2hyaXNUZXN0AENocmlzVGVzdABwbXpqZ3VseGF5ZWJjcGJ3cXFkaA==";
	char b64out[BUFSIZE];
	char b64pristine[] = "ChrisTest\0ChrisTest\0pmzjgulxayebcpbwqqdh";

	size_t rc = base64_decode(b64, b64out, sizeof b64out);

	if (rc == (size_t) -1)
	{
		fprintf(stderr, "Failed to decode data!\n");
		return EXIT_FAILURE;
	}

	printf("decode: %d sz: %zu\n", rc, sizeof(b64pristine));
	printf("b64pri: %s:%s:%s\n", b64pristine, b64pristine + 10, b64pristine + 20);
	printf("b64out: %s:%s:%s\n", b64out, b64out + 10, b64out + 20);

	if (memcmp(b64out, b64pristine, sizeof(b64pristine)))
	{
		printf("FAIL.\n");
		return EXIT_FAILURE;
	}
	else
	{
		printf("PASS.\n");
		return EXIT_SUCCESS;
	}
}
