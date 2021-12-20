TARGETS=change_hat child open
PROFILES=change_hat.profile  child.profile  open.profile  sh.profile
LIB:=apparmor
LIBS=-l$(LIB)

all: targets profiles

targets: $(TARGETS)

profiles:
	for i in $(PROFILES) ;\
	do \
		sed "s~BASE~$$PWD~" $$i.pre | sed "s/AA/${LIB}/" > $$i ;\
	done	

change_hat: change_hat.c
	cc -Wall -o $@ $< $(LIBS)
child: child.c
	cc -Wall -o $@ $< $(LIBS)
open: open.c
	cc -Wall -o $@ $< $(LIBS)

clean:
	rm -f $(TARGETS) $(PROFILES)
