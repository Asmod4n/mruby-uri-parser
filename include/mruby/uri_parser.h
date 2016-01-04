#ifndef MRUBY_URI_H
#define MRUBY_URI_H

#include <mruby.h>

#ifdef __cplusplus
extern "C" {
#endif

#define E_URI_ERROR (mrb_class_get_under(mrb, mrb_module_get(mrb, "URI"), "Error"))

#ifdef __cplusplus
}
#endif

#endif
