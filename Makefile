-include Config.mk

################ Source files ##########################################

EXE	:= ${NAME}
SRCS	:= $(wildcard *.c)
OBJS	:= $(addprefix $O,$(SRCS:.c=.o))

################ Compilation ###########################################

.PHONY: all clean dist distclean maintainer-clean

all:	Config.mk config.h ${EXE} ${DATAF}

run:	${EXE} ${DATAF}
	@./${EXE}

${EXE}:	${OBJS}
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ $^ ${LIBS}

$O%.o:	%.c
	@echo "    Compiling $< ..."
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	@${CC} ${CFLAGS} -MMD -MT "$(<:.c=.s) $@" -o $@ -c $<

%.s:	%.c
	@echo "    Compiling $< to assembly ..."
	@${CC} ${CFLAGS} -S -o $@ -c $<

################ Installation ##########################################

.PHONY:	install uninstall

ifdef BINDIR
EXEI	:= $(addprefix ${BINDIR}/,${EXE})

install:	${EXEI}
${EXEI}:	${EXE}
	@echo "Installing $< as $@ ..."
	@${INSTALLEXE} $< $@

uninstall:
	@echo "Removing ${EXEI} ..."
	@rm -f ${EXEI}
endif

################ Maintenance ###########################################

clean:
	@[ ! -d ./$O ] || rm -rf ./$O
	@rm -f ${EXE}

ifdef MAJOR
DISTVER	:= ${MAJOR}.${MINOR}
DISTNAM	:= ${NAME}-${DISTVER}
DISTTAR	:= ${DISTNAM}.tar.bz2

dist:
	@echo "Generating ${DISTTAR} and ${DISTLSM} ..."
	@mkdir .${DISTNAM}
	@rm -f ${DISTTAR}
	@cp -r * .${DISTNAM} && mv .${DISTNAM} ${DISTNAM}
	@+${MAKE} -sC ${DISTNAM} maintainer-clean
	@tar acf ${DISTTAR} ${DISTNAM} && rm -rf ${DISTNAM}
endif

distclean:	clean
	@rm -f Config.mk config.h config.status

maintainer-clean: distclean

${OBJS}:		Makefile Config.mk config.h
Config.mk:		Config.mk.in
config.h:		config.h.in
Config.mk config.h:	configure
	@if [ -x config.status ]; then echo "Reconfiguring ..."; ./config.status; \
	else echo "Running configure ..."; ./configure; fi

-include ${OBJS:.o=.d}
