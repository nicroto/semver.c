//
// semver.c
//
// Copyright (c) 2015 Tomas Aparicio
// MIT licensed
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "semver.h"
#include "version.h"

#define NUMBERS "0123456789"
#define ALPHA   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define DELIMITER "."
#define SLICE_SIZE 50

char *
substr (const char *input, int offset, int len, char *dest) {
  int input_len = strlen(input);
  if (offset + len > input_len) return NULL;

  strncpy(dest, input + offset, len);
  return dest;
}

int
strcut (char *str, int begin, int len) {
  int l = strlen(str);

  if (len < 0) len = l - begin;
  if (begin + len > l) len = l - begin;
  memmove(str + begin, str + begin + len, l - len + 1);

  return len;
}

char *
semver_parse_slice (const char *str, char *buf, char sep) {
  int len = strlen(str);

  char * pr = strchr(buf, sep);
  if (pr != NULL) {
    int slice = pr - str + 1;
    char * tail = str + slice;

    char * data = malloc(sizeof(tail));
    strcpy(data, tail);

    int offset = strlen(buf) - strlen(tail) -  1;
    strcut(buf, offset, len);

    return data;
  }

  return NULL;
}

int
semver_parse (const char *str, semver_t *ver) {
  char * buf[strlen(str)];
  strcpy(buf, str);

  int valid = semver_is_valid(buf);
  if (!valid) return -1;

  ver->metadata = semver_parse_slice(str, buf, '+');
  ver->prerelease = semver_parse_slice(str, buf, '-');

  return semver_parse_version(buf, ver);
}

int
semver_parse_version (const char *str, semver_t *ver) {
  char major_s[SLICE_SIZE] = "0";
  char minor_s[SLICE_SIZE] = "0";
  char patch_s[SLICE_SIZE] = "0";

  char * slice = strtok((char*)str, DELIMITER);

  int count = 0;
  while (slice != NULL) {
    count++;

    size_t len = strlen(slice);
    if (len > SLICE_SIZE) return -1;

    switch (count) {
      case 1: strcpy(major_s, slice); break;
      case 2: strcpy(minor_s, slice); break;
      case 3: strcpy(patch_s, slice); break;
    }

    if (count == 3) break;
    slice = strtok(NULL, DELIMITER);
  }

  int major = semver_parse_int(major_s);
  if (major == -1) return major;
  ver->major = major;

  int minor = semver_parse_int(minor_s);
  if (minor == -1) return minor;
  ver->minor = minor;

  int patch = semver_parse_int(patch_s);
  if (patch == -1) return patch;
  ver->patch = patch;

  return 0;
}

int
semver_parse_prerelease (const char *str, semver_t *ver) {
  size_t len = strlen(str);
  if (len > SLICE_SIZE) return -1;

  char * buf[len];
  strcpy(buf, str);

  char * slice = strtok(buf, DELIMITER);

  int count = 0;
  int vcount = 0;

  while (slice != NULL) {
    count++;

    if (semver_is_alpha(slice)) {
      if (count > 1 && ver->stage != NULL) {
        char *buf = malloc( sizeof(ver->stage) + sizeof(slice) );
        strcpy(buf, ver->stage);
        strcat(str, slice);
        ver->stage = buf;

        slice = strtok(NULL, DELIMITER);
        continue;
      }

      char *buf = malloc(sizeof(slice));
      strcpy(buf, slice);
      ver->stage = buf;
      slice = strtok(NULL, DELIMITER);
      continue;
    }

    if (semver_is_number(slice)) {
      int num = semver_parse_int(slice);
      if (num == -1) return num;
      ver->pr_version[vcount] = num;
    }

    if (semver_is_alpha(slice)) {
      for (int i = 0; i < len; i++) {
        char v = slice[i];
        ver->pr_version[vcount] = (int)v;
      }
    }

    vcount++;
    slice = strtok(NULL, DELIMITER);
  }

  char *prerelease = malloc(sizeof(str));
  strcpy(prerelease, str);
  ver->prerelease = prerelease;

  ver->pr_version_count = vcount;

  return 0;
}

/**
 * Compare two semantic versions (x, y).
 *
 * Returns:
 * - `1` if x is higher than y
 * - `0` if x is equal to y
 * - `-1` if x is lower than y
 */

int
semver_compare (semver_t x, semver_t y) {
  int matches = semver_compare_version(x, y);
  if (matches) return matches;

  // To do: prerelease and meta comparisons

  return 0;
}

static int
compare_versions (int x, int y) {
  if (x != y) {
    if (x > y) {
      return 1;
    }
    return -1;
  }
  return 0;
}

int
semver_compare_version (semver_t x, semver_t y) {
  int match;

  match = compare_versions(x.major, y.major);
  if (match) return match;

  match = compare_versions(x.minor, y.minor);
  if (match) return match;

  match = compare_versions(x.patch, y.patch);
  if (match) return match;

  return 0;
}

void
semver_free (semver_t *x) {
  free(x->metadata);
  free(x->prerelease);
  free(x);
}

int
semver_gt (semver_t x, semver_t y) {
  int resolution = semver_compare(x, y);
  return resolution == 1 ? 1 : 0;
}

int
semver_lt (semver_t x, semver_t y) {
  int resolution = semver_compare(x, y);
  return resolution == -1 ? 1 : 0;
}

int
semver_eq (semver_t x, semver_t y) {
  int resolution = semver_compare(x, y);
  return resolution == 0 ? 1 : 0;
}

int
semver_ne (semver_t x, semver_t y) {
  int resolution = semver_compare(x, y);
  return resolution == -1 || resolution == 1 ? 1 : 0;
}

int
semver_gte (semver_t x, semver_t y) {
  int resolution = semver_compare(x, y);
  return resolution == 1 || resolution == 0 ? 1 : 0;
}

int
semver_lte (semver_t x, semver_t y) {
  int resolution = semver_compare(x, y);
  return resolution == -1 || resolution == 0 ? 1 : 0;
}

/**
 * Helpers
 */

int
semver_parse_int (const char *s) {
  int invalid = semver_valid_chars(s, NUMBERS);
  if (invalid) return -1;
  return strtol(s, NULL, 10);
}

int
semver_is_alpha (const char *s) {
  return semver_valid_chars(s, ALPHA) == 0 ? 1 : 0;
}

int
semver_is_number (const char *s) {
  return semver_valid_chars(s, NUMBERS) == 0 ? 1 : 0;
}

int
semver_is_valid (const char *s) {
  char tokens[] = NUMBERS ALPHA ".-+";
  return semver_valid_chars(s, tokens) == 0 ? 1 : 0;
}

int
semver_valid_chars (const char *s, const char *c) {
  size_t clen = strlen(c);
  size_t slen = strlen(s);

  for (int i = 0; i < slen; i++) {
    char v = s[i];
    int match = -1;

    for (int x = 0; x < clen; x++) {
      char y = c[x];
      if (y == v) {
        match = 0;
        break;
      }
    }

    if (match) return -1;
  }

  return 0;
}
