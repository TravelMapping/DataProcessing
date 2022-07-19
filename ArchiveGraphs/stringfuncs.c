/*

  Originally from:

  UNIX Shell implementation

  Solution for Cow Shell Lab, Operating Systems
  Williams College Department of Computer Science

  string utility functions

  Copyright 2006, James D. Teresco

*/

#include <string.h>
#include <stdlib.h>
#include "stringfuncs.h"

/* 
   Function to remove leading and trailing spaces from a C string.

   This function is destructive in that the null terminator will be
   moved forward through any white space.  The return value is the
   first non-whitespace character at the start of the string
*/
char *trim_leading_trailing_spaces(char *str) {
  char *ptr;

  /* start with trailing spaces */
  ptr = str;
  while (*ptr) ptr++;
  /* we are pointing at the null terminator */
  if (ptr != str) {
    ptr--;
    while ((ptr >= str) && 
	   ((*ptr == ' ') || (*ptr == '\t'))) {
      *ptr = '\0';
      ptr--;
    }
  }

  /* we have eliminated trailing whitespace, now eliminate at the start */
  ptr = str;
  while (*ptr && ((*ptr == ' ') || (*ptr == '\t')))
    ptr++;

  return ptr;
}

/* 
   Function to allocate a copy of a C string, but with leading and
   trailing spaces removed.  The original string is unmodified.  The
   return value should be deallocated using free();
*/
char *trimmed_strdup(char *str) {
  char *ptr, *retval;

  /* skip over white space at the start */
  ptr = str;
  while (*ptr && ((*ptr == ' ') || (*ptr == '\t')))
    ptr++;

  /* now duplicate the string (so the duplicate will have space for
     trailing whitespace */
  retval = strdup(ptr);

  /* eliminate trailing whitespace */
  ptr = retval;
  while (*ptr) ptr++;
  /* we are pointing at the null terminator */
  if (ptr != retval) {
    ptr--;
    while ((ptr >= retval) && 
	   ((*ptr == ' ') || (*ptr == '\t'))) {
      *ptr = '\0';
      ptr--;
    }
  }

  return retval;
}

/* string list utility functions */

/* construct a new (empty) stringlist */
struct stringlist *new_stringlist() {
  struct stringlist *retval;

  retval = (struct stringlist *)malloc(sizeof(struct stringlist));
  retval->size = 0;
  retval->head = NULL;

  return retval;
}

/* construct a new (empty) stringlist */
static struct stringlist_entry *new_stringlist_entry(char *val) {
  struct stringlist_entry *retval;

  retval = (struct stringlist_entry *)malloc(sizeof(struct stringlist_entry));
  retval->val = val;
  retval->next = NULL;

  return retval;
}

/* append a new string to the end of an existing stringlist (which cannot
   be empty) */
void stringlist_append(struct stringlist *sl, char *str) {
  struct stringlist_entry *finger;

  if (sl->head) {
    finger = sl->head;
    /* find the end */
    while (finger->next) {
      finger = finger->next;
    }
    /* create a new entry at the end */
    finger->next = new_stringlist_entry(str);
  }
  else {
    sl->head = new_stringlist_entry(str);
  }
}

/* convert the stringlist into an array of strings suitable for
   passing as an argument list.  The returned pointer should be
   returned to the system with free() */
char **stringlist_getarray(struct stringlist *sl) {
  int len, i;
  struct stringlist_entry *finger;
  char **retval;

  len = 0;
  finger = sl->head;
  while (finger) {
    len++;
    finger = finger->next;
  }

  retval = (char **)malloc((len+1)*sizeof(char *));

  finger = sl->head;
  for (i=0; i<len; i++) {
    retval[i] = finger->val;
    finger = finger->next;
  }
  retval[len] = NULL;

  return retval;
}

/* clean up a stringlist */
void stringlist_free(struct stringlist *sl) {
  struct stringlist_entry *finger, *prev;

  finger = sl->head;

  while (finger) {
    prev = finger;
    finger = finger->next;
    free(prev);
  }

  free(sl);
}
