typedef struct {
  int a;
} almond_handover_struct_t;

almond_handover_struct_t f;

*((volatile (void )(almond_handover_struct_t *))0x100000)(&f);
