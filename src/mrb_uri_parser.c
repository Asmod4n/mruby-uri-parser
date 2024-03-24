#include "mruby/uri_parser.h"
#include "mrb_uri_parser.h"

#if (__GNUC__ >= 3) || (__INTEL_COMPILER >= 800) || defined(__clang__)
# define likely(x) __builtin_expect(!!(x), 1)
# define unlikely(x) __builtin_expect(!!(x), 0)
#else
# define likely(x) (x)
# define unlikely(x) (x)
#endif

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
      if (likely(parser.field_set & (1 << UF_HOST))) {
        argv[UF_HOST] = mrb_str_substr(mrb, uri_string, parser.field_data[UF_HOST].off, parser.field_data[UF_HOST].len);
      }
      if (parser.field_set & (1 << UF_PORT)) {
        argv[UF_PORT] = mrb_int_value(mrb, parser.port);
      } else if (mrb_string_p(argv[UF_SCHEMA])) {
        errno = 0;
        struct servent *answer = getservbyname(mrb_string_value_cstr(mrb, &argv[UF_SCHEMA]), NULL);
        if (answer != NULL) {
          argv[UF_PORT] = mrb_int_value(mrb, ntohs(answer->s_port));
        } else if (errno) {
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
    } break;
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

  return mrb_undef_value(); // can't be reached.
}

static mrb_value
mrb_uri_parser_get_port(mrb_state *mrb, mrb_value self)
{
  char *name, *proto = (char*)"tcp";

  mrb_get_args(mrb, "z|z", &name, &proto);

  errno = 0;
  struct servent *answer = getservbyname(name, proto);
  if (answer != NULL) {
    return mrb_int_value(mrb, ntohs(answer->s_port));
  } else if (errno == 0) {
    return mrb_nil_value();
  } else {
    mrb_sys_fail(mrb, "getservbyname");
  }

  return self;
}

// Adopted from http://stackoverflow.com/a/21491633

static const unsigned char encode_rfc3986[256] = {
    ['a'] = 1, ['b'] = 1, ['c'] = 1, ['d'] = 1, ['e'] = 1, ['f'] = 1, ['g'] = 1, 
    ['h'] = 1, ['i'] = 1, ['j'] = 1, ['k'] = 1, ['l'] = 1, ['m'] = 1, ['n'] = 1, 
    ['o'] = 1, ['p'] = 1, ['q'] = 1, ['r'] = 1, ['s'] = 1, ['t'] = 1, ['u'] = 1, 
    ['v'] = 1, ['w'] = 1, ['x'] = 1, ['y'] = 1, ['z'] = 1, ['A'] = 1, ['B'] = 1, 
    ['C'] = 1, ['D'] = 1, ['E'] = 1, ['F'] = 1, ['G'] = 1, ['H'] = 1, ['I'] = 1, 
    ['J'] = 1, ['K'] = 1, ['L'] = 1, ['M'] = 1, ['N'] = 1, ['O'] = 1, ['P'] = 1, 
    ['Q'] = 1, ['R'] = 1, ['S'] = 1, ['T'] = 1, ['U'] = 1, ['V'] = 1, ['W'] = 1, 
    ['X'] = 1, ['Y'] = 1, ['Z'] = 1, ['0'] = 1, ['1'] = 1, ['2'] = 1, ['3'] = 1, 
    ['4'] = 1, ['5'] = 1, ['6'] = 1, ['7'] = 1, ['8'] = 1, ['9'] = 1, ['-'] = 1, 
    ['_'] = 1, ['.'] = 1, ['~'] = 1
};

static const char hex_chars[] = "0123456789ABCDEF";

static mrb_value
mrb_url_encode(mrb_state *mrb, mrb_value self)
{
  char *url;
  mrb_int url_len;
  mrb_get_args(mrb, "s", &url, &url_len);

  if (likely(url_len < MRB_INT_MAX / 3)) {
    mrb_value url_encoded = mrb_str_new(mrb, NULL, url_len * 3);
    char *enc = RSTRING_PTR(url_encoded);

    for (mrb_int i = 0; i < url_len; i++)
    {
      if (encode_rfc3986[(unsigned char)url[i]]) {
        *enc++ = url[i];
      } else {
        *enc++ = '%';
        *enc++ = hex_chars[(unsigned char) url[i] >> 4];
        *enc++ = hex_chars[(unsigned char) url[i] & 15];
      }
    }

    return mrb_str_resize(mrb, url_encoded, enc - RSTRING_PTR(url_encoded));
  } else {
    mrb_raise(mrb, E_RANGE_ERROR, "string size too big");
  }
}

// Adopted from http://stackoverflow.com/a/30895866

static const char decode_rfc3986[] = {
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
     0, 1, 2, 3, 4, 5, 6, 7,  8, 9,-1,-1,-1,-1,-1,-1,
    -1,10,11,12,13,14,15,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,10,11,12,13,14,15,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1
};

static mrb_value
mrb_url_decode(mrb_state *mrb, mrb_value self)
{
  char *encoded;
  mrb_int encoded_len;
  mrb_get_args(mrb, "s", &encoded, &encoded_len);

  mrb_value decoded_str = mrb_str_new(mrb, NULL, encoded_len);
  char *decoded = RSTRING_PTR(decoded_str);

  char c, v1, v2;

  for(mrb_int i = 0; i < encoded_len; i++) {
    c = encoded[i];
    if(c == '%') {
      if(unlikely(i + 2 >= encoded_len)) { // Check if there are two more characters after '%'
        mrb_raise(mrb, E_URI_MALFORMED, "Incomplete Percent Encoding");
      }
      i++;
      v1 = decode_rfc3986[(unsigned char)encoded[i]];
      i++;
      v2 = decode_rfc3986[(unsigned char)encoded[i]];
      if(unlikely(v1 == -1 || v2 == -1)) {
        mrb_raise(mrb, E_URI_MALFORMED, "Invalid Percent Encoding");
      }
      c = (v1 << 4) | v2;
    }
    *decoded++ = c;
  }

  return mrb_str_resize(mrb, decoded_str, decoded - RSTRING_PTR(decoded_str));
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
  mrb_define_module_function(mrb, uri_mod, "encode", mrb_url_encode, MRB_ARGS_REQ(1));
  mrb_define_module_function(mrb, uri_mod, "decode", mrb_url_decode, MRB_ARGS_REQ(1));
}


void
mrb_mruby_uri_parser_gem_final(mrb_state* mrb)
{
#ifdef _WIN32
  WSACleanup();
#endif
}
