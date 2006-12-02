# Makefile for Xnmr Project
#
# UBC Physics
#
# ffloat-store prevents reals from being stored in registers.


#CC = icc
CFLAGS = -g  -O2 -fPIC -march=pentium4  -Wall  `pkg-config --cflags gthread-2.0 gtk+-2.0` 
#O3 causes serious problems with rtai!

# for icc:
#CFLAGS =    -O3 -axP -ipo  `pkg-config --cflags gthread-2.0 gtk+-2.0` 

all: Xnmr acq libxnmr.so Xnmr_preproc

spin_bug: spin_bug.c
	cc -o spin_bug -g -Wall `pkg-config --cflags gtk+-2.0` spin_bug.c `pkg-config --libs gtk+-2.0`

chooser_bug.o: chooser_bug.c
	cc -c chooser_bug.c $(CFLAGS)

chooser_bug: chooser_bug.o
	cc -o chooser_bug chooser_bug.o `pkg-config --libs gthread-2.0 gtk+-2.0`

Xnmr_preproc: Xnmr_preproc.c
	$(CC) $(CFLAGS)  -o Xnmr_preproc Xnmr_preproc.c


#libxnmr.a: pulse.o param_utils.o four1.o
#	ar -rc libxnmr.a pulse.o param_utils.o four1.o

libxnmr.so: pulse.o param_utils.o four1.o
	$(CC) $(CFLAGS) -shared -o libxnmr.so pulse.o param_utils.o four1.o


dsp.o: dsp.h dsp.c shm_data.h adepp.c adepp.h param_utils.h
	$(CC)  $(CFLAGS) -c dsp.c
adepp.o: adepp.c adepp.h  
	$(CC) -O1 $(CFLAGS) -c adepp.c
ad9850.o: ad9850.c ad9850.h  
	$(CC) -O1 $(CFLAGS) -c ad9850.c

clean:
	rm -f *.o acq Xnmr core libxnmr.a libxnmr.so Xnmr_preproc

#  added -I/usr/realtime/include/ for rtai
acq.o: acq.c acq.h shm_data.h shm_prog.h h_config.h p_signals.h pulse_hardware.h param_utils.h dsp.h ad9850.h
	$(CC) -I/usr/realtime/include/ $(CFLAGS) -c acq.c

pulse_hardware.o: pulse_hardware.c pulse_hardware.h shm_prog.h h_config.h param_utils.h p_signals.h
	$(CC) -O1 $(CFLAGS) -c pulse_hardware.c

# added -L/usr/realtime/lib and -lpthread for rtai
acq: acq.o pulse_hardware.o libxnmr.so dsp.o adepp.o ad9850.o libxnmr.so
	$(CC) $(CFLAGS) -L/usr/realtime/lib -L. -o acq acq.o pulse_hardware.o dsp.o adepp.o ad9850.o `pkg-config --libs gtk+-2.0` -lm -lpthread  -lxnmr
	@echo ""
	@echo "Don't forget to make acq suid!!!"
	@echo ""

Xnmr: xnmr.o buff.o panel.o process_f.o param_f.o xnmr_ipc.o  spline.o\
 splint.o nrutil.o  libxnmr.so
	$(CC) $(CFLAGS)  -L. xnmr.o   buff.o  panel.o process_f.o param_f.o\
 xnmr_ipc.o  spline.o splint.o nrutil.o \
-o Xnmr `pkg-config --libs gthread-2.0 gtk+-2.0` -L -lm -lport -lf2c -lgfortran  -lxnmr -Xlinker -defsym -Xlinker MAIN__=main

# the -Xlinker -defsym -Xlinker MAIN__=main   passes: '-defsym MAIN__=main' to the linker, let us use 
# fortran and C together.  The -lportP has the nonlinear fitting routine, and lf2c is necessary for fortran

xnmr.o: xnmr.c xnmr.h panel.h buff.h param_f.h xnmr_ipc.h p_signals.h
	$(CC) $(CFLAGS) -c xnmr.c

buff.o: buff.c xnmr.h buff.h param_f.h panel.h param_utils.h xnmr_ipc.h p_signals.h
	$(CC) $(CFLAGS) -c buff.c

four1.o: four1.c
	$(CC) $(CFLAGS) -c four1.c
spline.o: spline.c
	$(CC) $(CFLAGS) -c spline.c
splint.o: splint.c
	$(CC) $(CFLAGS) -c splint.c
nrutil.o: nrutil.c
	$(CC) $(CFLAGS) -c nrutil.c

panel.o: panel.c panel.h process_f.h param_f.h xnmr_ipc.h shm_data.h buff.h p_signals.h xnmr.h
	$(CC) $(CFLAGS) -c panel.c

process_f.o: process_f.c process_f.h panel.h xnmr.h nr.h buff.h param_utils.h 
	$(CC) $(CFLAGS) -c process_f.c

param_f.o: param_f.c param_f.h xnmr_ipc.h shm_data.h panel.h xnmr_ipc.h param_utils.h
	$(CC) $(CFLAGS) -c param_f.c

xnmr_ipc.o: xnmr_ipc.c xnmr_ipc.h shm_data.h p_signals.h process_f.h panel.h buff.h xnmr.h h_config.h param_f.h
	$(CC) $(CFLAGS) -c xnmr_ipc.c

pulse.o: pulse.c pulse.h h_config.h shm_data.h shm_prog.h p_signals.h param_utils.h
	$(CC) -fPIC $(CFLAGS) -c pulse.c

param_utils.o: param_utils.h param_utils.c shm_data.h
	$(CC) -fPIC $(CFLAGS) -c param_utils.c

ipcclean: ipcclean.c shm_data.h shm_prog.h
	$(CC) $(CFLAGS) -o ipcclean ipcclean.c
install: 
