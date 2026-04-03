assert("URI::Parsed - full URL") do
  uri = URI.parse("http://user:password@domain.tld:8080/path?query=string#fragment")

  # scheme
  assert_equal('http', uri.schema)
  assert_equal('http', uri.scheme)

  # credentials
  assert_equal('user:password', uri.userinfo)
  assert_equal('user',          uri.username)
  assert_equal('password',      uri.password)
  assert_true(uri.has_credentials?)

  # host
  assert_equal('domain.tld', uri.host)
  assert_equal('domain.tld', uri.hostname)
  assert_equal('domain.tld:8080', uri.authority)

  # port — explicit
  assert_equal(8080, uri.port)

  # path
  assert_equal('/path', uri.path)

  # query
  assert_equal('query=string', uri.query)
  assert_true(uri.has_query?)

  # fragment
  assert_equal('fragment', uri.fragment)
  assert_true(uri.has_fragment?)

  # string representations
  assert_equal("http://user:password@domain.tld:8080/path?query=string#fragment", uri.to_s)
  assert_equal('http://user:password@domain.tld:8080/path?query=string#fragment', uri.href)
  assert_equal('http://domain.tld:8080', uri.origin)
end

assert("URI::Parsed - default port from getservbyname") do
  uri = URI.parse('https://heise.de')
  assert_equal('https',      uri.scheme)
  assert_equal('heise.de',   uri.host)
  assert_equal(443,          uri.port)
  assert_equal('heise.de',   uri.authority)
  assert_equal('https://heise.de', uri.to_s)
  assert_equal('https://heise.de/', uri.href)
  assert_equal('https://heise.de', uri.origin)

  assert_false(uri.has_credentials?)
  assert_false(uri.has_query?)
  assert_false(uri.has_fragment?)
  assert_nil(uri.username)
  assert_nil(uri.password)
  assert_nil(uri.userinfo)
  assert_nil(uri.query)
  assert_nil(uri.fragment)

  uri = URI.parse("ftp://ftp.example.com")
  assert_equal('ftp',             uri.scheme)
  assert_equal('ftp.example.com', uri.host)
  assert_equal(21,                uri.port)

  # unknown scheme — port is nil
  uri = URI.parse("myscheme://example.com/")
  assert_equal('myscheme', uri.scheme)
  assert_nil(uri.port)
end

assert("URI::Parsed - credentials variants") do
  # username only
  uri = URI.parse("https://user@example.com/")
  assert_equal('user', uri.username)
  assert_equal('user', uri.userinfo)
  assert_nil(uri.password)
  assert_true(uri.has_credentials?)

  # no credentials
  uri = URI.parse("https://example.com/")
  assert_false(uri.has_credentials?)
  assert_nil(uri.username)
  assert_nil(uri.password)
  assert_nil(uri.userinfo)
end

assert("URI::Parsed - query and fragment variants") do
  # query only
  uri = URI.parse("https://example.com/path?foo=bar")
  assert_equal('foo=bar', uri.query)
  assert_nil(uri.fragment)
  assert_true(uri.has_query?)
  assert_false(uri.has_fragment?)

  # fragment only
  uri = URI.parse("https://example.com/path#section")
  assert_nil(uri.query)
  assert_equal('section', uri.fragment)
  assert_false(uri.has_query?)
  assert_true(uri.has_fragment?)

  # empty query
  uri = URI.parse("https://example.com/path?")
  assert_nil(uri.query)

  # empty fragment
  uri = URI.parse("https://example.com/path#")
  assert_nil(uri.fragment)
end

assert("URI::Parsed - IPv6") do
  uri = URI.parse("http://[::1]:8080/path")
  assert_equal('http',  uri.scheme)
  assert_equal('[::1]', uri.host)
  assert_equal(8080,    uri.port)
  assert_false(uri.has_credentials?)
end

assert("URI::Parsed - equality") do
  uri = URI.parse('https://user@example.com')
  assert_equal(uri, URI.parse('https://user@example.com'))
  assert_false(uri == URI.parse('https://other@example.com'))
  assert_false(uri == "https://user@example.com")
  assert_false(uri == nil)
end

assert("URI::Parsed - errors") do
  assert_raise(URI::Malformed) { URI.parse("not a url") }
  assert_raise(URI::Malformed) { URI.parse("") }
  assert_raise(TypeError)      { URI.parse(123) }
end

assert("URI.encode") do
  # unreserved characters pass through unchanged
  unreserved = "abc123-._~"
  assert_equal(unreserved, URI.encode(unreserved))

  # reserved characters are percent-encoded
  assert_equal("%2F%3A%40%21%24%26%27%28%29%2A%2B%2C%3B%3D", URI.encode("/:@!$&'()*+,;="))

  # all special characters
  assert_equal("%3F%2F%3A%40%21%24%26%27%28%29%2A%2B%2C%3B%3D", URI.encode("?/:@!$&'()*+,;="))
  assert_equal("%20%21%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D", URI.encode(" !$&'()*+,/:;=?@[]"))

  # mixed — unreserved pass through, reserved encoded
  assert_equal("AbC123-._~%3A%2F%40%21%24%26%27%28%29%2A%2B%2C%3B%3D", URI.encode("AbC123-._~:/@!$&'()*+,;="))

  # '%' itself is encoded — no double-decode behaviour
  assert_equal("abc%2520def", URI.encode("abc%20def"))

  # '+' is reserved and gets encoded
  assert_equal("%2B", URI.encode("+"))

  # multi-byte UTF-8 — each byte encoded separately
  assert_equal("%C3%A9", URI.encode("\xC3\xA9"))

  # empty string
  assert_equal("", URI.encode(""))

  # non-string raises TypeError
  assert_raise(TypeError) { URI.encode(123) }
end

assert("URI.decode") do
  # unreserved characters pass through unchanged
  assert_equal("abc123-._~", URI.decode("abc123-._~"))

  # percent-encoded sequences are decoded
  assert_equal("/:@!$&'()*+,;=", URI.decode("%2F%3A%40%21%24%26%27%28%29%2A%2B%2C%3B%3D"))
  assert_equal("?/:@!$&'()*+,;=", URI.decode("%3F%2F%3A%40%21%24%26%27%28%29%2A%2B%2C%3B%3D"))
  assert_equal(" !$&'()*+,/:;=?@[]", URI.decode("%20%21%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D"))

  # case-insensitive hex digits
  assert_equal(" ", URI.decode("%20"))
  assert_equal("*", URI.decode("%2a"))
  assert_equal("*", URI.decode("%2A"))

  # '+' is NOT decoded to space (RFC 3986, not form decode)
  assert_equal("+", URI.decode("+"))

  # empty string
  assert_equal("", URI.decode(""))

  # roundtrip
  original = "hello world/path?query=value&other=123"
  assert_equal(original, URI.decode(URI.encode(original)))

  # invalid first hex digit
  assert_raise(URI::Malformed) { URI.decode("%G0") }

  # invalid second hex digit
  assert_raise(URI::Malformed) { URI.decode("%2G") }

  # incomplete sequence
  assert_raise(URI::Malformed) { URI.decode("%2") }

  # non-string raises TypeError
  assert_raise(TypeError) { URI.decode(123) }
end
