module Hoist

  class Listed < Tracked
    
    # Listed::Manager
    class Manager
      def initialize()
        @list = []
      end
      
      # The C++ implementation returns a cloned list of Tracked
      # types.  The cloning is necessary for thread safety, while
      # the reinterpretation as a Tracked is needed because 
      # if the clones were of Listed type then they'd need to
      # be in yet another list (!)  To be forward-looking, that's
      # the interface we'll be offering here too.
      def list
        return @list.map { |listed| Tracked.new(listed.value, listed.where_constructed, listed.where_last_assigned) }
      end
      
      # no real way to make this privileged to Listed only ?
      def list_internal
        return @list
      end
    end
    
    def initialize(value, manager, cp)
      # Listed instances have the abilities of a tracked type...
      super(value, cp)
      
      # Append this pointer
      @manager = manager
      @manager.list_internal << self
    end
    
    def clone
      raise "Cannot clone a Listed object"
    end
    
    def hopefully_valid(cp)
      return hopefully(@manager != nil, "Already destroyed at " + where_last_assigned.to_s, cp)
    end
    
    # Ruby doesn't have destructors, so you have to
    # explicitly "unlist"
    def unlist(cp)
      if hopefully_valid(cp)
        idx = @manager.list_internal.index(self)
        hopefully(idx != nil, "Couldn't find listed instance in manager to unlist", cp)
        @manager.list_internal.delete_at(idx)
        
        # we indicate deletion by setting the manager to nil
        @manager = nil
        @last_assign_location = cp
      end
    end
  end

end
