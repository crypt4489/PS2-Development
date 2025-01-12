IRX_DIR := ./irx
ASSETS_DIR := ./assets
OBJS_DIR := ./objs
VCL_DIR := ./vcl
VSM_DIR := ./vcl/vsm
VSM_OBJ_DIR := ./vcl/vsm/objs
SRC_DIR := ./src
TARGET_DIR := ./bin
INC_DIR := ./inc/
WIN_DIR := /mnt/c/Users/dflet/Desktop/
OPL_DIR := /mnt/c/Users/Public/ps2/DVD/

OBJEXT    := o
CEXT      := c
VSMEXT    := vsm

EE_BIN = ABCD_123.45
EE_LIB = libps2.a

BUILD_TOOL ?= EE-CC

IOP_MOD = sio2man.irx padman.irx sound.irx libsd.irx audsrv.irx

EE_SRC_VSM := $(shell find $(VSM_DIR) -maxdepth 1 -type f -name *.$(VSMEXT))
EE_OBJS :=  $(patsubst $(VSM_DIR)/%, $(VSM_OBJ_DIR)/%, $(EE_SRC_VSM:.$(VSMEXT)=.$(OBJEXT)))

EE_SRC_C += $(shell find $(SRC_DIR) -type f -name *.$(CEXT))
EE_OBJS += $(patsubst $(SRC_DIR)/%, $(OBJS_DIR)/%, $(EE_SRC_C:.$(CEXT)=.$(OBJEXT)))

ifeq ($(BUILD_TOOL), EE-AR)
  FILTER := $(OBJS_DIR)/main.o $(OBJS_DIR)/skybox.o $(OBJS_DIR)/pad.o $(OBJS_DIR)/body.o
  EE_OBJS := $(filter-out $(FILTER), $(EE_OBJS)) 
endif


EE_DVP = dvp-as

EE_LIBS=-ldma -lgraph -ldraw -lkernel -lpacket -lpad -lcdvd -lpng -lz -lunzip -laudsrv

PS2SDK=/usr/local/ps2dev/ps2sdk

LOG_LEVEL ?= 3

EE_CFLAGS += -DPS_LOG_LVL=$(LOG_LEVEL) -Wall -Wno-strict-aliasing -Wno-char-subscripts --std=gnu99 -I$(INC_DIR)
EE_LDFLAGS = -L$(PS2SDK)/ee/common/lib -L$(PS2SDK)/ee/lib -L$(PS2SDK)/ports/lib

ISO_TGT=test.iso

include ./Makefile.eeglobal
include $(PS2SDK)/samples/Makefile.pref

.PHONY : all clean compile directories disc irx update_win archive

all: compile disc update_win

archive:  
	make BUILD_TOOL=EE-AR

compile:
	make

directories:
	if [ ! -d $(IRX_DIR) ]; then mkdir -p $(IRX_DIR) ; fi

	if [ ! -d $(ASSETS_DIR) ]; then mkdir -p $(ASSETS_DIR) ; fi

	if [ ! -d $(OBJS_DIR) ]; then mkdir -p $(OBJS_DIR) ; fi

	if [ ! -d $(TARGET_DIR) ]; then mkdir -p $(TARGET_DIR) ; fi

	if [ ! -d $(INC_DIR) ]; then mkdir -p $(INC_DIR) ; fi

update_win:
	if [ -d $(WIN_DIR) ]; then cp $(ISO_TGT) $(WIN_DIR) ; fi
	if [ -d $(OPL_DIR) ]; then cp $(ISO_TGT) $(OPL_DIR) ; fi

disc:
	mkisofs -l -o $(ISO_TGT) $(TARGET_DIR)/$(EE_BIN) SYSTEM.CNF $(IRX_DIR) $(ASSETS_DIR)

irx:
	mv $(IOP_MOD) "$(IRX_DIR)"

clean:
	rm -rf $(ISO_TGT) $(TARGET_DIR)/$(EE_BIN) $(EE_OBJS)

%.vcl:
	make -B -C $(VCL_DIR) $@