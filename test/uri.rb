assert("URI") do
  uri = URI.parse("http://user:password@domain.tld:8080/path?query=string#fragment")
  assert_equal('http', uri.schema)
  assert_equal('http', uri.scheme)
  assert_equal('user:password', uri.userinfo)
  assert_equal('domain.tld', uri.host)
  assert_equal(8080, uri.port)
  assert_equal('/path', uri.path)
  assert_equal('query=string', uri.query)
  assert_equal('fragment', uri.fragment)
  assert_equal("http://user:password@domain.tld:8080/path?query=string#fragment", uri.to_s)

  uri = URI.parse('https://heise.de')
  assert_equal('https', uri.scheme)
  assert_equal('heise.de', uri.host)
  assert_equal(443, uri.port)
  assert_equal('https://heise.de', uri.to_s)

  #HTTP CONNECT parsing
  uri = URI.parse("heise.de:443", true)
  assert_equal('heise.de', uri.host)
  assert_equal(443, uri.port)
end

assert("URI encoding/decoding") do
# Test 1: Unreserved characters
unreserved = "abc123-._~"
assert_equal(unreserved, URI.encode(unreserved))
assert_equal(unreserved, URI.decode(unreserved))

# Test 2: Reserved characters
reserved = "/:@!$&'()*+,;="
encoded_reserved = "%2F%3A%40%21%24%26%27%28%29%2A%2B%2C%3B%3D"
assert_equal(encoded_reserved, URI.encode(reserved))
assert_equal(reserved, URI.decode(encoded_reserved))

# Test 3: Reserved characters in query
query = "?/:@!$&'()*+,;="
encoded_query = "%3F%2F%3A%40%21%24%26%27%28%29%2A%2B%2C%3B%3D"
assert_equal(encoded_query, URI.encode(query))
assert_equal(query, URI.decode(encoded_query))

# Test 4: Percent-encoding
decoded = " !$&'()*+,/:;=?@[]"
encoded = "%20%21%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D"
assert_equal(encoded, URI.encode(decoded))
assert_equal(decoded, URI.decode(encoded))

# Test 5: Mixed case
mixed_case = "AbC123-._~:/@!$&'()*+,;="
encoded_mixed_case = "AbC123-._~%3A%2F%40%21%24%26%27%28%29%2A%2B%2C%3B%3D"
assert_equal(encoded_mixed_case, URI.encode(mixed_case))
assert_equal(mixed_case, URI.decode(encoded_mixed_case))

# Test 6: Edge case - empty string
empty = ""
assert_equal(empty, URI.encode(empty))
assert_equal(empty, URI.decode(empty))

# Test 7: Invalid percent-encoding
invalid_percent_encoding = "abc%2G"
assert_raise(URI::Malformed) { URI.decode(invalid_percent_encoding) }

# Test 8: Incomplete percent-encoding
incomplete_percent_encoding = "abc%2"
assert_raise(URI::Malformed) { URI.decode(incomplete_percent_encoding) }

# Test 9: Non-string input for encoding
non_string_input = 123
assert_raise(TypeError) { URI.encode(non_string_input) }

# Test 10: Non-string input for decoding
non_string_input = 123
assert_raise(TypeError) { URI.decode(non_string_input) }
end
