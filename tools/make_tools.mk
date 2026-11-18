
MAKEFLAGS += --no-print-directory

# 只重建自带 Makefile 的工具目录；agbcc 随仓库提供，不重建。
TOOLDIRS := $(patsubst %/Makefile,%,$(wildcard tools/*/Makefile))

.PHONY: all $(TOOLDIRS) clean

all: $(TOOLDIRS)

$(TOOLDIRS):
	@$(MAKE) -C $@

clean:
	@$(foreach tooldir,$(TOOLDIRS),$(MAKE) clean -C $(tooldir);)
