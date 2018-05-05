


#CROSS_COMPILE :=  ~/mytools/gcc-linaro-4.9/bin/arm-linux-gnueabihf-
CROSS_COMPILE := $(TOPDIR)/../armv8_toolchain/gcc-linaro-arm-linux-gnueabihf-4.9-2014.09_linux/bin/arm-linux-gnueabihf-

AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
LDR		= $(CROSS_COMPILE)ldr
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump

##########################################################
COMPILEINC :=  -isystem $(shell dirname `$(CC)  -print-libgcc-file-name`)/include 
SPLINCLUDE    := \
		-I$(SRCTREE)/include \
		-I$(SRCTREE)/arch/arm/include \
		-I$(SPLDIR)/include 
		
PLATFORM_RELFLAGS +=  -march=armv8-a  

 COMM_FLAGS := -nostdinc  $(COMPILEINC) \
 -g  -Wall -Os   -fno-common -msoft-float -mfpu=neon  \
 -fno-builtin -ffreestanding \
 -D__KERNEL__  \
 -DCONFIG_ARM -D__ARM__ \
 -D__NEON_SIMD__  \
 -mabi=aapcs-linux \
 -mthumb-interwork \
 -Wstrict-prototypes -fno-stack-protector \
 -Wno-format-nonliteral \
 -Wno-format-security  \
 -pipe 


  
C_FLAGS += $(SPLINCLUDE)   $(COMM_FLAGS) 
S_FLAGS += $(SPLINCLUDE)   -D__ASSEMBLY__  $(COMM_FLAGS) 
#LDFLAGS += --gap-fill=0xff 
###########################################################

###########################################################
PLATFORM_LIBGCC = -L $(shell dirname `$(CC) $(CFLAGS) -print-libgcc-file-name`) -lgcc
export PLATFORM_LIBGCC
###########################################################

# Allow boards to use custom optimize flags on a per dir/file basis
ALL_AFLAGS = $(AFLAGS)  $(PLATFORM_RELFLAGS) $(S_FLAGS)
ALL_CFLAGS = $(CFLAGS)  $(PLATFORM_RELFLAGS) $(C_FLAGS)
export ALL_CFLAGS ALL_AFLAGS


$(obj)%.o:	%.S
	@$(CC)  $(ALL_AFLAGS) -o $@ $< -c
	@echo " CC      "$< ...
$(obj)%.o:	%.c
	@$(CC)  $(ALL_CFLAGS) -o $@ $< -c
	@echo " CC      "$< ...

#########################################################################

# If the list of objects to link is empty, just create an empty built-in.o
cmd_link_o_target = $(if $(strip $1),\
		      @$(LD) $(LDFLAGS) -r -o $@ $1,\
		      rm -f $@; $(AR) rcs $@ )

#########################################################################


sinclude $(SPLDIR)/autoconf.mk