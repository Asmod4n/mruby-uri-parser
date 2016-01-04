assert("Uri#parse") do
  uri = URI.parse("http://user:password@domain.tld:8080/path?query=string#fragement")
  assert_equal('http', uri.schema)
  assert_equal('user:password', uri.userinfo)
  assert_equal('domain.tld', uri.host)
  assert_equal(8080, uri.port)
  assert_equal('/path', uri.path)
  assert_equal('query=string', uri.query)
  assert_equal('fragment', uri.fragment)
end
