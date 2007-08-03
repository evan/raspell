$:.unshift(File.dirname(__FILE__) + "/../lib/")
begin
  require 'rubygems'
  require_gem 'raspell'
rescue LoadError
  require 'raspell'
end

aspell = Aspell.new
# Set suggestion mode: one of [ULTRA|FAST|NORMAL|BADSPELLERS]
aspell.suggestion_mode = Aspell::ULTRA

  
# some content to check - with many errors
content = ["This is a simpel sample texxt, with manny erors to find.",
           "To check this, we need an englishh diktionary!"]
puts content.join("\n")+ "\n\nChecking..."

# get a list of all misspelled words
misspelled = aspell.list_misspelled(content).sort
puts "\n#{misspelled.length} misspelled words:\n  -"+misspelled.join("\n  -")

# correct content
correctreadme = aspell.correct_lines(content) { |badword|
  puts "\nMisspelled: #{badword}\n"
  suggestions = aspell.suggest(badword)
  suggestions.each_with_index { |word, num|
    puts "  [#{num+1}] #{word}\n"
  }
  puts "Enter number or correct word: "
  input = gets.chomp
  if (input.to_i != 0)
    input = suggestions[input.to_i-1] 
  else
    if (!aspell.check(input))
      puts "\nthe word #{input} is not known inside dictionary." 
      #possible to add the word into private dictionary or into session
      #via aspell.add_to_personal or aspell.add_to_session
    end
  end
  input #return input
}

puts "\n\nThe correct text is:\n"+correctreadme.join("\n")

# It is possible to correct files directly via aspell.correct_file(filename)
