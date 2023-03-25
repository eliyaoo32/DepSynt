/***************************************************************************
Copyright (c) 2006-2015, Armin Biere, Johannes Kepler University.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
***************************************************************************/

#include "aiger.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>

#define PERCENT(a,b) ((b) ? (100.0 * (a))/ (double)(b) : 0.0)

typedef struct stream stream;
typedef struct memory memory;

struct stream
{
  double bytes;
  FILE *file;
};

struct memory
{
  double bytes;
  double max;
};

static void *
aigtoaig_malloc (memory * m, size_t bytes)
{
  m->bytes += bytes;
  assert (m->bytes);
  if (m->bytes > m->max)
    m->max = m->bytes;
  return malloc (bytes);
}

static void
aigtoaig_free (memory * m, void *ptr, size_t bytes)
{
  assert (m->bytes >= bytes);
  m->bytes -= bytes;
  free (ptr);
}

static int
aigtoaig_put (char ch, stream * stream)
{
  int res;

  res = putc ((unsigned char) ch, stream->file);
  if (res != EOF)
    stream->bytes++;

  return res;
}

static int
aigtoaig_get (stream * stream)
{
  int res;

  res = getc (stream->file);
  if (res != EOF)
    stream->bytes++;

  return res;
}

static double
size_of_file (const char *file_name)
{
  struct stat buf;
  buf.st_size = 0;
  stat (file_name, &buf);
  return buf.st_size;
}

static void
die (const char *fmt, ...)
{
  va_list ap;
  fputs ("*** [aigtoaig] ", stderr);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  exit (1);
}

int aigtoaig(FILE* src_file, FILE* dst_file, int ascii)
{
  const char *src, *dst, *src_name, *dst_name, *error;
  int verbose, strip, res;
  stream reader, writer;
  unsigned num_outputs;
  int remove_outputs;
  aiger_mode mode;
  memory memory;
  aiger *aiger;
  unsigned i;

  res = verbose =  0;
  src_name = dst_name = src = dst = 0;
  remove_outputs = 0;
  num_outputs = 0;
  strip = 0;

  if (dst && ascii)
    die ("'dst' file and '-a' specified");

  if (!dst && !ascii && isatty (1))
    ascii = 1;

  if (src && dst && !strcmp (src, dst))
    die ("identical 'src' and 'dst' file");

  memory.max = memory.bytes = 0;
  aiger = aiger_init_mem (&memory,
			  (aiger_malloc) aigtoaig_malloc,
			  (aiger_free) aigtoaig_free);
  if (src)
    {
      error = aiger_open_and_read_from_file (aiger, src);
      if (error)
	{
	READ_ERROR:
	  fprintf (stderr, "*** [aigtoaig] %s\n", error);
	  res = 1;
	}
      else
	{
	  reader.bytes = size_of_file (src);

	  if (verbose)
	    {
	      fprintf (stderr,
		       "[aigtoaig] read from '%s' (%.0f bytes)\n",
		       src, (double) reader.bytes);
	      fflush (stderr);
	    }
	}
    }
  else
    {
      reader.file = src_file;
      reader.bytes = 0;

      error = aiger_read_generic (aiger, &reader, (aiger_get) aigtoaig_get);

      if (error) {
          goto READ_ERROR;
      }

      if (verbose)
	{
	  fprintf (stderr,
		   "[aigtoaig] read from '<stdin>' (%.0f bytes)\n",
		   (double) reader.bytes);
	  fflush (stderr);
	}
    }

  if (!res)
    {
      if (strip)
	{
	  i = aiger_strip_symbols_and_comments (aiger);

	  if (verbose)
	    {
	      fprintf (stderr, "[aigtoaig] stripped %u symbols\n", i);
	      fflush (stderr);
	    }
	}

      if (remove_outputs)
	{
	  if (verbose)
	    {
	      fprintf (stderr,
		       "[aigtoaig] removing %u outputs\n",
		       aiger->num_outputs);
	      fflush (stderr);
	  }
	  num_outputs = aiger->num_outputs;
	  aiger->num_outputs = 0;
	}

      if (dst)
	{
	  if (aiger_open_and_write_to_file (aiger, dst))
	    {
	      writer.bytes = size_of_file (dst);

	      if (verbose)
		{
		  fprintf (stderr,
			   "[aigtoaig] wrote to '%s' (%.0f bytes)\n",
			   dst, (double) writer.bytes);
		  fflush (stderr);
		}
	    }
	  else
	    {
	      unlink (dst);
	    WRITE_ERROR:
	      fprintf (stderr, "*** [aigtoai]: write error\n");
	      res = 1;
	    }
	}
      else
	{
	  writer.file = dst_file;
	  writer.bytes = 0;

	  if (ascii)
	    mode = aiger_ascii_mode;
	  else
	    mode = aiger_binary_mode;

	  if (!aiger_write_generic (aiger, mode,
				    &writer, (aiger_put) aigtoaig_put))
	    goto WRITE_ERROR;

	  if (verbose)
	    {
	      fprintf (stderr,
		       "[aigtoaig] wrote to '<stdout>' (%.0f bytes)\n",
		       (double) writer.bytes);
	      fflush (stderr);
	    }
	}
    }

  if (remove_outputs)
    aiger->num_outputs = num_outputs;

  aiger_reset (aiger);

  if (!res && verbose)
    {
      if (reader.bytes > writer.bytes)
	fprintf (stderr, "[aigtoaig] deflated to %.1f%%\n",
		 PERCENT (writer.bytes, reader.bytes));
      else
	fprintf (stderr, "[aigtoaig] inflated to %.1f%%\n",
		 PERCENT (writer.bytes, reader.bytes));

      fprintf (stderr,
	       "[aigtoaig] allocated %.0f bytes maximum\n", memory.max);

      fflush (stderr);
    }

  return res;
}
