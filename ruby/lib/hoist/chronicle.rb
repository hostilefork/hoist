module Hoist

  @@chronicle_handler = lambda { |enable_where_constructed, enable_where_last_assigned, message, cp_output|
    puts message
    puts "    output from: " + cp_output.to_s
    puts "    controlled by: " + enable_where_constructed.to_s
    puts "    enabled at: " + enable_where_last_assigned.to_s
  }
  
  def chronicle(enabled, message, cp)
    enabled.hopefully_in_set([false, true], cp)
    
    if (enabled.value)
      @@chronicle_handler.call(enabled.where_constructed, enabled.where_last_assigned, message, cp)
    end
  end

  def set_chronicle_handler(new_handler)
    old_handler = @@chronicle_handler
    
    # is there any way to do basic checking, such as that this lambda takes the right # of params?
    @@chronicle_handler = new_handler
    
    return old_handler
  end
end
