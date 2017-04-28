/**********************************************************************
  Copyright(c) 2011-2016 Intel Corporation All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "igzip_lib.h"

#define BUF_SIZE 8192
const uint32_t gzip_hdr_bytes = 10;

int compress(FILE *in, FILE *out, int level, int gzip_flag) {
	struct isal_zstream stream;
	uint8_t inbuf[BUF_SIZE], outbuf[BUF_SIZE];
	// fflush(0);
	printf("Using igzip compression\n");
	isal_deflate_init(&stream);
	stream.end_of_stream = 0;
	stream.flush = NO_FLUSH;
	// Set gzip header
	stream.gzip_flag = gzip_flag;
	stream.level = level;

	if (level == 1) {
		stream.level = 1;
		stream.level_buf = malloc(ISAL_DEF_LVL1_DEFAULT);
		stream.level_buf_size = ISAL_DEF_LVL1_DEFAULT;
		if (stream.level_buf == 0) {
			printf("Failed to allocate level compression buffer\n");
			exit(0);
		}
	}

	do {
		stream.avail_in = (uint32_t) fread(inbuf, 1, BUF_SIZE, in);
		/* TODO EOF error handle*/
		stream.end_of_stream = feof(in) ? 1 : 0;
		stream.next_in = inbuf;
		do {
			stream.avail_out = BUF_SIZE;
			stream.next_out = outbuf;

			isal_deflate(&stream);

			fwrite(outbuf, 1, BUF_SIZE - stream.avail_out, out);
		} while (stream.avail_out == 0);

		assert(stream.avail_in == 0);
	} while (stream.internal_state.state != ZSTATE_END);

	fclose(out);
	fclose(in);
	return 0;
}

int decompress(FILE *in, FILE *out, int level, int gzip_flag) {
	struct inflate_state state;
	uint8_t inbuf[BUF_SIZE], outbuf[BUF_SIZE];
	// fflush(0);
	int ret = 0;
	int count = 0;
	printf("Using igzip decompression\n");
	isal_inflate_init(&state);
	do {
		state.avail_in = (uint32_t) fread(inbuf, 1, BUF_SIZE, in);
		/* skip gzip header */
		if (gzip_flag == IGZIP_GZIP && count == 0) {
			state.next_in = inbuf + gzip_hdr_bytes;
			state.avail_in -= gzip_hdr_bytes;
		}
		if (state.avail_in == 0)
            break;
		count += 1;
		do {
			state.avail_out = BUF_SIZE;
			state.next_out = outbuf;
			ret = isal_inflate(&state);
			if (ret) {
				printf("Error in decompression with error %d\n", ret);
				break;
			}
			fwrite(outbuf, 1, BUF_SIZE - state.avail_out, out);
		} while (state.avail_out == 0);
	} while (ret != ISAL_END_INPUT);
	fclose(out);
	fclose(in);
	return ret;
}


int main(int argc, char *argv[])
{
	FILE *in, *out;
	int level = 1;
	int flag = 1;
	if (argc < 3) {
		fprintf(stderr, "Usage: igzip_example infile outfile\n");
		exit(0);
	}
	in = fopen(argv[1], "rb");
	if (!in) {
		fprintf(stderr, "Can't open %s for reading\n", argv[1]);
		exit(0);
	}
	out = fopen(argv[2], "wb");
	if (!out) {
		fprintf(stderr, "Can't open %s for writing\n", argv[2]);
		exit(0);
	}
	if (argc > 3) {
		flag = 0;
	}
	printf("Current Flag %d\n", flag);
	printf("igzip_example\nWindow Size: %d K\n", IGZIP_HIST_SIZE / 1024);
	if (flag) 
		compress(in, out, level, IGZIP_GZIP);
	else
		decompress(in, out, level, IGZIP_GZIP);
}
