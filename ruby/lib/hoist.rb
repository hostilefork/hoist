dir = File.expand_path(File.dirname(__FILE__))
$: << dir unless $:.include?(dir) # should we really have to do this? It's necessary to run examples.rb

# http://ruby-doc.org/stdlib-1.8.7/libdoc/base64/rdoc/classes/Base64.html
require "base64"

require "rubygems"
# http://uuidtools.rubyforge.org/
require 'uuidtools'

require "hoist/codeplace"
require "hoist/hopefully"
require "hoist/tracked"
require "hoist/listed"
require "hoist/stacked"
require "hoist/mapped"
require "hoist/chronicle"
require "hoist/version"
