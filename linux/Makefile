
CFLAGS=-g -pthread -fsanitize=address -Wall -Wextra --pedantic -g
GTKCFLAGS:=$(subst -I,-isystem ,$(shell pkg-config --cflags gtk+-2.0))
GTKLDFLAGS:=$(shell pkg-config --libs gtk+-2.0) $(shell pkg-config --libs gthread-2.0)

BADGE_SRC=../src
BADGE_INC=../include
LINUX_OBJS=linuxcompat.o bline.o achievements.o xorshift.o

APPDIR=../badge_apps
MAZEPICS=${APPDIR}/maze_drawings
MONSTERS=${APPDIR}/badge_monster_drawings
IRXMIT=irxmit
LASERTAG=lasertag
SMASHOUT=smashout
GHOSTDETECTOR=ghost-detector
MAZE=maze
USERNAME=username
LUNARLANDER=lunarlander
HACKING_SIMULATOR=hacking_simulator
GAME_OF_LIFE=game_of_life
SPACETRIPPER=spacetripper

all:	${LINUX_OBJS} bin/${MAZE} bin/${IRXMIT} bin/${LASERTAG} bin/badge_monsters bin/fixup_monster bin/username \
	bin/sample_app bin/smashout bin/ghost-detector bin/lunarlander bin/${HACKING_SIMULATOR} bin/${SPACETRIPPER} bin/${GAME_OF_LIFE}

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
			${MONSTERS}/eddie.h \
			${MONSTERS}/freshmon.h \
			${MONSTERS}/goat_mon.h \
			${MONSTERS}/godzilla.h \
			${MONSTERS}/gophermon.h \
			${MONSTERS}/heartbleed.h \
			${MONSTERS}/hrvamon.h \
			${MONSTERS}/mcturtle.h \
			${MONSTERS}/octocat.h \
			${MONSTERS}/octomon.h \
			${MONSTERS}/othermon.h \
			${MONSTERS}/smileymon.h \
			${MONSTERS}/spectre.h \
			${MONSTERS}/stacksmasher.h \
			${MONSTERS}/tuxmon.h \
			${MONSTERS}/worm.h \
			${MONSTERS}/zombieload.h
	$(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} -o bin/badge_monsters -I . -I ${BADGE_INC} ${APPDIR}/badge_monsters.c ${GTKLDFLAGS}

bin/sample_app:	${BIN} ${APPDIR}/sample_app.c linuxcompat.o bline.o ${LINUX_OBJS} \
			${MONSTERS}/smileymon.h
	$(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} -o bin/sample_app -I . -I ${BADGE_INC} ${APPDIR}/sample_app.c ${GTKLDFLAGS}

bin/fixup_monster:	${BIN} fixup_monster.c
	$(CC) ${CFLAGS} -o bin/fixup_monster fixup_monster.c 

bin/${IRXMIT}:	${BIN} ${APPDIR}/${IRXMIT}.c linuxcompat.o bline.o
	$(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} -o bin/${IRXMIT} -I . -I ${BADGE_INC} ${APPDIR}/${IRXMIT}.c ${GTKLDFLAGS}

bin/${LASERTAG}:	${BIN} ${APPDIR}/${LASERTAG}.c linuxcompat.o bline.o
	$(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} -o bin/${LASERTAG} -I . -I ${BADGE_INC} ${APPDIR}/${LASERTAG}.c ${GTKLDFLAGS}

bin/${SMASHOUT}:	${BIN} ${APPDIR}/${SMASHOUT}.c linuxcompat.o
	$(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} -o bin/${SMASHOUT} -I . -I ${BADGE_INC} ${APPDIR}/${SMASHOUT}.c ${GTKLDFLAGS}

bin/${GHOSTDETECTOR}:	${BIN} ${APPDIR}/${GHOSTDETECTOR}.c linuxcompat.o
	$(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} -o bin/${GHOSTDETECTOR} -I . -I ${BADGE_INC} ${APPDIR}/${GHOSTDETECTOR}.c ${GTKLDFLAGS}

bin/${LUNARLANDER}:	${BIN} ${APPDIR}/${LUNARLANDER}.c linuxcompat.o ${LINUX_OBJS}
	$(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} -o bin/${LUNARLANDER} -I . -I ${BADGE_INC} ${APPDIR}/${LUNARLANDER}.c ${GTKLDFLAGS}

bin/hacking_simulator:	${BIN} ${APPDIR}/${HACKING_SIMULATOR}.c linuxcompat.o bline.o ${LINUX_OBJS} dynmenu.o
	$(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} dynmenu.o -o bin/${HACKING_SIMULATOR} -I . -I ${BADGE_INC} ${APPDIR}/${HACKING_SIMULATOR}.c ${GTKLDFLAGS}

bin/game_of_life:	${BIN} ${APPDIR}/${GAME_OF_LIFE}.c linuxcompat.o bline.o ${LINUX_OBJS} 
	$(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} -o bin/${GAME_OF_LIFE} -I . -I ${BADGE_INC} ${APPDIR}/${GAME_OF_LIFE}.c ${GTKLDFLAGS}

dynmenu.o:
	$(CC) -c ${CFLAGS} -I../include ../src/dynmenu.c

bin/spacetripper:	${BIN} ${APPDIR}/${SPACETRIPPER}.c linuxcompat.o bline.o ${LINUX_OBJS} dynmenu.o
	$(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} -o bin/${SPACETRIPPER} -I . -I ${BADGE_INC} ${APPDIR}/${SPACETRIPPER}.c ${GTKLDFLAGS} dynmenu.o

bin/${MAZE}:	${BIN} ${APPDIR}/${MAZE}.c linuxcompat.o bline.o xorshift.o achievements.o dynmenu.o \
			${MAZEPICS}/bones_points.h \
			${BADGE_INC}/build_bug_on.h \
			${MAZEPICS}/chalice_points.h \
			${MAZEPICS}/chest_points.h \
			${MAZEPICS}/cobra_points.h \
			${MAZEPICS}/down_ladder_points.h \
			${MAZEPICS}/dragon_points.h \
			${MAZEPICS}/grenade_points.h \
			${MAZEPICS}/orc_points.h \
			${MAZEPICS}/phantasm_points.h \
			${MAZEPICS}/player_points.h \
			${MAZEPICS}/potion_points.h \
			${MAZEPICS}/scroll_points.h \
			${MAZEPICS}/shield_points.h \
			${MAZEPICS}/sword_points.h \
			${MAZEPICS}/up_ladder_points.h
	$(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} -o bin/${MAZE} -I . -I ${BADGE_INC} ${APPDIR}/${MAZE}.c ${GTKLDFLAGS} dynmenu.o 

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

