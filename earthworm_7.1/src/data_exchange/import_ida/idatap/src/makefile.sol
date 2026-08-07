# Makefile for idatap library

CC      = cc
RANLIB  = ranlib
LIBDIR  = ..
INCDIR  = ../include
OPTMIZ  = -g
INCS    = -I$(INCDIR)
DEFS    = -DSOLARIS
CFLAGS  = $(OPTMIZ) $(INCS) $(DEFS)
ARCHIVE = libidatap.a
OUTPUT  = $(LIBDIR)/$(ARCHIVE)
OBJS    = $(OUTPUT)(case.o)
OBJS   += $(OUTPUT)(client.o)
OBJS   += $(OUTPUT)(common.o)
OBJS   += $(OUTPUT)(compress.o)
OBJS   += $(OUTPUT)(convert.o)
OBJS   += $(OUTPUT)(ezio.o)
OBJS   += $(OUTPUT)(getline.o)
OBJS   += $(OUTPUT)(globals.o)
OBJS   += $(OUTPUT)(lock.o)
OBJS   += $(OUTPUT)(misc.o)
OBJS   += $(OUTPUT)(parse.o)
OBJS   += $(OUTPUT)(rdwr.o)
OBJS   += $(OUTPUT)(signal.o)
OBJS   += $(OUTPUT)(string.o)
OBJS   += $(OUTPUT)(swap.o)
OBJS   += $(OUTPUT)(syserr.o)
OBJS   += $(OUTPUT)(timefunc.o)
OBJS   += $(OUTPUT)(utillog.o)
OBJS   += $(OUTPUT)(xferlog.o)

.PRECIOUS : $(OUTPUT)

all: $(OUTPUT)

$(OUTPUT): $(OBJS)

FORCE:
