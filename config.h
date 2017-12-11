#ifndef __CONFIG_H_
#define __CONFIG_H_

/*
 * config.h - malloc lab configuration file
 *
 * Copyright (c) 2002, R. Bryant and D. O'Hallaron, All rights reserved.
 * May not be used, modified, or copied without permission.
 */

/*
 * This is the default path where the driver will look for the
 * default tracefiles. You can override it at runtime with the -t flag.
 */
#define TRACEDIR "./traces/"


#define MALLOC_MMAP_THRESHOLD 1024 * 1024

/*
 * This is the list of default tracefiles in TRACEDIR that the driver
 * will use for testing. Modify this if you want to add or delete
 * traces from the driver's test suite.
 */

#define DEFAULT_TRACEFILES  \
  "syn-array-short.rep",	\
  "syn-struct-short.rep",	\
  "syn-string-short.rep",	\
  "syn-mix-short.rep",	\
  "ngram-fox1.rep", \
  "syn-mix-realloc.rep",	\
  "bdd-aa4.rep", \
  "bdd-aa32.rep", \
  "bdd-ma4.rep", \
  "bdd-nq7.rep", \
  "cbit-abs.rep", \
  "cbit-parity.rep", \
  "cbit-satadd.rep", \
  "cbit-xyz.rep", \
  "ngram-gulliver1.rep", \
  "ngram-gulliver2.rep", \
  "ngram-moby1.rep", \
  "ngram-shake1.rep", \
  "syn-array.rep", \
  "syn-mix.rep", \
  "syn-string.rep", \
  "syn-struct.rep"

#define DEFAULT_GIANT_TRACEFILES \
  "syn-giantarray-short.rep", \
  "syn-giantmix-short.rep", \
  "syn-giantarray-med.rep", \
  "syn-giantarray.rep", \
  "syn-giantmix.rep"


#endif /* __CONFIG_H */
