# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "raspell"
  s.version = "1.3"

  s.required_rubygems_version = Gem::Requirement.new(">= 1.2") if s.respond_to? :required_rubygems_version=
  s.authors = [""]
  s.date = "2012-09-03"
  s.description = "An interface binding for the Aspell spelling checker."
  s.email = ""
  s.extensions = ["ext/extconf.rb"]
  s.extra_rdoc_files = ["CHANGELOG", "LICENSE", "README.rdoc", "lib/raspell_stub.rb"]
  s.files = ["CHANGELOG", "LICENSE", "Manifest", "README.rdoc", "Rakefile", "ext/extconf.rb", "ext/raspell.c", "ext/raspell.h", "lib/raspell_stub.rb", "test/simple_test.rb", "raspell.gemspec"]
  s.homepage = "http://evan.github.com/evan/raspell/"
  s.require_paths = ["lib", "ext"]
  s.rubyforge_project = "evan"
  s.rubygems_version = "1.8.23"
  s.summary = "An interface binding for the Aspell spelling checker."
  s.test_files = ["test/simple_test.rb"]

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
    else
    end
  else
  end
end
