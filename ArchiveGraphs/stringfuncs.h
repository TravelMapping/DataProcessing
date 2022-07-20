/*
  Originally from:

  UNIX Shell implementation

  Solution for Cow Shell Lab, Operating Systems
  Williams College Department of Computer Science

  string utility header file

  Copyright 2006, James D. Teresco

*/

struct stringlist_entry {
  char *val;
  struct stringlist_entry *next;
};

struct stringlist {
  int size;
  struct stringlist_entry *head;
};

extern char *trim_leading_trailing_spaces(char *str);
extern char *trimmed_strdup(char *str);
extern struct stringlist *new_stringlist();
extern void stringlist_append(struct stringlist *, char *);
extern char **stringlist_getarray(struct stringlist *);
extern void stringlist_free(struct stringlist *);
extern void double_up_single_quotes(char *);
