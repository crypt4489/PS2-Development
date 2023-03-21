IOP_BIN = proc_texture.irx

IOP_OBJS = checkerboard.o perlin.o psxgte.o rpc_client.o rpc_server.o proc_texture_main.o proc_texture_exports.o imports.o exports.o

all: $(IOP_BIN)
	cp $(IOP_BIN) ../irx/

clean:
	rm -f -r $(IOP_OBJS) $(IOP_BIN)

include $(PS2SDK)/Defs.make
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.iopglobal


