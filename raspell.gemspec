require 'rubygems'
spec = Gem::Specification.new do |s|
  s.name = "raspell"
  s.version = "0.3"
  s.author = "Evan Weaver"
  s.email = "evan at cloudbur st"
  s.homepage = "http://rubyforge.org/projects/fauna"
  s.platform = Gem::Platform::RUBY
  s.summary = "an interface binding for ruby to aspell (aspell.net)"
  candidates = Dir.glob("{.,ext,lib,test,examples}/**/*")
  s.files = candidates.delete_if do |item|
                  item.include?("CVS") || item.include?("rdoc") || item.include?("svn")
                end
  s.require_path = "lib"
  s.autorequire = "raspell"
  s.extensions = ["ext/extconf.rb"]
  s.test_file = "test/simple_test.rb"
  s.has_rdoc = false
  s.extra_rdoc_files = ["README"]
end

if $0 == __FILE__
  Gem::manage_gems
  Gem::Builder.new(spec).build
end
