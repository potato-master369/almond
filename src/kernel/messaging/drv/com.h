#ifndef COM_H
#define COM_H
#include "kernel_types.h"
typedef struct {
  bool_t com1;
  bool_t com2;
} com_featureset_t;
void com_set_baud(void);
void com_send_char(char a);
#endif
