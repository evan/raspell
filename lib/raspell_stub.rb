
# Stub for documentation purposes
raise "This file is only for documentation purposes. Require #{File.dirname(__FILE__)}/../ext/raspell} instead."

# The functionality of Aspell is accessible via this class.
class ::Aspell

  # Creates an Aspell instance. All parameters are optional and have default values. In most situations it is enough to create an Aspell-instance with only a language.
  # * <tt>language</tt> -  ISO639 language code plus optional ISO 3166 counry code as string (eg: "de" or "us_US").
  # * <tt>jargon</tt> - A special jargon of the selected language.
  # * <tt>size</tt> - The size of the dictionary to chose (if there are options).
  # * <tt>encoding</tt> - The encoding to use.
  def initialize(language, jargon, size, encoding); end

  # Returns a list of AspellDictInfo-objects describing each dictionary.
  def self.list_dicts; end
  
  # Returns a list of checker options.
  def self.CheckerOptions; end
  
  # Returns a list of dictionary options.
  def self.DictionaryOptions; end 
  
  # Returns a list of filter options.
  def self.FilterOptions; end
  
  # Returns a list of misc options.
  def self.MiscOptions; end 
  
  # Returns a list of run-together options.
  def self.RunTogetherOptions; end 
  
  # Returns a list of utility options.
  def self.UtilityOptions; end 
  
  

  # Add a word to the private dictionary.
  def add_to_personal(word); end

  # Add a word to the session ignore list. The session persists for the lifetime of <tt>self</tt>.
  def add_to_session(word); end

  # Check given word for correctness. Returns <tt>true</tt> or <tt>false</tt>.
  def check(word); end

  # Check an entire file for correctness. This method requires a block, which will yield each misspelled word.
  def correct_file(filename); end

  # Check an array of strings for correctness. This method requires a block, which will yield each misspelled word.
  def correct_lines(array_of_strings)
    yield
  end

  # Clear the the session wordlist.
  def clear_session(); end

  # Value of an option in config. Returns a string.
  def get_option(option); end

  # Value of option in config. Returns a list of strings.
  def get_option_as_list(option); end

  # Returns a list of all words in the passed array that are misspelled.
  def list_misspelled(array_of_strings); end

  # Return a list of words inside the private dictionary.
  def personal_wordlist(); end

  # Sync changed dictionaries to disk.
  def save_all_word_lists(); end

  # Return the session wordlist.
  def session_wordlist(); end

  # Set a passthrough option to a value. Use the strings "true" and "false" for <tt>true</tt> and <tt>false</tt>.
  def set_option(option, value); end

  # Returns an array of suggestions for the given word. Note that <tt>suggest</tt> returns multiple corrections even for words that are not wrong.
  def suggest(word); end

  # Sets the suggestion mode. Use Aspell::ULTRA, Aspell::FAST, Aspell::NORMAL, or Aspell::BADSPELLERS as the parameter.
  def suggestion_mode=(mode); end

  ULTRA = Fastest mode. Look only for sound-a-likes within one edit distance. No typo analysis.;
  
  NORMAL = Normal speed mode. Look for sound-a-likes within two edit distances, with typo analysis.;
  
  FAST = Fast mode. Look for sound-a-likes within one edit distance, with typo analysis.;
  
  BADSPELLERS = Slowest mode. Tailored for bad spellers.;
  
end

=begin rdoc
Represents an Aspell dictionary. Returned by <tt>Aspell. list_dicts</tt>.     
=end

class ::AspellDictInfo

  # Returns the code of the dictionary.
  def code; end

  # Returns the jargon of the dictionary.
  def jargon; end

  # Returns the name of the dictionary.
  def name; end

  # Returns the size of the dictionary.
  def size; end
    
end
