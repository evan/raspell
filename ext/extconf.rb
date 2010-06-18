require "mkmf"

dir_config("opt", "/opt/local/include",  "/opt/local/lib")
have_header("ruby.h")
have_header("aspell.h")
have_library("aspell")
create_makefile("raspell")
