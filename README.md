# mruby-uri-parser

WHATWG-compliant URI parser for mruby, backed by [ada-url](https://github.com/ada-url/ada).

## Features

- WHATWG URL Standard compliant parsing
- Zero-copy component extraction via string views into the normalized href
- RFC 3986 percent-encoding and decoding
- Cross-platform (Linux, macOS, Windows)

## Examples

### URI Parsing

```ruby
uri = URI.parse("http://user:password@domain.tld:8080/path?query=string#fragment")

uri.scheme    # => "http"
uri.userinfo  # => "user:password"
uri.username  # => "user"
uri.password  # => "password"
uri.host      # => "domain.tld"
uri.hostname  # => "domain.tld"
uri.port      # => 8080
uri.path      # => "/path"
uri.query     # => "query=string"
uri.fragment  # => "fragment"
uri.origin    # => "http://domain.tld:8080"
uri.authority # => "domain.tld:8080"
uri.to_s      # => "http://user:password@domain.tld:8080/path?query=string#fragment" (original input)
uri.href      # => "http://user:password@domain.tld:8080/path?query=string#fragment" (WHATWG normalized)
```

Default ports are resolved via `getservbyname`:

```ruby
uri = URI.parse("https://heise.de")
uri.port      # => 443
uri.authority # => "heise.de"
```

IPv6 hosts are supported:

```ruby
uri = URI.parse("http://[::1]:8080/path")
uri.host      # => "[::1]"
uri.port      # => 8080
```

Predicate helpers:

```ruby
uri.has_credentials? # => true/false
uri.has_query?       # => true/false
uri.has_fragment?    # => true/false
```

Equality is based on the WHATWG normalized href:

```ruby
URI.parse("https://example.com") == URI.parse("https://example.com") # => true
```

### Error Handling

```ruby
URI.parse("not a url")  # raises URI::Malformed
URI.parse(123)          # raises TypeError
```

### URI Encoding / Decoding

Encodes all characters except RFC 3986 unreserved (`A-Z a-z 0-9 - . _ ~`):

```ruby
URI.encode("hello world/path?q=1") # => "hello%20world%2Fpath%3Fq%3D1"
URI.decode("hello%20world")        # => "hello world"
```

```ruby
URI.decode("%2G")  # raises URI::Malformed — invalid hex digit
URI.decode("%2")   # raises URI::Malformed — incomplete sequence
URI.decode(123)    # raises TypeError
URI.encode(123)    # raises TypeError
```

## Dependencies

- [ada-url](https://github.com/ada-url/ada) — built automatically from `deps/ada` via CMake
- [mruby-c-ext-helpers](https://github.com/Asmod4n/mruby-c-ext-helpers)

## License

Copyright 2016–2026 Hendrik Beskow

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this project except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.