module Hoist

  class Stacked < Tracked
    
    # Stacked::Manager
    class Manager
      def initialize
        @stack = []
      end
      
      def stack
        return @stack.map { |stacked| Tracked.new(stacked.value, stacked.where_constructed, stacked.where_last_assigned) }
      end
      
      def stack_internal
        return @stack
      end
    end
    
    def initialize(value, manager, cp)
      # Stacked instances have the abilities of a tracked type...
      super(value, cp)
        
      # Append this pointer
      @manager = manager
      @manager.stack_internal.push(self);
    end

    def clone
      raise "Cannot clone a Stacked object"
    end

    def hopefully_valid(cp)
      return hopefully(@manager != nil, "Already destroyed at " + where_last_assigned.to_s, cp)
    end
    
    # Ruby doesn't have destructors, so you have to
    # explicitly "unstack"; we enforce that the unstack
    # is in the reverse order of the stack
    def unstack(cp)
      if hopefully_valid(cp)
        if @manager.stack_internal.empty?
          raise "Trying to unstack an empty stacked type."
        end
        hopefully(@manager.stack_internal.pop == self, cp)
        @manager = nil
      end
    end
  end

end
