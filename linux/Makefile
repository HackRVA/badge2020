
CFLAGS=-pthread -fsanitize=address -Wall --pedantic -g
GTKCFLAGS:=$(subst -I,-isystem ,$(shell pkg-config --cflags gtk+-2.0))
GTKLDFLAGS:=$(shell pkg-config --libs gtk+-2.0) $(shell pkg-config --libs gthread-2.0)

BADGE_SRC=../src
BADGE_INC=../include
LINUX_OBJS=linuxcompat.o bline.o achievements.o xorshift.o

APPDIR=../badge_apps
IRXMIT=irxmit
LASERTAG=lasertag
SMASHOUT=smashout
MAZE=maze
USERNAME=username

all:	${LINUX_OBJS} bin/${MAZE} bin/${IRXMIT} bin/${LASERTAG} bin/badge_monsters bin/fixup_monster bin/username bin/sample_app bin/smashout

# we need to depend on ${BIN}/.t not on ${BIN} because BIN is a directory and the
# timestamp changes whenever the directory contents change, and we need to depend
# only on the *existence* of the directory, not on the contents.
BIN=bin/.t

${BIN}:
	@mkdir -p bin
	@if [ ! -f ${BIN} ] ; then \
		touch ${BIN}  ; \
	fi

bin/badge_monsters:	${BIN} ${APPDIR}/badge_monsters.c linuxcompat.o bline.o ${LINUX_OBJS} \
			${APPDIR}/eddie.h \
			${APPDIR}/freshmon.h \
			${APPDIR}/goat_mon.h \
			${APPDIR}/godzilla.h \
			${APPDIR}/gophermon.h \
			${APPDIR}/heartbleed.h \
			${APPDIR}/hrvamon.h \
			${APPDIR}/mcturtle.h \
			${APPDIR}/octocat.h \
			${APPDIR}/octomon.h \
			${APPDIR}/othermon.h \
			${APPDIR}/smileymon.h \
			${APPDIR}/spectre.h \
			${APPDIR}/stacksmasher.h \
			${APPDIR}/tuxmon.h \
			${APPDIR}/worm.h \
			${APPDIR}/zombieload.h
	$(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} -o bin/badge_monsters -I . -I ${BADGE_INC} ${APPDIR}/badge_monsters.c ${GTKLDFLAGS}

bin/sample_app:	${BIN} ${APPDIR}/sample_app.c linuxcompat.o bline.o ${LINUX_OBJS} \
			${APPDIR}/smileymon.h
	$(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} -o bin/sample_app -I . -I ${BADGE_INC} ${APPDIR}/sample_app.c ${GTKLDFLAGS}

bin/fixup_monster:	${BIN} fixup_monster.c
	$(CC) ${CFLAGS} -o bin/fixup_monster fixup_monster.c 

bin/${IRXMIT}:	${BIN} ${APPDIR}/${IRXMIT}.c linuxcompat.o bline.o
	$(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} -o bin/${IRXMIT} -I . -I ${BADGE_INC} ${APPDIR}/${IRXMIT}.c ${GTKLDFLAGS}

bin/${LASERTAG}:	${BIN} ${APPDIR}/${LASERTAG}.c linuxcompat.o bline.o
	$(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} -o bin/${LASERTAG} -I . -I ${BADGE_INC} ${APPDIR}/${LASERTAG}.c ${GTKLDFLAGS}

bin/${SMASHOUT}:	${BIN} ${APPDIR}/${SMASHOUT}.c linuxcompat.o
	$(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} -o bin/${SMASHOUT} -I . -I ${BADGE_INC} ${APPDIR}/${SMASHOUT}.c ${GTKLDFLAGS}

bin/${MAZE}:	${BIN} ${APPDIR}/${MAZE}.c linuxcompat.o bline.o xorshift.o achievements.o\
			${APPDIR}/bones_points.h \
			${BADGE_INC}/build_bug_on.h \
			${APPDIR}/chalice_points.h \
			${APPDIR}/chest_points.h \
			${APPDIR}/cobra_points.h \
			${APPDIR}/down_ladder_points.h \
			${APPDIR}/dragon_points.h \
			${APPDIR}/grenade_points.h \
			${BADGE_INC}/lasertag.h \
			${APPDIR}/lasertag-protocol.h \
			${APPDIR}/orc_points.h \
			${APPDIR}/phantasm_points.h \
			${APPDIR}/player_points.h \
			${APPDIR}/potion_points.h \
			${APPDIR}/scroll_points.h \
			${APPDIR}/shield_points.h \
			${APPDIR}/sword_points.h \
			${APPDIR}/up_ladder_points.h
	$(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} -o bin/${MAZE} -I . -I ${BADGE_INC} ${APPDIR}/${MAZE}.c ${GTKLDFLAGS}

bin/${USERNAME}:	${BIN} ${APPDIR}/${USERNAME}.c
	$(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} -o bin/${USERNAME} -I . -I ${BADGE_INC} ${APPDIR}/${USERNAME}.c ${GTKLDFLAGS}

linuxcompat.o:	linuxcompat.c linuxcompat.h
	$(CC) ${CFLAGS} ${GTKCFLAGS} -c -I . linuxcompat.c -o linuxcompat.o

bline.o:	bline.c bline.h
	$(CC) ${CFLAGS} -c bline.c -o bline.o

achievements.o:	${BADGE_SRC}/achievements.c ${BADGE_INC}/achievements.h
	$(CC) ${CFLAGS} -c ${BADGE_SRC}/achievements.c -I ${BADGE_INC} -o achievements.o

xorshift.o:	${APPDIR}/xorshift.c ${APPDIR}/xorshift.h
	$(CC) ${CFLAGS} ${GTKCFLAGS} -c ${APPDIR}/xorshift.c -o xorshift.o

scan-build:
	make clean
	rm -fr /tmp/badge-scan-build-output
	scan-build -o /tmp/badge-scan-build-output make CC=clang
	xdg-open /tmp/badge-scan-build-output/*/index.html

clean:
	rm -f *.o
	rm -fr bin

