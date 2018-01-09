#
# This makefile should work with no changes for compiling under SunOS, Ultrix,
# NeXTSTEP, HP/UX, DG/UX, and Linux.  If you're using AIX, IRIX, some version
# of SVR4 or Solaris, uncomment the appropriate lines below.  If you're using
# some other operating system not mentioned, just type 'make' and cross your
# fingers..

#compiler name and flags
CC = gcc32
MYFLAGS = -g -Wall
LIBS = -lcrypt

#flags for profiling (see hacker.doc for more information)
#PROFILE = -pg

#remove the hash marks below if compiling under AIX
#CC = cc
#MYFLAGS = -g

#remove the hash marks below if compiling under IRIX
#CC = cc -cckr -fullwarn
#MYFLAGS = -g
#LIBS = -lmalloc

#remove the hash mark below if compiling under SVR4 or Solaris, or if your
#linker complains that the socket functions such as 'connect()', 'bind()',
#etc are undefined.
#LIBS = -lsocket -lnsl

#############################################################################

CFLAGS = $(MYFLAGS) $(PROFILE)

OBJFILES = \
	act.comm.o \
	act.informative.o \
	act.movement.o \
	act.obj.o \
	act.offensive.o \
	act.other.o \
	act.social.o \
	act.wizard.o \
	ascii_pfiles.o \
	auction.o \
	ban.o \
	boards.o \
	clan.o \
	class.o \
	creation.o \
	comm.o \
	config.o \
	constants.o \
	db.o \
	dshop.o \
	fight.o \
	foldarea.o \
	graph.o \
	handler.o \
	house.o \
	interpreter.o \
	limits.o \
	magic.o \
	mail.o \
	medit.o \
	mobact.o \
	mobcmd.o \
	mobprog.o \
	modify.o \
	objprog.o \
	olc.o \
	oedit.o \
	objsave.o \
	pawnshop.o \
	pets.o \
	quest.o \
	race.o \
	random.o \
	redit.o \
	screen.o \
	sedit.o \
	ship.o \
	shop.o \
	shop2.o \
	spec_assign.o \
	spec_procs.o \
	spell_parser.o \
	spells.o \
	textsave.o \
	timed.o \
	utils.o \
	weather.o \
	zedit.o

../bin/circle : $(OBJFILES)

utils:
	$(MAKE) ../bin/autowiz
	$(MAKE) ../bin/delobjs
	$(MAKE) ../bin/hasmail
	$(MAKE) ../bin/listrent
	$(MAKE) ../bin/mailindex
	$(MAKE) ../bin/mudpasswd
	$(MAKE) ../bin/purgeplay
	$(MAKE) ../bin/readmail
	$(MAKE) ../bin/showplay
	$(MAKE) ../bin/sign
	$(MAKE) ../bin/split
	$(MAKE) ../bin/syntax_check

all:
#	$(MAKE) ../bin/circle
#	$(MAKE) utils
	make circle

circle:
	$(MAKE) ../bin/circle
autowiz:
	$(MAKE) ../bin/autowiz
delobjs:
	$(MAKE) ../bin/delobjs
hasmail:
	$(MAKE) ../bin/hasmail
listrent:
	$(MAKE) ../bin/listrent
mailindex:
	$(MAKE) ../bin/mailindex
mudpasswd:
	$(MAKE) ../bin/mudpasswd
purgeplay:
	$(MAKE) ../bin/purgeplay
readmail:
	$(MAKE) ../bin/readmail
showplay:
	$(MAKE) ../bin/showplay
sign:
	$(MAKE) ../bin/sign
split:
	$(MAKE) ../bin/split
syntax_check:
	$(MAKE) ../bin/syntax_check

../bin/autowiz : util/autowiz.c structs.h db.h utils.h
	$(CC) $(CFLAGS) -o ../bin/autowiz util/autowiz.c
../bin/delobjs : util/delobjs.c structs.h
	$(CC) $(CFLAGS) -o ../bin/delobjs util/delobjs.c
../bin/hasmail : util/hasmail.c mail.h
	$(CC) $(CFLAGS) -o ../bin/hasmail util/hasmail.c
../bin/listrent : util/listrent.c structs.h
	$(CC) $(CFLAGS) -o ../bin/listrent util/listrent.c
../bin/mailindex : util/mailindex.c mail.h
	$(CC) $(CFLAGS) -o ../bin/mailindex util/mailindex.c
../bin/mudpasswd   : util/mudpasswd.c structs.h
	$(CC) $(CFLAGS) -o ../bin/mudpasswd util/mudpasswd.c
../bin/purgeplay : util/purgeplay.c structs.h
	$(CC) $(CFLAGS) -o ../bin/purgeplay util/purgeplay.c
../bin/readmail : util/readmail.c mail.h
	$(CC) $(CFLAGS) -o ../bin/readmail util/readmail.c
../bin/showplay : util/showplay.c structs.h
	$(CC) $(CFLAGS) -o ../bin/showplay util/showplay.c
../bin/sign: util/sign.c
	$(CC) $(CFLAGS) -o ../bin/sign $(COMMFLAGS) util/sign.c
../bin/split: util/split.c
	$(CC) $(CFLAGS) -o ../bin/split util/split.c
../bin/syntax_check: util/syntax_check.c structs.h utils.h db.h comm.h \
	handler.h
	$(CC) $(CFLAGS) -o ../bin/syntax_check util/syntax_check.c

clean:
	rm -f *.o

# Dependencies for the main mud

comm.o : comm.c structs.h utils.h comm.h interpreter.h handler.h db.h \
	house.h olc.h
	$(CC) -c $(CFLAGS) comm.c
act.comm.o : act.comm.c structs.h utils.h comm.h interpreter.h handler.h \
	db.h screen.h
	$(CC) -c $(CFLAGS) act.comm.c
act.informative.o : act.informative.c structs.h utils.h comm.h interpreter.h \
	handler.h db.h spells.h screen.h
	$(CC) -c $(CFLAGS) act.informative.c
act.movement.o : act.movement.c structs.h utils.h comm.h interpreter.h \
	handler.h db.h spells.h house.h
	$(CC) -c $(CFLAGS) act.movement.c
act.obj.o : act.obj.c structs.h utils.h comm.h interpreter.h handler.h \
	db.h spells.h
	$(CC) -c $(CFLAGS) act.obj.c
act.offensive.o : act.offensive.c structs.h utils.h comm.h interpreter.h \
	handler.h db.h spells.h
	$(CC) -c $(CFLAGS) act.offensive.c
act.other.o : act.other.c structs.h utils.h comm.h interpreter.h handler.h \
	db.h spells.h screen.h house.h
	$(CC) -c $(CFLAGS) act.other.c
act.social.o : act.social.c structs.h utils.h comm.h interpreter.h \
	handler.h db.h spells.h screen.h
	$(CC) -c $(CFLAGS) act.social.c
act.wizard.o : act.wizard.c structs.h utils.h comm.h interpreter.h \
	handler.h db.h spells.h house.h screen.h olc.h
	$(CC) -c $(CFLAGS) act.wizard.c
ban.o : ban.c structs.h utils.h comm.h interpreter.h handler.h db.h
	$(CC) -c $(CFLAGS) ban.c
boards.o : boards.c structs.h utils.h comm.h db.h boards.h interpreter.h \
	handler.h
	$(CC) -c $(CFLAGS) boards.c
class.o : class.c structs.h utils.h handler.h spells.h interpreter.h
	$(CC) -c $(CFLAGS) class.c
config.o : config.c structs.h
	$(CC) -c $(CFLAGS) config.c
constants.o : constants.c structs.h
	$(CC) -c $(CFLAGS) constants.c
db.o : db.c structs.h utils.h db.h comm.h handler.h spells.h \
	mail.h interpreter.h
	$(CC) -c $(CFLAGS) db.c
fight.o : fight.c structs.h utils.h comm.h handler.h interpreter.h db.h \
	spells.h screen.h
	$(CC) -c $(CFLAGS) fight.c
graph.o : graph.c structs.h utils.h comm.h interpreter.h handler.h db.h \
	spells.h
	$(CC) -c $(CFLAGS) graph.c
handler.o : handler.c structs.h utils.h comm.h db.h handler.h interpreter.h \
	spells.h auction.h
	$(CC) -c $(CFLAGS) handler.c 
house.o : house.c structs.h comm.h handler.h db.h interpreter.h utils.h	house.h
	$(CC) -c $(CFLAGS) house.c
interpreter.o : interpreter.c structs.h comm.h interpreter.h db.h utils.h \
	spells.h handler.h mail.h screen.h olc.h
	$(CC) -c $(CFLAGS) interpreter.c 
limits.o : limits.c structs.h utils.h spells.h comm.h db.h handler.h
	$(CC) -c $(CFLAGS) limits.c
magic.o : magic.c structs.h utils.h comm.h spells.h handler.h interpreter.h \
	db.h
	$(CC) -c $(CFLAGS) magic.c 
mail.o : mail.c mail.h structs.h utils.h comm.h db.h interpreter.h handler.h
	$(CC) -c $(CFLAGS) mail.c
mobact.o : mobact.c utils.h structs.h db.h comm.h interpreter.h handler.h \
	spells.h
	$(CC) -c $(CFLAGS) mobact.c
modify.o : modify.c structs.h utils.h interpreter.h handler.h db.h comm.h \
	spells.h mail.h boards.h olc.h
	$(CC) -c $(CFLAGS) modify.c
objsave.o : objsave.c structs.h comm.h handler.h db.h interpreter.h utils.h \
	spells.h
	$(CC) -c $(CFLAGS) objsave.c
random.o : random.c
	$(CC) -c $(CFLAGS) random.c
shop.o : shop.c structs.h comm.h handler.h db.h interpreter.h utils.h shop.h
	$(CC) -c $(CFLAGS) shop.c
spec_assign.o : spec_assign.c structs.h db.h interpreter.h utils.h
	$(CC) -c $(CFLAGS) spec_assign.c
spec_procs.o : spec_procs.c structs.h utils.h comm.h interpreter.h \
	handler.h db.h spells.h
	$(CC) -c $(CFLAGS) spec_procs.c
spell_parser.o : spell_parser.c structs.h utils.h comm.h db.h interpreter.h \
	spells.h handler.h
	$(CC) -c $(CFLAGS) spell_parser.c 
spells.o : spells.c structs.h utils.h comm.h db.h interpreter.h spells.h \
	handler.h
	$(CC) -c $(CFLAGS) spells.c 
utils.o : utils.c structs.h utils.h comm.h screen.h
	$(CC) -c $(CFLAGS) utils.c
weather.o : weather.c structs.h utils.h comm.h handler.h interpreter.h db.h
	$(CC) -c $(CFLAGS) weather.c
clan.o : clan.c structs.h db.h utils.h comm.h spells.h interpreter.h handler.h \
	boards.h
	$(CC) -c $(CFLAGS) clan.c
mobcmd.o : mobcmd.c structs.h db.h utils.h handler.h interpreter.h comm.h \
	spells.h
	$(CC) -c $(CFLAGS) mobcmd.c
mobprog.o : mobprog.c structs.h utils.h interpreter.h handler.h db.h
	$(CC) -c $(CFLAGS) mobprog.c
race.o : race.c structs.h db.h utils.h comm.h spells.h interpreter.h handler.h
	$(CC) -c $(CFLAGS) race.c
screen.o : screen.c structs.h utils.h comm.h interpreter.h handler.h db.h \
	spells.h screen.h
	$(CC) -c $(CFLAGS) screen.c
textsave.o : textsave.c structs.h comm.h handler.h db.h interpreter.h utils.h \
	spells.h
	$(CC) -c $(CFLAGS) textsave.c
ship.o : ship.c structs.h utils.h comm.h interpreter.h handler.h db.h
	$(CC) -c $(CFLAGS) ship.c
shop2.o : shop2.c structs.h comm.h handler.h db.h interpreter.h utils.h
	$(CC) -c $(CFLAGS) shop2.c
olc.o : olc.c structs.h utils.h comm.h interpreter.h handler.h \
	db.h olc.h screen.h
	$(CC) -c $(CFLAGS) olc.c
redit.o : redit.c structs.h utils.h comm.h interpreter.h boards.h db.h olc.h
	$(CC) -c $(CFLAGS) redit.c
oedit.o : oedit.c structs.h utils.h comm.h boards.h spells.h \
	db.h olc.h shop.h
	$(CC) -c $(CFLAGS) oedit.c
zedit.o : zedit.c structs.h utils.h comm.h db.h olc.h
	$(CC) -c $(CFLAGS) zedit.c
medit.o : medit.c structs.h utils.h comm.h spells.h shop.h \
	db.h olc.h
	$(CC) -c $(CFLAGS) medit.c
sedit.o : sedit.c structs.h utils.h comm.h db.h olc.h shop.h
	$(CC) -c $(CFLAGS) sedit.c
auction.o : auction.c structs.h comm.h handler.h db.h spells.h interpreter.h \
	utils.h screen.h auction.h
	$(CC) -c $(CFLAGS) auction.c
ascii_pfiles.o : ascii_pfiles.c structs.h comm.h handler.h db.h utils.h
	$(CC) -c $(CFLAGS) ascii_pfiles.c
creation.o : creation.c structs.h comm.h db.h interpreter.h utils.h \
	spells.h
	$(CC) -c $(CFLAGS) creation.c
pawnshop.o : pawnshop.c structs.h comm.h db.h interpreter.h utils.h \
	handler.h
	$(CC) -c $(CFLAGS) pawnshop.c
pets.o : pets.c structs.h comm.h db.h interpreter.h utils.h handler.h
	$(CC) -c $(CFLAGS) pets.c
timed.o : timed.c structs.h db.h handler.h interpreter.h utils.h comm.h
	$(CC) -c $(CFLAGS) timed.c
dshop.o : dshop.c structs.h utils.h db.h interpreter.h comm.h handler.h
	$(CC) -c $(CFLAGS) dshop.c
objprog.o : objprog.c structs.h utils.h db.h interpreter.h comm.h screen.h \
	spells.h
	$(CC) -c $(CFLAGS) objprog.c
foldarea.o : foldarea.c structs.h utils.h db.h interpreter.h comm.h screen.h \
	spells.h
	$(CC) -c $(CFLAGS) foldarea.c
quest.o : quest.c structs.h utils.h interpreter.h comm.h
	$(CC) -c $(CFLAGS) quest.c

../bin/circle :
	$(CC) -o ../bin/circle $(PROFILE) $(OBJFILES) $(LIBS)

