module URI
  class Parsed
    attr_reader :schema, :host, :port, :path, :query, :fragment, :userinfo

    HTTP = 'http'
    HTTPS = 'https'
    FTP = 'ftp'
    SSH = 'ssh'
    SFTP = 'sftp'

    def initialize(schema, host, port, path, query, fragment, userinfo, uri)
      @schema, @host, @port, @path, @query, @fragment, @userinfo, @uri = schema, host, port, path, query, fragment, userinfo, uri

      unless port
        case schema.downcase
        when HTTP
          @port = 80
        when HTTPS
          @port = 443
        when FTP
          @port = 23
        when SSH, SFTP
          @port = 22
        end
      end
    end

    def to_s
      @uri
    end
  end
end
