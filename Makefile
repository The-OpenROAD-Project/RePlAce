SUBDIR = module/lef/5.8-p027 module/def/5.8-p027 module/verilog-parser/src 
HASHDIR = module/sparsehash
REPLACEDIR = src


all: prep hash
	$(MAKE) -C $(REPLACEDIR);

hash:
	echo $(MAKEFLAGS)
	cd $(HASHDIR); \
	mkdir -p install-sp; \
	./configure --prefix=$(CURDIR)/$(HASHDIR)/install-sp; \
	$(MAKE); \
	$(MAKE) install;

prep: 
	for dir in $(SUBDIR); do \
		$(MAKE) -C $$dir; \
	done

clean:
	for dir in $(SUBDIR); do \
		$(MAKE) -C $$dir clean > /dev/null 2>&1; \
	done;
	cd $(HASHDIR) && $(MAKE) distclean && rm -rf install-sp > /dev/null 2>&1; true 
	cd $(REPLACEDIR) && $(MAKE) clean;
