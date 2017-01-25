module URI
  class Parsed
    attr_reader :schema, :host, :port, :path, :query, :fragment, :userinfo
    alias :scheme :schema

    def initialize(schema, host, port, path, query, fragment, userinfo, uri)
      @schema, @host, @port, @path, @query, @fragment, @userinfo, @uri = schema, host, port, path, query, fragment, userinfo, uri

      if port.nil? && !schema.nil?
        @port = URI.get_port(schema.downcase)
      end
    end

    def to_s
      @uri
    end
  end
end
