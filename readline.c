#include <stdio.h>
#include <stdlib.h>

// reads a line of input from the socket
// reference: www.scs.stanford.edu/07wi-cs244b/refs/net2.pdf
char * readline(int s) {
	char *buf = NULL, *nbuf;
	int buf_pos = 0, buf_len = 0;
	int i, n;
	for (;;) {
		if (buf_pos == buf_len) {
			buf_len = buf_len ? buf_len << 1 : 4;
			nbuf = realloc(buf, buf_len);
			if (!nbuf) {
				free(buf);
				return NULL ;
			}
			buf = nbuf;
		}

		// reads data to buffer
		n = read(s, buf + buf_pos, buf_len - buf_pos);
		if (n <= 0) {
			if (n < 0)
				perror("read");
			else
				fprintf(stderr, "read: EOF\n");
			free(buf);
			return NULL ;
		}

		// returns buffer if end of line reached, also terminate it with a null
		for (i = buf_pos; i < buf_pos + n; i++)
			if (buf[i] == '\0' || buf[i] == '\r' || buf[i] == '\n') {
				buf[i] = '\0';
				return buf;
			}

		buf_pos += n;
	}
}
