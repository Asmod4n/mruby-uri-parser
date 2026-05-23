#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <mruby.h>
#include <mruby/string.h>
#include <mruby/class.h>
#include <mruby/numeric.h>
#include <mruby/error.h>
#include <mruby/presym.h>
#include <mruby/variable.h>
#include <mruby/hash.h>
#include <mruby/branch_pred.h>
#include <mruby/num_helpers.hpp>
#include <ada.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

/* argv layout matches URI::Parsed#initialize:
   schema, host, port, path, query, fragment, userinfo, uri,
   username, password, origin, hostname, href               */
enum {
  IDX_SCHEMA    = 0,
  IDX_HOST      = 1,
  IDX_PORT      = 2,
  IDX_PATH      = 3,
  IDX_QUERY     = 4,
  IDX_FRAGMENT  = 5,
  IDX_USERINFO  = 6,
  IDX_URI       = 7,
  IDX_USERNAME  = 8,
  IDX_PASSWORD  = 9,
  IDX_ORIGIN    = 10,
  IDX_HOSTNAME  = 11,
  IDX_HREF          = 12,
  IDX_PORT_EXPLICIT = 13,
  IDX_MAX           = 14
};

static void
register_default_ports(mrb_state* mrb, struct RClass* uri_mod)
{
  static const struct { const char* scheme; int port; } entries[] = {
    { "http",   80   }, { "https",  443  },
    { "ws",     80   }, { "wss",    443  },
    { "ftp",    21   }, { "ftps",   990  },
    { "sftp",   22   }, { "ssh",    22   }, { "scp",  22  },
    { "smtp",   25   }, { "smtps",  465  },
    { "imap",   143  }, { "imaps",  993  },
    { "pop3",   110  }, { "pop3s",  995  },
    { "ldap",   389  }, { "ldaps",  636  },
    { "telnet", 23   }, { "tftp",   69   },
    { "gopher", 70   }, { "dict",   2628 },
    { "rtmp",   1935 }, { "rtsp",   554  },
    { "mqtt",   1883 },
  };

  size_t n = sizeof(entries) / sizeof(entries[0]);
  mrb_value h = mrb_hash_new_capa(mrb, (mrb_int)n);
  for (size_t i = 0; i < n; i++) {
    mrb_hash_set(mrb, h,
      mrb_str_new_cstr(mrb, entries[i].scheme),
      mrb_int_value(mrb, entries[i].port));
  }
  mrb_define_const_id(mrb, uri_mod, MRB_SYM(DEFAULT_PORTS), h);
}

/* ── URI.parse ──────────────────────────────────────────────────────────── */

static mrb_value
mrb_uri_parse(mrb_state *mrb, mrb_value klass)
{
  mrb_value uri_str;
  mrb_get_args(mrb, "S", &uri_str);
  char *str = RSTRING_PTR(uri_str);
  mrb_int len     = RSTRING_LEN(uri_str);

  auto result = ada::parse(std::string_view(str, (size_t)len));

  if (!result) {
    mrb_raise(mrb,
      mrb_class_get_under_id(mrb, mrb_class_ptr(klass), MRB_SYM(Malformed)),
      "Malformed URL");
  }

  auto &url = result.value();
  const ada::url_components &c = url.get_components();

  mrb_value argv[IDX_MAX];
  for (int i = 0; i < IDX_MAX; i++) argv[i] = mrb_nil_value();

  /* href — one allocation, all substrings reference into this */
  std::string_view href_sv = url.get_href();
  mrb_value href = mrb_str_new(mrb, href_sv.data(), (mrb_int)href_sv.size());
  argv[IDX_HREF] = href;

  /* uri — original input, no copy */
  argv[IDX_URI] = uri_str;

  /* schema — href[0 .. protocol_end) strip trailing ':' */
  {
    mrb_int end = (mrb_int)c.protocol_end;
    if (end > 0) end--;
    argv[IDX_SCHEMA] = mrb_str_substr(mrb, href, 0, end);
  }

  /* username — href[protocol_end+2 .. username_end) */
  {
    mrb_int start = (mrb_int)c.protocol_end + 2;
    mrb_int ulen  = (mrb_int)c.username_end - start;
    if (ulen > 0) argv[IDX_USERNAME] = mrb_str_substr(mrb, href, start, ulen);
  }

/* userinfo — href[protocol_end+2 .. host_start), host_start already points at '@' so no strip needed */
  {
    mrb_int start = (mrb_int)c.protocol_end + 2;
    mrb_int end   = (mrb_int)c.host_start; /* '@' is at host_start, not host_start-1 */
    mrb_int uilen = end - start;
    if (uilen > 0) argv[IDX_USERINFO] = mrb_str_substr(mrb, href, start, uilen);
  }

  /* password — href[username_end+1 .. host_start) */
  {
    mrb_int start = (mrb_int)c.username_end + 1;
    mrb_int end   = (mrb_int)c.host_start;
    mrb_int plen  = end - start;
    if (plen > 0) argv[IDX_PASSWORD] = mrb_str_substr(mrb, href, start, plen);
  }

/* host / hostname — href[host_start .. host_end), skip leading '@' if present */
  {
    mrb_int start = (mrb_int)c.host_start;
    if (start < (mrb_int)href_sv.size() && href_sv[start] == '@') start++;
    mrb_int hlen  = (mrb_int)c.host_end - start;
    mrb_value host = mrb_str_substr(mrb, href, start, hlen);
    argv[IDX_HOST]     = host;
    argv[IDX_HOSTNAME] = host;
  }
  /* port */
  {
    if (c.port != ada::url_components::omitted) {
      mrb_int start = (mrb_int)c.host_end + 1;
      mrb_int plen = (mrb_int)c.pathname_start - start;
      mrb_value port_str = mrb_str_substr(mrb, href, start, plen);
      argv[IDX_PORT] = mrb_str_to_integer(mrb, port_str, 10, FALSE);
      argv[IDX_PORT_EXPLICIT] = mrb_true_value();
    }
    else {
      argv[IDX_PORT_EXPLICIT] = mrb_false_value();
      if (mrb_string_p(argv[IDX_SCHEMA])) {
        mrb_value ports = mrb_const_get(mrb, klass, MRB_SYM(DEFAULT_PORTS));
        mrb_value port = mrb_hash_get(mrb, ports, argv[IDX_SCHEMA]);
        if (!mrb_nil_p(port)) argv[IDX_PORT] = port;
      }
    }
  }

  /* path — href[pathname_start .. search_start or hash_start or end) */
  {
    mrb_int start = (mrb_int)c.pathname_start;
    mrb_int end   = (mrb_int)href_sv.size();
    if (c.search_start != ada::url_components::omitted)
      end = (mrb_int)c.search_start;
    else if (c.hash_start != ada::url_components::omitted)
      end = (mrb_int)c.hash_start;
    argv[IDX_PATH] = mrb_str_substr(mrb, href, start, end - start);
  }

  /* query — href[search_start+1 .. hash_start or end) strip leading '?' */
  {
    if (c.search_start != ada::url_components::omitted) {
      mrb_int start = (mrb_int)c.search_start + 1;
      mrb_int end   = (mrb_int)href_sv.size();
      if (c.hash_start != ada::url_components::omitted)
        end = (mrb_int)c.hash_start;
      mrb_int qlen  = end - start;
      if (qlen > 0) argv[IDX_QUERY] = mrb_str_substr(mrb, href, start, qlen);
    }
  }

  /* fragment — href[hash_start+1 .. end) strip leading '#' */
  {
    if (c.hash_start != ada::url_components::omitted) {
      mrb_int start = (mrb_int)c.hash_start + 1;
      mrb_int flen  = (mrb_int)href_sv.size() - start;
      if (flen > 0) argv[IDX_FRAGMENT] = mrb_str_substr(mrb, href, start, flen);
    }
  }

/* origin — std::string by value, must bind to local before taking view */
  {
    std::string origin_str = url.get_origin();
    argv[IDX_ORIGIN] = mrb_str_new(mrb, origin_str.data(), (mrb_int)origin_str.size());
  }
  struct RClass *parsed_c = mrb_class_get_under_id(mrb, mrb_class_ptr(klass), MRB_SYM(Parsed));
  return mrb_obj_new(mrb, parsed_c, IDX_MAX, argv);
}

/* ── encode / decode ─────────────────────────────────────────────────────── */

static const unsigned char *
encode_table()
{
  static unsigned char t[256];
  static bool ready = false;
  if (unlikely(!ready)) {
    const char *u = "abcdefghijklmnopqrstuvwxyz"
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    "0123456789-._~";
    for (const char *p = u; *p; p++) t[(unsigned char)*p] = 1;
    ready = true;
  }
  return t;
}

