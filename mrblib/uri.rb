module URI
  class Parsed
    attr_reader :schema, :host, :port, :path, :query, :fragment, :userinfo
    alias :scheme :schema

    def initialize(schema, host, port, path, query, fragment, userinfo, uri)
      @schema, @host, @port, @path, @query, @fragment, @userinfo, @uri = schema, host, port, path, query, fragment, userinfo, uri

      unless port
        @port = URI.get_port(String(schema).downcase)
      end
    end

    def to_s
      @uri
    end
  end
end
