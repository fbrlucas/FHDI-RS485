# STM32 application builder
#
# Author: Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>
# License: BSD
# 

# makefile template

CFLAGS=$(G_CFLAGS) -c
CPPFLAGS=$(G_CPPFLAGS) -c
LDFLAGS=$(G_LDFLAGS) 
INCS=$(G_INCS)
LIBS=$(G_LIBS)
SRCDIR=$(G_SRCDIR)
BLDDIR=$(G_BLDDIR)
LIBDIR=$(G_LIBDIR)
LDFILE=$(G_LDFILE)

C_OBJS=$(C_SRCS:%.c=output/%.o)
CPP_OBJS=$(CPP_SRCS:%.cpp=output/%.o)
ASM_OBJS=$(ASM_SRCS:%.S=output/%.o)

#$(info ----------)
#$(info $(BLDDIR))
#$(info $(SRCDIR))
#$(info $(C_OBJS))
#$(info ----------)

all: $(BLDDIR) $(STATICLIB) $(EXEC) 

# creating output directory
$(BLDDIR):
	-mkdir -p ../$(BLDDIR)
	-mkdir -p $(BLDDIR)

# generating executable
$(EXEC): $(C_OBJS) $(CPP_OBJS) $(ASM_OBJS) $(LDFILE)
	$(CC) $(LDFLAGS) $(C_OBJS) $(CPP_OBJS) $(ASM_OBJS) $(LIBS) $(LIBDIR) -o $(EXEC) 
	$(COPY) -O binary $(EXEC) $(FLASH)
	$(SIZE) $(EXEC)

# generating lib
$(STATICLIB): $(C_OBJS) $(CPP_OBJS) $(ASM_OBJS)
	$(AR) -cvr $(STATICLIB) $(C_OBJS) $(ASM_OBJS) $(CPP_OBJS) $(LIBS)

# building generic c files
$(BLDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(INCS) $(CFLAGS) $< -o $@

# building generic cpp files
$(BLDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CPP) $(INCS) $(CPPFLAGS) $< -o $@
    
# building generic asm files
$(BLDDIR)/%.o: $(SRCDIR)/%.S
	$(CC) $(INCS) $(CFLAGS) $< -o $@

clean:
	-rm -fR $(BLDDIR)

cscope:
	@for f in $(C_SRCS) $(CPP_SRCS) $(ASM_SRCS); do \
		echo $(shell pwd)/$(SRCDIR)/$$f >> ../$(BLDDIR)/cscope.files ; \
	done 

.PHONY: cscope
.PHONY: $(LDFILE)