static const char hex_chars[] = "0123456789ABCDEF";

static mrb_value
mrb_url_encode(mrb_state *mrb, mrb_value self)
{
  const char *url;
  mrb_int url_len;
  mrb_get_args(mrb, "s", &url, &url_len);

  if (likely(url_len < MRB_INT_MAX / 3)) {
    mrb_value url_encoded = mrb_str_new(mrb, NULL, url_len * 3);
    const unsigned char *tbl = encode_table();
    char *enc = RSTRING_PTR(url_encoded);

    for (mrb_int i = 0; i < url_len; i++) {
      unsigned char ch = (unsigned char)url[i];
      if (tbl[ch]) {
        *enc++ = (char)ch;
      } else {
        *enc++ = '%';
        *enc++ = hex_chars[ch >> 4];
        *enc++ = hex_chars[ch & 15];
      }
    }

    return mrb_str_resize(mrb, url_encoded, (mrb_int)(enc - RSTRING_PTR(url_encoded)));
  } else {
    mrb_raise(mrb, E_RANGE_ERROR, "string size too big");
    return mrb_nil_value();
  }
}

static const signed char decode_rfc3986[256] = {
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
  const char *encoded;
  mrb_int encoded_len;
  mrb_get_args(mrb, "s", &encoded, &encoded_len);

  struct RClass *malformed = mrb_class_get_under_id(mrb,
                               mrb_module_get_id(mrb, MRB_SYM(URI)),
                               MRB_SYM(Malformed));

  mrb_value decoded_str = mrb_str_new(mrb, NULL, encoded_len);
  char *decoded = RSTRING_PTR(decoded_str);

  for (mrb_int i = 0; i < encoded_len; i++) {
    unsigned char c = (unsigned char)encoded[i];
    if (c == '%') {
      if (unlikely(i + 2 >= encoded_len)) {
        mrb_raise(mrb, malformed, "Incomplete Percent Encoding");
      }
      signed char v1 = decode_rfc3986[(unsigned char)encoded[i + 1]];
      signed char v2 = decode_rfc3986[(unsigned char)encoded[i + 2]];
      if (unlikely(v1 == -1 || v2 == -1)) {
        mrb_raise(mrb, malformed, "Invalid Percent Encoding");
      }
      *decoded++ = (char)((v1 << 4) | v2);
      i += 2;
    } else {
      *decoded++ = (char)c;
    }
  }

  return mrb_str_resize(mrb, decoded_str, (mrb_int)(decoded - RSTRING_PTR(decoded_str)));
}

/* ── gem init ────────────────────────────────────────────────────────────── */

MRB_BEGIN_DECL

void
mrb_mruby_uri_parser_gem_init(mrb_state *mrb)
{

  struct RClass *uri_mod = mrb_define_module_id(mrb, MRB_SYM(URI));

  struct RClass *uri_error = mrb_define_class_under_id(mrb, uri_mod,
                               MRB_SYM(Error), mrb->eStandardError_class);
  mrb_define_class_under_id(mrb, uri_mod, MRB_SYM(Malformed),        uri_error);
  mrb_define_class_under_id(mrb, uri_mod, MRB_SYM(HostNotPresent),   uri_error);
  mrb_define_class_under_id(mrb, uri_mod, MRB_SYM(HostNotParseable), uri_error);
  mrb_define_class_under_id(mrb, uri_mod, MRB_SYM(ConnectMalformed), uri_error);
  mrb_define_class_under_id(mrb, uri_mod, MRB_SYM(PortTooLarge),     uri_error);

  mrb_define_module_function_id(mrb, uri_mod, MRB_SYM(parse),  mrb_uri_parse,  MRB_ARGS_REQ(1));
  mrb_define_module_function_id(mrb, uri_mod, MRB_SYM(encode), mrb_url_encode, MRB_ARGS_REQ(1));
  mrb_define_module_function_id(mrb, uri_mod, MRB_SYM(decode), mrb_url_decode, MRB_ARGS_REQ(1));
  register_default_ports(mrb, uri_mod);
}

void
mrb_mruby_uri_parser_gem_final(mrb_state *mrb)
{
  (void)mrb;
}

MRB_END_DECL
