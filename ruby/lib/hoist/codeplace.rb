module Hoist

  class Codeplace
    @@all_codeplaces = {}
    
    def initialize(filename, line, uuid)
      # use symbols instead of strings to save on memory
      @filename = filename.to_sym
      @line = line
      if uuid == nil
        @uuid = nil
        @permanent = false
      elsif uuid.kind_of? String
        # uuids are usually passed in as Base64 encoded strings
        @@all_codeplaces[uuid] = self
        @uuid = uuid
        @permanent = true
      else
        raise "Only Base64 string UUID supported at the moment by Codeplace"
      end
    end
    
    def filename
      return @filename.to_s
    end
    
    def line
      return @line
    end

    # this gets called a lot and is a performance bottleneck.
    # need a better and faster way
    # http://eigenclass.org/hiki/ruby+backtrace+data
    # alternately we can accept it as slow as it is temporary
    def self.make_here(depth)
      # http://grosser.it/2009/07/01/getting-the-caller-method-in-ruby/
      if /^(.+?):(\d+)(?::in `(.*)')?/ =~ caller(depth).first
        filename = Regexp.last_match[1]
        line = Regexp.last_match[2].to_i
        method = Regexp.last_match[3]
        return Codeplace.new(filename, line, nil)
      end
      
      raise "Codeplace 'here' could not parse file and line information from callstack"
    end
    
    def self.make_place(depth, uuid)
      # hash lookup is faster than walking/parsing the stack...
      cp = @@all_codeplaces[uuid]
      if cp != nil
        return cp
      end
      
      # http://grosser.it/2009/07/01/getting-the-caller-method-in-ruby/
      if /^(.+?):(\d+)(?::in `(.*)')?/ =~ caller(depth).first
        filename = Regexp.last_match[1]
        line = Regexp.last_match[2].to_i
        method = Regexp.last_match[3]
        return Codeplace.new(filename, line, uuid)
      end
      
      raise "Codeplace 'place' could not parse file and line information from callstack"
    end
    
    def uuid
      if @uuid == nil
        # we generate temporary uuids on demand, this way if it's never asked for
        # it doesn't cost us anything
        @uuid = UUIDTools::UUID.md5_create(UUIDTools::UUID_URL_NAMESPACE, filename + line.to_s)
      elsif @uuid.kind_of? String
        # we don't turn the string into a uuid value until it is needed
        # by convention we strip off the "==" at the end so we put it back here 
        @uuid = UUIDTools::UUID.parse_raw(Base64.decode64(@uuid + "=="));
      end
      return @uuid
    end
    
    def is_permanent
      return @permanent
    end
    
    def to_s
      return "File: '" + filename + "' - " + " Line # " + line.to_s
    end
  end
  
  def here
    return Codeplace.make_here(2)
  end

  def place(uuid)
    return Codeplace.make_place(2, uuid)
  end

end
