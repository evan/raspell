$:.unshift(File.dirname(__FILE__) + "/../lib/")
begin
  require 'rubygems'
  require_gem 'raspell'
rescue LoadError
  require 'raspell'
end

# Show all installed dictionaries.
# Select one to use.
puts "Choose Dictionary:\n"
dicts = Aspell.list_dicts
raise "\nNo dictionary installed!\nYou have to install some dictionaries (see: www.aspell.net)" if dicts.empty?
dicts.each_with_index { |dict, num|
  puts " [#{num+1}] #{dict.name}    (code:#{dict.code}, jargon:#{dict.jargon}, size:#{dict.size})\n"
}
puts "enter number of dictionary:"
dictionary = dicts[gets.to_i-1]


# Create a new aspell instance
# Aspell.new(language, jargon, size, encoding) 
# It is possible to omit all parameters (just forget or nil as placeholder).
aspell = Aspell.new(dictionary.code, dictionary.jargon, dictionary.size.to_s)
# Set suggestion mode: one of [ULTRA|FAST|NORMAL|BADSPELLERS]
aspell.suggestion_mode = Aspell::ULTRA

puts "\n\nCurrent Dictionary config=====================\n"
Aspell::DictionaryOptions.each { |option|
  begin
    option_details = aspell.get_option(option)
  rescue 
    option_details = "unknown"
  end
  puts "  -#{option}: #{option_details}\n"
}
puts "==============================================\n"

