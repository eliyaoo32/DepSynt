/***************************************************************************
Copyright (c) 2006-2011, Armin Biere, Johannes Kepler University.
Copyright (c) 2006, Marc Herbstritt, University of Freiburg

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
#include "aigtoblif.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

static const char *
on (unsigned i, aiger *mgr, char buffer[20])
{
  assert (mgr && i < mgr->num_outputs);
  if (mgr->outputs[i].name)
    return mgr->outputs[i].name;

  sprintf (buffer, "o%u", i);

  return buffer;
}

static int
isblifsymchar (int ch)
{
  if (isspace (ch)) return 0;
  if (ch == '.') return 0;
  if (ch == '\\') return 0;
  if (!isprint (ch)) return 0;
  return 1;
}

static void
print_mangled (const char * name, FILE * file) 
{
  const char * p;
  char ch;
  for (p = name; (ch = *p); p++)
    if (isblifsymchar (ch))
      fputc (ch, file);
    else
      fprintf (file, "\\%0X", ch);
}

static void
pl (unsigned lit, FILE *file, aiger *mgr, int count)
{
  const char *name;
  char ch;
  int i;

  if (lit == 0) {
    fputs ("c0", file);
  } else if (lit == 1) {
    fputs ("c1", file);
  } else if ((lit & 1))
    putc ('!', file), pl (lit - 1, file, mgr, count);
  else if ((name = aiger_get_symbol (mgr, lit)))
    {
      print_mangled (name, file);
    }
  else
    {
      if (aiger_is_input (mgr, lit))
	ch = 'i';
      else if (aiger_is_latch (mgr, lit))
	ch = 'l';
      else
	{
	  assert (aiger_is_and (mgr, lit));
	  ch = 'a';
	}

      for (i = 0; i <= count; i++)
	fputc (ch, file);

      fprintf (file, "%u", lit);
    }
}

static int
count_ch_prefix (const char *str, char ch)
{
  const char *p;

  assert (ch);
  for (p = str; *p == ch; p++)
    ;

  if (*p && !isdigit (*p))
    return 0;

  return p - str;
}

static int
setupcount (aiger *mgr)
{
  const char *symbol;
  unsigned i;
  int tmp;

  int count = 0;
  for (i = 1; i <= mgr->maxvar; i++)
    {
      symbol = aiger_get_symbol (mgr, 2 * i);
      if (!symbol)
	continue;

      if ((tmp = count_ch_prefix (symbol, 'i')) > count)
	count = tmp;

      if ((tmp = count_ch_prefix (symbol, 'l')) > count)
	count = tmp;

      if ((tmp = count_ch_prefix (symbol, 'o')) > count)
	count = tmp;

      if ((tmp = count_ch_prefix (symbol, 'a')) > count)
	count = tmp;
    }
  return count;
}


extern int aigtoblif(FILE* aig_src, FILE* file, const char* model_name)
{
    aiger* mgr;
  unsigned i, j, latch_helper_cnt;
  const char *dst, *error;
  int *latch_helper = 0;
  int res, strip, ag;
  int count = 0;
  int require_const0 = 1;
  int require_const1 = 1;
  latch_helper_cnt = 0;
  strip = 0;
  res = 0;
  ag = 0;
   char buffer[20];

  mgr = aiger_init ();

  error = aiger_read_from_file (mgr, aig_src);

  if (error)
    {
      fprintf (stderr, "=[aigtoblif] %s\n", error);
      res = 1;
    }
  else
    {
      if (strip)
	aiger_strip_symbols_and_comments (mgr);
      else
	count = setupcount (mgr);

      fputs (".model ", file), fputs (model_name, file), fputs ("\n", file);
      fputs (".inputs ", file);
      for (i = 0; i < mgr->num_inputs; i++)
	{
	  pl (mgr->inputs[i].lit, file, mgr, count), fputs (" ", file);

	  if ((i + 1) % 10 == 0 && (i < (mgr->num_inputs - 1)))
	    fputs ("\\\n", file);
	}
      fputs ("\n", file);
      fputs (".outputs ", file);
      for (i = 0; i < mgr->num_outputs; i++)
	{
	  fputs (on (i, mgr, buffer), file), fputs (" ", file);

	  if ((i + 1) % 10 == 0 && (i < (mgr->num_outputs - 1)))
	    fputs ("\\\n", file);
	}
      fputs ("\n", file);

      /* this is a non-efficient hack for assuring that BLIF-inverters
       * are only inserted once even when multiple latches have the same
       * next-state-function!
       * latch_helper[i] stores the i-th AIG-index for which a INV has to
       * be inserted. checking duplicates is simply done by comparing 
       * a new potential INV with all other INV that must already be inserted!
       */
      latch_helper = calloc (mgr->num_latches, sizeof (latch_helper[0]));
      for (i = 0; i < mgr->num_latches; i++)
	{
	  latch_helper[i] = 0;
	  if (mgr->latches[i].next == aiger_false)
	    {
	      require_const0 = 1;
	    }
	  if (mgr->latches[i].next == aiger_true)
	    {
	      require_const1 = 1;
	    }

	  /* this case normally makes no sense, but you never know ... */
	  if (mgr->latches[i].next == aiger_false ||
	      mgr->latches[i].next == aiger_true)
	    {
	      fputs (".latch ", file),
		(mgr->latches[i].next == aiger_false) ? fputs ("c0", file) : fputs ("c1", file),
		fputs (" ", file), pl (mgr->latches[i].lit, file, mgr, count), fputs (" 0\n", file);
	    }
	  /* this should be the general case! */
	  else
	    {
	      if (!aiger_sign (mgr->latches[i].next))
		{
		  fputs (".latch ", file), pl (mgr->latches[i].next, file, mgr, count), fputs (" ", file),
		    pl (mgr->latches[i].lit, file, mgr, count), fputs (" 0\n", file);
		}
	      else
		{
		  /* add prefix 'n' to inverted AIG nodes. 
		   * corresponding inverters are inserted below!
		   */
		  fputs (".latch n", file), pl (aiger_strip (mgr->latches[i].next), file, mgr, count),
		    fputs (" ", file), pl (mgr->latches[i].lit, file, mgr, count), fputs (" 0\n", file);
		  int already_done = 0;
		  for (j = 0; j < latch_helper_cnt && !already_done; j++)
		    {
		      if (mgr->latches[latch_helper[j]].next ==
			  mgr->latches[i].next)
			{
			  already_done = 1;
			}
		    }
		  if (!already_done)
		    {
		      latch_helper[latch_helper_cnt] = i;
		      latch_helper_cnt++;
		    }
		}
	    }
	}

      for (i = 0; i < mgr->num_ands; i++)
	{
	  aiger_and *n = mgr->ands + i;

	  unsigned rhs0 = n->rhs0;
	  unsigned rhs1 = n->rhs1;

	  fputs (".names ", file), pl (aiger_strip (rhs0), file, mgr, count), fputs (" ", file),
	    pl (aiger_strip (rhs1), file, mgr, count), fputs (" ", file), pl (n->lhs, file, mgr, count), fputs ("\n", file);
	  aiger_sign (rhs0) ? fputs ("0", file) : fputs ("1", file);
	  aiger_sign (rhs1) ? fputs ("0", file) : fputs ("1", file);
	  fputs (" 1\n", file);
	}

      /* for those outputs having an inverted AIG node, insert an INV,
       * otherwise just a BUF
       */
      for (i = 0; i < mgr->num_outputs; i++)
	{

	  /* this case normally makes no sense, but you never know ... */
	  if (mgr->outputs[i].lit == aiger_false ||
	      mgr->outputs[i].lit == aiger_true)
	    {
	      fputs (".names ", file);
	      ((mgr->outputs[i].lit ==
		aiger_false) ? fputs ("c0 ", file) : fputs ("c1 ", file)),
		fputs (on (i, mgr, buffer), file), fputs ("\n", file), fputs ("1 1\n", file);
	      (mgr->outputs[i].lit == aiger_false) ? (require_const0 =
						      1) : (require_const1 =
							    1);
	    }
	  /* this should be the general case! */
	  else if (aiger_sign (mgr->outputs[i].lit))
	    {
	      fputs (".names ", file);
	      pl (aiger_strip (mgr->outputs[i].lit), file, mgr, count);
	      fputs (" ", file);
	      fputs (on (i, mgr, buffer), file);
	      fputs ("\n", file);
	      fputs ("0 1\n", file);
	    }
	  else
	    {
	      fputs (".names ",file), pl (aiger_strip (mgr->outputs[i].lit), file, mgr, count),
		fputs (" ",file), fputs (on (i, mgr, buffer), file), fputs ("\n", file), fputs ("1 1\n", file);
	    }
	}

      /* for those latches having an inverted AIG node as next-state-function, 
       * insert an INV. these latches were already saved in latch_helper!
       */
      for (i = 0; i < latch_helper_cnt; i++)
	{
	  unsigned l = latch_helper[i];
	  if (mgr->latches[l].next != aiger_false &&
	      mgr->latches[l].next != aiger_true)
	    {
	      assert (aiger_sign (mgr->latches[l].next));
	      fputs (".names ",file), pl (aiger_strip (mgr->latches[l].next), file, mgr, count),
		fputs (" n",file), pl (aiger_strip (mgr->latches[l].next), file, mgr, count),
		fputs ("\n",file), fputs ("0 1\n",file);
	    }
	}

      /* insert constants when necessary */
      if (require_const0)
	{
	  fputs (".names c0\n", file);
	}
      if (require_const1)
	{
	  fputs (".names c1\n", file), fputs ("1\n", file);
	}
      fputs (".end\n", file);

      /* close file */
//      if (dst)
//	fclose (file);
    }

  aiger_reset (mgr);

  free (latch_helper);

  return res;
}
