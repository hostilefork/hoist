module Hoist

  @@hope_failed_handler = lambda { |message, cp|
    puts message
    puts "    output from: " + cp.to_s
  }
  
  def hopefully_not_reached(*args)
    if args.length == 1
      message = "Program Integrity Protection Triggered"
      cp = args[0]
    elsif args.length == 2
      message = args[0]
      cp = args[1]
    else
      raise "hopefully_not_reached requires either 1 or 2 parameters."
    end
    
    @@hope_failed_handler.call(message, cp)

    return false
  end

  def hopefully(condition, *args)
    if !(1..2).member?(args.length)
      raise "hopefully requires either 2 or 3 parameters."
      # how much checking to do here?  it might be worthwhile to ensure that
      # the parameters are a codeplace and/or string because asserts do not
      # necessarily fire that often...
    end
    
    if (condition) 
      return true
    end    
    
    return hopefully_not_reached(*args)
  end

  def set_hope_failed_handler(new_handler)
    old_handler = @@hope_failed_handler
    
    @@hope_failed_handler = new_handler
    
    return old_handler
  end
end