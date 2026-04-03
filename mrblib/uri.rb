module URI
  class Parsed
    attr_reader :schema, :host, :port, :path, :query, :fragment, :userinfo,
                :username, :password, :origin, :hostname, :href

    alias :scheme   :schema

    def initialize(schema, host, port, path, query, fragment, userinfo, uri,
                   username = nil, password = nil, origin = nil, hostname = nil, href = nil,
                   port_explicit = false)
      @schema        = schema
      @host          = host
      @port          = port
      @port_explicit = port_explicit
      @path          = path
      @query         = query
      @fragment      = fragment
      @userinfo      = userinfo
      @uri           = uri
      @username      = username
      @password      = password
      @origin        = origin
      @hostname      = hostname
      @href          = href || uri
    end

    def to_s
      @uri
    end

    def inspect
      "#<URI::Parsed #{@uri}>"
    end

    def ==(other)
      return false unless other.is_a?(Parsed)
      @href == other.href
    end

    def has_credentials?
      !@userinfo.nil?
    end

    def has_query?
      !@query.nil?
    end

    def has_fragment?
      !@fragment.nil?
    end

    # Returns "host:port" or just "host" if port is nil
    def authority
      @port_explicit ? "#{@host}:#{@port}" : @host
    end
  end
end
