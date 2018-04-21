# because autotools devs are retarded nazis
# we're not allowed to redefine named variables
# so only meaningless $(1) $(2) etc are allowed
# this is because they know what's best for us
# and what we were doing isn't the right way to do it anyway
# not that they know what the right way is
# but it's surely not what they don't allow

define make_sublib_fn
$(srcdir)/$(1)/build/$(2): | $(srcdir)/$(1)/build/Makefile 
  $(MAKE) -C $(srcdir)/$(1)/build

$(srcdir)/$(1)/build/Makefile: $(srcdir)/$(1)/configure | $(srcdir)/$(name
)/build
  (cd $(srcdir)/$(1)/build; . ../$(srcdir)/$(1)/configure)

$(srcdir)/$(1)/build: | $(srcdir)/$(1)
  mkdir $$@

$(srcdir)/$(1)/configure: | $(srcdir)/$(1)/configure.ac
  (cd $(srcdir)/$(1); . ./autogen.sh)
endef

define make_local_git_fn
$(srcdir)/$(1)/configure.ac:
	if [[ -d "$(2)" ]]; then \
		git clone $(2) $(srcdir)/$(1)/ \
	else \
		git clone $(3) $(srcdir)/$(1)/ \
	fi
endef

make_sublib=$(eval $(call make_sublib_fn, $(1), $(2)))
make_local_git=$(eval $(call make_local_git_fn, $(1), $(2), $(3)))
