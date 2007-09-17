
require 'rubygems'
gem 'echoe', '>=2.3'
require 'echoe'

Echoe.new("raspell") do |p|  
  p.rubyforge_name = "fauna"
  p.summary = "An interface binding for the Aspell spelling checker."
  p.url = "http://blog.evanweaver.com/files/doc/fauna/raspell/"  
  p.docs_host = "blog.evanweaver.com:~/www/bax/public/files/doc/"
  p.has_rdoc = false # avoids warnings on gem install
  p.rdoc_pattern = /CHANGELOG|EXAMPLE|LICENSE|README|lib/
  p.rdoc_options = [] # don't want to include the stubbed out source
  p.require_signed = true
end
