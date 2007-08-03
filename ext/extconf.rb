require "mkmf"

have_header("ruby.h")
have_header("aspell.h")
have_library("aspell")
create_makefile("raspell")
