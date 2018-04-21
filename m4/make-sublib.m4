AC_DEFUN([MAKE_SUBLIB],[

$(srcdir)/@name@/build/@target@: | $(srcdir)/@name@/build/Makefile 
  $(MAKE) -C $(srcdir)/@name@/build

$(srcdir)/@name@/build/Makefile: $(srcdir)/@name@/configure | $(srcdir)/$(name
)/build
  (cd $(srcdir)/@name@/build; . ../$(srcdir)/@name@/configure)

$(srcdir)/@name@/build: | $(srcdir)/@name@
  mkdir $$@

$(srcdir)/@name@/configure: | $(srcdir)/@name@/configure.ac
  (cd $(srcdir)/@name@; . ./autogen.sh)
])

AC_DEFUN([MAKE_LOCAL_GIT], [
$(srcdir)/@name@/configure.ac:
	if [[ -d "@repodir@" ]]; then \
		git clone @repodir@ $(srcdir)/@name@/; \
	else \
		git clone @repourl@ $(srcdir)/@name@/ \
	fi
endef
