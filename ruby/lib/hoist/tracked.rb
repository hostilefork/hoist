module Hoist

  class Tracked
    
    def initialize(value, cp, *args)
      @value = value
      @construct_location = cp
      if args.length == 0
        @last_assign_location = cp
      elsif args.length == 1
        @last_assign_location = args[0]
      else
        raise "More than 3 arguments passed to Tracked.initialize()"
      end
    end
    
    # This may be too conservative...
    # ...but Listed/Stacked/Mapped should not be cloned
    def clone
      raise "Cannot clone a Tracked object"
    end
    
    # Derived classes may have validity rules that override this
    def hopefully_valid(cp)
      return true
    end
    
    # Currently by using the method_missing dispatch, you should be able
    # to pass a tracked value through to anything that expected just the
    # value.  But if you want to extract the value itself, you use this
    # method...
    def value
      # since we have the stack in Ruby, should there be a "caller" codeplace?
      if !hopefully_valid(here)
        return nil
      end
      return @value
    end
    
    def where_constructed
      return @construct_location
    end
    
    def where_last_assigned
      return @last_assign_location
    end
    
    def assign(value, cp)
      if hopefully_valid(cp)
        @last_assign_location = cp
        @value = value
      end
    end
    
    # Ruby defines ensure as a keyword for exception processing
    # It seems to not mind if there's a method called ensure, but better
    # to avoid the conflict...
    def guarantee(value, cp)
      if hopefully_valid(cp)
        if @value != value
          assign(value, cp)
        end
      end
    end
    
    def hopefully_equal_to(value, cp)
      if !hopefully_valid(cp)
        return false
      end
      
      if (value == @value)
        return true
      else
        message = "Expected value to be " + value.to_s + " and it was " + @value.to_s + "\n"
        message = message + "Last assignment was at " + @last_assign_location.to_s
        return hopefully_not_reached(message, cp)
      end
    end
    
    def hopefully_in_set(set, cp)
      # implement this
    end
    
    # by default we pass any failed method calls on to the value
    def method_missing(id, *args)
      if hopefully_valid(here)
        value.method(id).call(*args)
      end
    end
  end

end
