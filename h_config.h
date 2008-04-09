/*  h_config.h
 *
 *  This file specifies the hardware configuration of the pulse 
 *  programmer hardware and for the oscilloscope
 *
 * Part of Xnmr software project
 *
 * UBC Physics
 * April, 2000
 * 
 * written by: Scott Nelson, Carl Michal
 */

#ifndef H_CONFIG_H
#define H_CONFIG_H


/*
 *  Pulse Hardware information
 */


#define NUM_DEVICES 45
#define MAX_EVENTS 32768
#define NUM_CHIPS 21
#define CLOCK_SPEED 20000000
#define DEFAULT_RCVR_CLK 60000000.
#define MAX_CLOCK_CYCLES 4294967295UL
/*
 * This is the configuration format which must be followed
 * (For use with sscanf)
 */

#define H_CONFIG_FORMAT "#define %s %u // %u %u %u %u %lg\n"
#define PARSE_START "#define H_CONFIG_PARSE\n"
#define PARSE_END "#undef H_CONFIG_PARSE\n"


/*
 * This next line signals a parsing algorithm to begin searching this file.
 * Do not add any comments where CONFIG_PARSE is defined except to specify
 * device start bit, etc. 
 *
 *
 * Format for parsing is as follows
 *
 * #define <symbol> <device number> // <start bit> <num bits> <latch> <default> <max duration>
 *
 */

// the synthesizer device names are used internally in libxnmr (pulse.c) - freq set routines.

// the amplitude and phase lines as well as RF and RF_BNLK are used internally in there as well.

// the TIMER, XTRN_TRIG and PP_OVER are all hardwired inside, and used in pulse.c



#define H_CONFIG_PARSE

#define TIMER            0  //  0   32  0  0  0
#define XTRN_TRIG        1  //  38   1  0  0  0
#define PP_OVER          2  //  39   1  0  0  0
#define BNC_0            3  //  32   1  0  0  0
#define BNC_1            4  //  33   1  0  0  0
#define BNC_2            5  //  34   1  0  0  0
#define BNC_3            6  //  35   1  0  0  0
#define BNC_4            7  //  36   1  0  0  0
#define BNC_5            8  //  37   1  0  0  0
#define SYNTH1_LATCH     9  //  40   3  0  0  0
#define SYNTH1_1         10 //  43   4  0  0  0
#define SYNTH1_2         11 //  47   4  0  0  0
#define AD_SYNC          12 //  51   1  0  0  0
#define RCVR_GATE        13 //  52   1  0  0  0
#define AD_STAQ          14 //  53   1  0  0  0
#define SPARES2B         15 //  54   2  0  0  0
#define SYNTH2_LATCH     16 //  56   3  0  0  0
#define SYNTH2_1         17 //  59   4  0  0  0
#define SYNTH2_2         18 //  63   4  0  0  0
#define DUCK             19 //  67   4  0  0  0
#define SPARES3          20 //  71   1  0  0  0
#define RFA              21 // 72   1   0  0  0
#define CLKA             22 // 73   1   0  0  0
#define _AMPA            23 // 74   10  1  512  0
#define IA               24 // 84   10  1  512  0
#define QA               25 // 94   10  1  512  0
#define RFB              26 // 104  1   0  0  0
#define CLKB             27 // 105  1   0  0  0
#define _AMPB            28 // 106  10  1  512  0
#define IB               29 // 116  10  1  512  0
#define QB               30 // 126  10  1  512  0
#define RFC              31 // 136   1  0  0  0
#define CLKC             32 // 137   1  0  0  0
#define _AMPC            33 // 138   10 1  512  0
#define IC               34 // 148   10 1  512  0
#define QC               35 // 158   10 1  512  0
#define AMPC             36 // -1    0  0  0  0        
#define PHASEC           37 // -1     0  0  0  0
#define AMPB             38 // -1     0  0  0  0
#define PHASEB           39 // -1     0  0  0  0
#define AMPA             40 // -1     0  0  0  0
#define PHASEA           41 // -1     0  0  0  0
#define GRADX            42 // -1     0  0  0  0
#define GRADY            43 // -1     0  0  0  0
#define GRADZ            44 // -1     0  0  0  0


#undef H_CONFIG_PARSE




// the I,Q, and _AMP lines must latch! - so if we only set one of them, the others
// don't get reset unexpectedly?
// PHASE_ Should latch too!

// these are bogus ones that pulse.c will translate for us
// according to users toggle button channel assignment
// don't change these numbers!
// these devices get translated to RFA, AMPA, IA, QA etc.
// write_device wrap recognizes those and does the right thing with them

// everything above starting with RFA is hardcoded somehow in pulse.c.  The RFA, CLKA, _AMPA, IA, QA point to the real 
// devices are generally aren't used in pulse programs.  AMPA, PHASEA and GRADX are pseudo devices that
// write_device_wrap recognizes and deals with.
// the devices below get mapped at pulse-program run-time to the appropriate pseudo-devices or real devices above.


#define RF_OFFSET 200
#define RF1      200
#define PHASE1   201
#define AMP1     202
#define RF1_BLNK 203

#define RF2      204
#define PHASE2   205
#define AMP2     206
#define RF2_BLNK 207
#define GRAD_ON  208


/*
 *   Device Aliases
 */

#define ACQ_TRIG AD_STAQ

#define SCOPE_TRIG BNC_5

#define RFA_BLNK BNC_2
#define RFB_BLNK BNC_2
#define RFC_BLNK BNC_3


#ifdef CHANNEL1_A 
#warning  CHANNEL defines are obsolete
#endif
#ifdef CHANNEL1_B
#warning  CHANNEL defines are obsolete
#endif
#ifdef CHANNEL1_C 
#warning  CHANNEL defines are obsolete
#endif
#ifdef CHANNEL2_A 
#warning  CHANNEL defines are obsolete
#endif
#ifdef CHANNEL2_B
#warning  CHANNEL defines are obsolete
#endif
#ifdef CHANNEL2_C 
#warning  CHANNEL defines are obsolete
#endif




#endif








//#define _AMPA            23 // 74   10  1  512  0
//#define IA               24 // 84   10  1  512  0
//#define QA               25 // 94   10  1  512  0
//#define RFB              26 // 104  1   0  0  0
//#define CLKB             27 // 105  1   0  0  0
//#define _AMPB            28 // 106  10  1  512  0
//#define IB               29 // 116  10  1  512  0
//#define QB               30 // 126  10  1  512  0
//#define RFC              31 // 136   1  0  0  0
//#define CLKC             32 // 137   1  0  0  0
//#define _AMPC            33 // 138   10 1  512  0
//#define IC               34 // 148   10 1  512  0
//#define QC               35 // 158   10 1  512  0
