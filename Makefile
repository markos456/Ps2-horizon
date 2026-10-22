EE_BIN = horizon.elf
EE_OBJS = source/main.o
EE_LIBS = -lgskit -ldmakit -lpad -lm
EE_INCS += -I$(GSKIT)/include

all: $(EE_BIN)

clean:
\trm -f $(EE_BIN) $(EE_OBJS)

include $(PS2SDK)/samples/Makefile.eeglobal
