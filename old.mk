include coolmake/main.mk

coolmake/top.mk coolmake/main.mk: | coolmake
	$(MAKE)
coolmake: | html_when/libxmlfixes/coolmake
	ln -rs $| $@

html_when/libxmlfixes/coolmake: | html_when
	$(MAKE) -C $|

html_when:
	sh setup.sh
