#ifndef __WIGLE_H__
#define __WIGLE_H__

#include <stdint.h>
#include <stddef.h>

typedef struct {
  char line[256];
} wigle_row_t;

int wigle_get_header(char *buf, size_t buf_len);
int wigle_process(void);
int wigle_get_total_unique(void);

#endif // __WIGLE_H__
