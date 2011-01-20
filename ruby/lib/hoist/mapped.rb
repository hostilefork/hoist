module Hoist

  class Mapped < Tracked
    class Manager
      def initialize
        @map = {}
      end
      
      def map_internal
        return @map
      end
      
      def map
        dup = {}
        @map.each do |key, mapped|
          dup[key] = Tracked.new(mapped.value, mapped.where_constructed, mapped.where_last_assigned)
        end
        return dup
      end
      
      def lookup_value(key, default)
        mapped = @map.fetch(key, nil)
        if mapped == nil
          return default
        else
          return mapped.value
        end
      end
      
      def lookup_hopefully(key, cp)
        return @map.fetch(key) { |el| hopefully_not_reached("Failed Mapped lookup", cp); return nil }
      end
    end
    
    def initialize(key, value, manager, cp)
      super(value, cp)
      @key = key
      
      @manager = manager
      hopefully(!@manager.map_internal.has_key?(key), "Inserting key that already exists for Mapped type", cp)
      @manager.map_internal[key] = self
    end
    
    def clone
      raise "Cannot clone a Mapped object"
    end  
    
    def key
      return @key
    end
    
    def hopefully_valid(cp)
      return hopefully(@manager != nil, "Already destroyed at " + where_last_assigned.to_s, cp)
    end
    
    def unmap(cp)
      if hopefully_valid(cp)
        if (!@manager.map_internal.has_key?(key))
          raise "Cannot find key for Mapped type in unmap"
        end
        hopefully(@manager.map_internal.delete(key) == self, "Value for Key in mapped type was not object being unmapped", cp)
        @manager = nil
      end
    end
  end

end
