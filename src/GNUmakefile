include $(ROOT)/usr/include/make/PRdefs

APP =		xinput

OPTIMIZER =	-g -O2

TARGETS =	rom

CODE_HFILES =	xinput.h

CODE_CFILES =	xinput.c graph.c asci.c usb.c

CODE_OBJECTS=	$(CODE_CFILES:.c=.o)

CODESEGMENT =	code.o

LCINCS =	-I. -I$(ROOT)/usr/include/PR
LCOPTS =	-mips2 -Wall -non_shared -G 0 -g

LASINCS =	$(LCINCS)
LASOPTS =	-non_shared -G 0

LDIRT  =	$(APP) 

LDFLAGS =	$(MKDEPOPT) -L$(ROOT)/usr/lib -lultra_d -lgcc --no-warn-mismatch

default:	$(TARGETS)

include $(COMMONRULES)

install:	default

$(CODESEGMENT):		$(CODE_OBJECTS)
		$(LD) -o $@ -r $(CODE_OBJECTS) $(LDFLAGS)

$(TARGETS) $(APP):	spec code.o $(MAKEROM) 
		$(MAKEROM) -r $(TARGETS) spec
		dd if=rom of=008aeae5.z64 bs=16384 conv=sync

