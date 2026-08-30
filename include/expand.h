#ifndef EXPAND_H
#define EXPAND_H

#include "parser.h"

// String-based expander
char *expand_word(const char *word);

#ifdef HISTORY_H
  // In main.c and history.c, expand_variables takes a Pipeline pointer
  void expand_variables(Pipeline *pipeline);
#else
  // Map expand_variables to expand_word for parser.c and expand.c
  #define expand_variables expand_word
#endif

#endif // EXPAND_H
