define make_sublib
$(srcdir)/$(name)/build/$(target): | $(srcdir)/$(name)/build/Makefile 
  $(MAKE) -C $(srcdir)/$(name)/build

$(srcdir)/$(name)/build/Makefile: $(srcdir)/$(name)/configure | $(srcdir)/$(name
)/build
  (cd $(srcdir)/$(name)/build; . ../$(srcdir)/$(name)/configure)

$(srcdir)/$(name)/build: | $(srcdir)/$(name)
  mkdir $$@

$(srcdir)/$(name)/configure: | $(srcdir)/$(name)/configure.ac
  (cd $(srcdir)/$(name); . ./autogen.sh)
endef
