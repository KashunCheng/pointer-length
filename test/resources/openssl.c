//{"func": "ossl_punycode_decode"}
#include "string.h"
#define LABEL_BUF_SIZE 512

int ossl_punycode_decode(const char *pEncoded, const size_t enc_len,
                         unsigned int *pDecoded, unsigned int *pout_length)
{
  return 0;
}

int ossl_a2ulabel(const char *in, char *out, size_t outlen)
{
  /*-
   * Domain name has some parts consisting of ASCII chars joined with dot.
   * If a part is shorter than 5 chars, it becomes U-label as is.
   * If it does not start with xn--,    it becomes U-label as is.
   * Otherwise we try to decode it.
   */
  const char *inptr = in;
  int result = 1;
  unsigned int i;
  unsigned int buf[LABEL_BUF_SIZE];      /* It's a hostname */

  while (1) {
    char *tmpptr = strchr(inptr, '.');
    size_t delta = tmpptr != NULL ? (size_t)(tmpptr - inptr) : strlen(inptr);

    if (0) {
      result = 0;
    } else {
      unsigned int bufsize = LABEL_BUF_SIZE;

      if (ossl_punycode_decode(inptr + 4, delta - 4, buf, &bufsize) <= 0) {
        result = -1;
        goto end;
      }
    }
  }
  end:
  return result;
}