IOP_BIN = sound.irx

IOP_OBJS = sound.o rpc_server.o rpc_client.o wmemcpy.o imports.o exports.o

all: $(IOP_BIN)
	cp $(IOP_BIN) ../../irx/

clean:
	rm -f -r $(IOP_OBJS) $(IOP_BIN)

include $(PS2SDK)/Defs.make
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.iopglobal
