ifeq ($(CPU),stm32f407vgt6)
	DIRS = cortex-m stm32f407vgt6
endif
ifeq ($(CPU),stm32f103rey6)
	DIRS = STM32F10x_StdPeriph_Lib_V3.5.0 stm32f103rey6
endif

.PHONY: cpus
.PHONY: $(DIRS)

cpus: $(DIRS)

$(DIRS): 
	@$(MAKE) -C $@

clean:
	@for i in $(DIRS) ; do $(MAKE) -C $$i clean ; done ;
