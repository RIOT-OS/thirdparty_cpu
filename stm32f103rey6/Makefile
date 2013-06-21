MODULE =cpu

INCLUDES = -Iinclude
INCLUDES += -I$(RIOTCPU)/STM32F10x_StdPeriph_Lib_V3.5.0/Libraries/CMSIS/Include
INCLUDES += -I$(RIOTCPU)/STM32F10x_StdPeriph_Lib_V3.5.0/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x
INCLUDES += -I$(RIOTCPU)/STM32F10x_StdPeriph_Lib_V3.5.0/Libraries/STM32F10x_StdPeriph_Driver/inc
INCLUDES += -I$(RIOTBASE)/core/include -I$(RIOTBASE)/sys/include -I$(RIOTBASE)/sys/lib 

DIRS =
ifneq (,$(findstring cpu_drivers,$(USEMODULE)))
    DIRS += drivers
endif

all: $(BINDIR)$(MODULE).a
	@for i in $(DIRS) ; do "$(MAKE)" -C $$i ; done ;

include $(MAKEBASE)/Makefile.base

clean::
	@for i in $(DIRS) ; do "$(MAKE)" -C $$i clean ; done ;
