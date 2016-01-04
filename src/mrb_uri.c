#include "uri_parser.h"
#include "mruby/uri.h"
#include <mruby/string.h>

#define MRB_URI_PARSED (mrb_class_get_under(mrb, mrb_module_get(mrb, "URI"), "Parsed"))

static mrb_value
mrb_http_parser_parse_url(mrb_state *mrb, mrb_value self)
{
  mrb_value uri_string;
  mrb_bool is_connect = FALSE;

  mrb_get_args(mrb, "S|b", &uri_string, &is_connect);

  struct http_parser_url parser;
  http_parser_url_init(&parser);
  int rc = http_parser_parse_url(RSTRING_PTR(uri_string), RSTRING_LEN(uri_string), is_connect, &parser);
  if (rc == 0) {
    mrb_value argv[UF_MAX + 1];
    for (int curr_url_field = 0; curr_url_field < UF_MAX; curr_url_field++) {
      if (parser.field_set & (1 << curr_url_field)) {
        if (curr_url_field == UF_PORT) {
          argv[curr_url_field] = mrb_fixnum_value(parser.port);
        } else {
          argv[curr_url_field] = mrb_str_substr(mrb, uri_string, parser.field_data[curr_url_field].off, parser.field_data[curr_url_field].len);
        }
      } else {
        argv[curr_url_field] = mrb_nil_value();
      }
    }
    argv[UF_MAX] = uri_string;

    return mrb_obj_new(mrb, MRB_URI_PARSED, sizeof(argv) / sizeof(argv[0]), argv);
  } else {
    mrb_raise(mrb, E_URI_ERROR, "malformed URI");
  }
}

void
mrb_mruby_uri_gem_init(mrb_state* mrb)
{
  struct RClass *uri_mod = mrb_define_module(mrb, "URI");
  mrb_define_class_under(mrb, uri_mod, "Error", E_RUNTIME_ERROR);
  mrb_define_module_function(mrb, uri_mod, "parse", mrb_http_parser_parse_url, MRB_ARGS_ARG(1, 1));
}


void
mrb_mruby_uri_gem_final(mrb_state* mrb)
{
}
