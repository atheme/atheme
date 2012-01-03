#include "atheme.h"

int main(int argc, const char *argv)
{
	char b64[] = "Q2hyaXNUZXN0AENocmlzVGVzdABwbXpqZ3VseGF5ZWJjcGJ3cXFkaA==";
	char b64out[BUFSIZE];
	char b64pristine[] = "ChrisTest\0ChrisTest\0pmzjgulxayebcpbwqqdh";
	int rc;

	rc = base64_decode(b64, b64out, BUFSIZE);

	printf("decode: %d sz: %zu\n", rc, sizeof(b64pristine));
	printf("b64pri: %s:%s:%s\n", b64pristine, b64pristine + 10, b64pristine + 20);
	printf("b64out: %s:%s:%s\n", b64out, b64out + 10, b64out + 20);

	if (memcmp(b64out, b64pristine, sizeof(b64pristine)))
	{
		printf("FAIL.\n");
	}
	else
	{
		printf("PASS.\n");
	}
}
