#include "mruby/uri_parser.h"
#include "mrb_uri_parser.h"

#define MRB_URI_PARSED (mrb_class_get_under(mrb, mrb_module_get(mrb, "URI"), "Parsed"))

static mrb_value
mrb_http_parser_parse_url(mrb_state *mrb, mrb_value self)
{
  mrb_value uri_string;
  mrb_bool is_connect = FALSE;

  mrb_get_args(mrb, "S|b", &uri_string, &is_connect);

  struct http_parser_url parser;
  http_parser_url_init(&parser);
  enum http_parser_url_rcs rc = http_parser_parse_url(RSTRING_PTR(uri_string), RSTRING_LEN(uri_string), is_connect, &parser);
  switch (rc) {
    case URL_OKAY: {
      mrb_value argv[UF_MAX + 1] = {mrb_nil_value()};
      if (parser.field_set & (1 << UF_SCHEMA)) {
        argv[UF_SCHEMA] = mrb_str_substr(mrb, uri_string, parser.field_data[UF_SCHEMA].off, parser.field_data[UF_SCHEMA].len);
      }
      if (parser.field_set & (1 << UF_HOST)) {
        argv[UF_HOST] = mrb_str_substr(mrb, uri_string, parser.field_data[UF_HOST].off, parser.field_data[UF_HOST].len);
      }
      if (parser.field_set & (1 << UF_PORT)) {
        argv[UF_PORT] = mrb_fixnum_value(parser.port);
      } else {
        errno = 0;
        mrb_value schema = argv[UF_SCHEMA]
        if (mrb_test(schema)) {
          mrb_value test = mrb_funcall(mrb, schema, "downcase", 0);
          if (mrb_test(test)) {
            schema = test;
          }
        }
        const char *name = mrb_string_value_cstr(mrb, &schema);
        struct servent *answer = getservbyname(name, "tcp");
        if (answer != NULL) {
          argv[UF_PORT] = mrb_fixnum_value(ntohs(answer->s_port));
        }
        else if (errno) {
          mrb_sys_fail(mrb, "getservbyname");
        }
      }
      if (parser.field_set & (1 << UF_PATH)) {
        argv[UF_PATH] = mrb_str_substr(mrb, uri_string, parser.field_data[UF_PATH].off, parser.field_data[UF_PATH].len);
      }
      if (parser.field_set & (1 << UF_QUERY)) {
        argv[UF_QUERY] = mrb_str_substr(mrb, uri_string, parser.field_data[UF_QUERY].off, parser.field_data[UF_QUERY].len);
      }
      if (parser.field_set & (1 << UF_FRAGMENT)) {
        argv[UF_FRAGMENT] = mrb_str_substr(mrb, uri_string, parser.field_data[UF_FRAGMENT].off, parser.field_data[UF_FRAGMENT].len);
      }
      if (parser.field_set & (1 << UF_USERINFO)) {
        argv[UF_USERINFO] = mrb_str_substr(mrb, uri_string, parser.field_data[UF_USERINFO].off, parser.field_data[UF_USERINFO].len);
      }
      argv[UF_MAX] = uri_string;

      return mrb_obj_new(mrb, MRB_URI_PARSED, sizeof(argv) / sizeof(argv[0]), argv);
    }
      break;
    case MALFORMED_URL:
      mrb_raise(mrb, E_URI_MALFORMED, "Malformed URL");
      break;
    case HOST_NOT_PRESENT:
      mrb_raise(mrb, E_URI_HOST_NOT_PRESENT, "Host not present");
      break;
    case HOST_NOT_PARSEABLE:
      mrb_raise(mrb, E_URI_HOST_NOT_PARSEABLE, "Host not parseable");
      break;
    case CONNECT_MALFORMED:
      mrb_raise(mrb, E_URI_CONNECT_MALFORMED, "Connect malformed");
      break;
    case PORT_TOO_LARGE:
      mrb_raise(mrb, E_URI_PORT_TOO_LARGE, "Port too large");
      break;
  }
}

static mrb_value
mrb_uri_parser_get_port(mrb_state *mrb, mrb_value self)
{
  char *name, *proto = (char*)"tcp";

  mrb_get_args(mrb, "z|z", &name, &proto);

  errno = 0;
  struct servent *answer = getservbyname(name, proto);
  if (answer != NULL) {
    return mrb_fixnum_value(ntohs(answer->s_port));
  } else {
    if (errno == 0) {
      return mrb_nil_value();
    } else {
      mrb_sys_fail(mrb, "getservbyname");
    }
  }

  return self;
}

void
mrb_mruby_uri_parser_gem_init(mrb_state* mrb)
{
#ifdef _WIN32
  WSADATA wsaData;
  errno = 0;
  int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (err != 0) {
    mrb_sys_fail(mrb, "WSAStartup");
  }
#endif
  struct RClass *uri_mod, *uri_error_class;
  uri_mod = mrb_define_module(mrb, "URI");
  uri_error_class = mrb_define_class_under(mrb, uri_mod, "Error", E_RUNTIME_ERROR);
  mrb_define_class_under(mrb, uri_mod, "Malformed", uri_error_class);
  mrb_define_class_under(mrb, uri_mod, "HostNotPresent", uri_error_class);
  mrb_define_class_under(mrb, uri_mod, "HostNotParseable", uri_error_class);
  mrb_define_class_under(mrb, uri_mod, "ConnectMalformed", uri_error_class);
  mrb_define_class_under(mrb, uri_mod, "PortTooLarge", uri_error_class);
  mrb_define_module_function(mrb, uri_mod, "parse", mrb_http_parser_parse_url, MRB_ARGS_ARG(1, 1));
  mrb_define_module_function(mrb, uri_mod, "get_port", mrb_uri_parser_get_port, MRB_ARGS_ARG(1, 1));
}


void
mrb_mruby_uri_parser_gem_final(mrb_state* mrb)
{
#ifdef _WIN32
  WSACleanup();
#endif
}
