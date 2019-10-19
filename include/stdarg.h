typedef struct {
  unsigned int gp_offset;
  unsigned int fp_offset;
  void *overflow_arg_area;
  void *reg_save_area;
} va_list[1];

#define va_start(ap, _s) __builtin_va_start(ap, _s)
#define va_arg(ap, _type) __builtin_va_arg(ap)
#define va_end(ap) __builtin_va_end(ap)
