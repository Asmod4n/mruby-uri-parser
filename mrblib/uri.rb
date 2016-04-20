module URI
  class Parsed
    attr_reader :scheme, :host, :port, :path, :query, :fragment, :userinfo

    def initialize(scheme, host, port, path, query, fragment, userinfo, uri)
      @scheme, @host, @port, @path, @query, @fragment, @userinfo, @uri = scheme, host, port, path, query, fragment, userinfo, uri

      unless port
        @port = URI.get_port(String(scheme).downcase)
      end
    end

    def to_s
      @uri
    end
  end
end
