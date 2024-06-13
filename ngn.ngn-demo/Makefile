subdirs = acm-demo monitoring-client

goals = all install clean

# $(call delegate,<goal>,<list of subdirs>)
define delegate
$1::
	@true $(foreach d,$2,&& $(MAKE) --no-print-directory -C $d $1)

endef

# delegate to all given goals
$(eval $(foreach g,$(goals),$(call delegate,$g,$(subdirs))))

