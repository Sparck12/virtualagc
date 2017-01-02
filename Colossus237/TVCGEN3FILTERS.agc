### FILE="Main.annotation"
## Copyright:   Public domain.
## Filename:	TVCGEN3FILTERS.agc
## Purpose:     Part of the source code for Colossus build 237.
##              This is for the Command Module's (CM) Apollo Guidance
##              Computer (AGC), for Apollo 8.
## Assembler:   yaYUL
## Contact:     Jim Lawton <jim DOT lawton AT gmail DOT com>
## Website:     www.ibiblio.org/apollo/index.html
## Page Scans:  www.ibiblio.org/apollo/ScansForConversion/Colossus237/
## Mod history: 2011-03-12 JL	Adapted from corresponding Colossus 249 file.
##		2017-01-01 RSB	Proofed comment text using octopus/ProoferComments,
##				and fixed errors found.

## Page 958
# PROGRAM NAME.... GEN3DAP FILTERS, CONSISTING OF NP0NODE, NP1NODE, NY0NODE, NY1NODE, ETC.
# LOG SECTION.... GEN3DAP FILTERS   SUBROUTINE....DAPCSM
# MOD BY ENGEL                      20 OCT, 1967

# FUNCTIONAL DESCRIPTION....

#      THE GEN3DAP FILTER PACKAGE IS DESIGNED TO PROVIDE FLEXIBLE, LAST-MINUTE CHANGEABLE DIGITAL AUTOPILOT
# FILTERS FOR LEM-OFF FLIGHT.  GROUNDRULES FOR THE DESIGN AND USE OF THE PACKAGE ARE AS FOLLOWS.....

#      1. FILTER COEFFICIENTS AND GAINS IN ERASABLE MEMORY
#      2. UP TO THIRD-ORDER NUMERATOR OR DENOMINATOR
#      3. OPERATIONAL FIT WITHIN THE STRUCTURE OF THE REGULAR LEM-ON DAP CODING
#      4. DENOMINATOR POLES INSIDE THE Z-PLANE UNIT CIRCLE
#      5. NUMERATOR ZEROS INSIDE THE Z-PLANE DOUBLE-UNIT CIRCLE
#      6. HIGH FREQUENCY (BODE) GAIN LESS THAN 8ASCREVS, OR 8.6380088 DEG/DEG

# THE FILTERS ARE SHOWN IN THE FOLLOWING DIAGRAMS.....


# PITCH GEN3DAP FILTER..

#                                                                  KPGEN3
#                                                                 ********
#                    *****************************************           *
#                    *                                       *           *
#                    *              -1        -2        -3   *           *
#       EP = ERRBTMP *    AP0 + AP1 Z   + AP2 Z   + AP3 Z     *   NP0          NPD = CMDTMP  **
#      ***************  -----------------------------------  **********( X )*********************
#                    *              -1        -2        -3   *                               **
#                    *    1  + BP1 Z   + BP2 Z   + BP3 Z     *
#                    *                                       *
#                    *****************************************


# YAW GEN3DAP FILTER..

#                                                                KYGEN3
#                                                               ********
#                   *****************************************           *
#                   *                                       *           *
#                   *              -1        -2        -3   *           *
#      EY = ERRBTMP *    AY0 + AY1 Z   + AY2 Z   + AY3 Z     *   NY0          NYP = CMDTMP  **
#     ***************  -----------------------------------  **********( X )*********************
#                   *              -1        -2        -3   *                               **
#                   *    1  + BY1 Z   + BY2 Z   + BY3 Z     *
#                   *                                       *
#                   *****************************************

## Page 959
# THE IMPLEMENTING EQUATIONS FOR THESE FILTERS ARE AS FOLLOWS.....

#      PITCH GEN3DAP....                                 YAW GEN3DAP....
#          NPD = (B+4) KPGEN3 NP0                            NYD = (B+4) KYGEN3 NY0
#          NP0 = AP0 EP	          +4(Z-1) NP1                NY0 = AY0 EY           +4(Z-1) NY1
#          NY1 = AP1 EP - BP1 NP0 + (Z-1) NP2                NY1 = AY1 EY - BY1 NY0 + (Z-1) NY2
#          NF2 = AP2 EP - BP2 NP0 + (Z-1) NP3                NY2 = AY2 EY - BY2 NY0 + (Z-1) NY3
#          NF3 = AP3 EP - BP3 NP0                            NY3 = AY3 EY - BY3 NY0


#      FILTER INPUTS EP AND EY ARE PICKED UP FROM REGULAR LEM-ON CODING AT ERRBTMP  (UPPER WORD ONLY),  THUS ARE
# SINGLE PRECISION QUANTITIES SCALED AT B-1 REVS. FILTER OUTPUTS NPD AND NYD ARE LEFT IN DOUBLE PRECISION AT
# CMDTMP, SCALED AT 1 ASCREV, READY FOR OUTPUT PROCESSING VIA REGULAR LEM-ON CODING AT ..P,YOFFSET..
# FOLLOWING OUTPUT PROCESSING, RETURN TO THE GEN3DAP FILTERS IS MADE FOR CALCULATION OF THE REMAINING NODES
# NP1 TO NP3, OR NY1 TO NY3.  GEN3DAP FILTERS THEN RETURN TO THE LEM-ON CODING AT  ..DELBARP,Y..   FOR RESPECTIVE
# OFFSET-TRACKER-FILTER COMPUTATIONS AND COPYCYCLES. NOTE THE EQUIVALENCES...NP1TMP=J5TMP, NP1=J5,
# NP2TMP=NSUMTMP, NP2=PNSUM, NP3TMP=DSUMTMP, NP3=PDSUM, WITH CORRESPONDING RELATIONS FOR YAW.  THUS THE COPY-
# CYCLE  PCOPY, FROM THE GEN3DAP STANDPOINT, IS EFFECTIVE FROM PMISC-3 TO ITS END AT TC Q.  YCOPY FROM YMISC-3.


# SCALING OF THE FILTER NODES, COEFFICIENTS, AND GAINS WITHIN THE AGC IS AS FOLLOWS.....

#          QUANTITY   QUANTITY      PHYS.UNITS     MAX.VALUE      SCALE AT (FOR)

#          EP         EY            REVS              1/8           B-1 REV           (CDU SCALING)
	
#          NP0        NY0           REVS             (B+1)          B+1 REV
#          NP1        NY1           REVS             (B+3)          B+3 REV
#          NP2        NY2           REVS             (B+3)          B+3 REV
#          NP3        NY3           REVS             (B+3)          B+3 REV

#          NPD        NYD         ASC REVS            (1)           1 ASCREV          (ACTUATOR CDU SCALING)
	
#          KPGEN3     KYGEN3     ASCREV/REV           (8)         B+3 ASCREV/REV

#          AP0        AY0         DIMLESS.             1              B+2
#          AP1        AY1         DIMLESS.             6              B+4
#          AP2        AY2         DIMLESS.             12             B+4
#          AP3        AY3         DIMLESS.             8              B+4

#          BP1        BY1         DIMLESS.             3              B+2
#          BP2        BY2         DIMLESS.             3              B+2
#          BP3        BY3         DIMLESS.             1              B+2

# FILTER COEFFICIENTS, GAINS, AND NODES ARE HELD IN DOUBLE PRECISION (ERASABLE) TO PERMIT CONSERVATIVE
# SCALING AND YET OFFSET TRUNCATION LOSSES.  THIS APPEARS NECESSARY IF FILTER FLEXIBILITY IS TO BE MAINTAINED.
# COMPUTATION TIME IS NOT CRITICAL.

## Page 960
# CALLING SEQUENCE....

#     *TC POSTJUMP....
#      CADR NP0NODE, NP1, NY0, NY1.  SPECIFICALLY, FROM PITCHDAP OR YAWDAP
#      (TVCDAP),AT P1FILJMP, P2FILJMP, Y1FILJMP, Y2FILJMP

# NORMAL EXIT MODE....

#     *TC POSTJUMP....
#      CADR  (POFFSET, DELBARP), (YOFFSET, DELBARY).     IE, RETURNS TO
#      PITCHDAP OR YAWDAP AT APPROPRIATE ENTRY POINT

# ALARM OR ABORT EXIT MODES....NONE

# SUBROUTINES CALLED.... NONE

# ERASABLE INITIALIZATION REQUIRED....

#     *AP0(SP),AP1(DP),...AP3(DP), (PITCH AND YAW) NUMERATOR COEFFICIENTS
#      (PAD LOADS)
#     *BP1(DP),...BP3(DP), (PITCH AND YAW) DENOMINATOR COEFFICIENTS
#      (PAD LOADS)
#     *KPGEN3 (S40.15 OF R03)

# OUTPUT....

#     *CMDTMP (NPD, NYD) FOR OUTPUT PROCESSING BY PITCHDAP OR YAWDAP
#     *OTHER FILTER NODES

# DEBRIS....TVC TEMPORARIES, SHAREABLE WITH RCS/ENTRY IN EBANK6 ONLY


		BANK	21
		SETLOC	DAPS4
		BANK
		EBANK=	EP
		COUNT*	$$/GEN3

## Page 961
# PITCH GEN3DAP FILTER.....

NP0NODE		EXTEND			# FORM NODE NP0....COLLECT  (PAST NP1)
		DCA	NP1		#      (COMES HERE FROM REG. DAP CODING)
		DDOUBL
		DDOUBL
		DXCH	NP0

AP0(EP)		CAE	EP		# SPXSP MULTIPLY FOR NUMERATOR COMPONENT
		EXTEND			#      EP = ERRBTMP,  SP, SC.AT B-1 REVS
		MP	AP0
		DAS	NP0		# COMPLETED NODE NP0, SC.AT B+1 REVS


NPDNODE		CAE	NP0		# FORM NODE NPD....SPXDP MULTIPLY BY GAIN
		EXTEND
		MP	KPGEN3
		DXCH	NPD
		CAE	NP0 +1
		EXTEND
		MP	KPGEN3
		ZL
		LXCH	A
		DAS	NPD		# SC.AT B+4 ASCREV SINCE KPGEN3 AT B+3

		DXCH	NPD		# FIX UP SCALING
		DDOUBL
		DDOUBL
		DDOUBL
		DDOUBL
		DXCH	NPD		# COMPLETED NODE NPD, SC.AT 1ASCREV


		TC	POSTJUMP	# TRANSFER BACK TO REGULAR DAP CODING FOR
		CADR	POFFSET		#      OUTPUT  (NPD = CMDTMP, DP)
NP1NODE		EXTEND			# FORM NODE NP1....COLLECT  (PAST NP2)
		DCA	NP2		#      (COMES HERE FROM REG. DAP CODING)
		DXCH	NP1TMP

BP1(NP0)	CS	NP0		# DPXDP MULTIPLY FOR DENOMINATOR COMPONENT
		EXTEND
		MP	BP1
		DAS	NP1TMP
		CS	NP0 +1
		EXTEND
		MP	BP1
		ADS	NP1TMP +1
		TS	L
		TCF	+2
		ADS	NP1TMP
## Page 962
		CS	NP0
		EXTEND
		MP	BP1 +1
		ADS	NP1TMP +1
		TS	L
		TCF	+2
		ADS	NP1TMP

AP1(EP)		CAE	EP		# DPXSP MULTIPLY FOR NUMERATOR COMPONENT
		EXTEND
		MP	AP1
		DAS	NP1TMP
		CAE	EP
		EXTEND
		MP	AP1 +1
		ADS	NP1TMP +1
		TS	L
		TCF	+2
		ADS	NP1TMP		# COMPLETED NODE NP1

NP2NODE		EXTEND			# FORM NODE NP2....COLLECT (PAST NP3)
		DCA	NP3
		DXCH	NP2TMP
BP2(NP0)	CS	NP0		# DPXDP MULTIPLY FOR DENOMINATOR COMPONENT
		EXTEND
		MP	BP2
		DAS	NP2TMP
		CS	NP0 +1
		EXTEND
		MP	BP2
		ADS	NP2TMP +1
		TS	L
		TCF	+2
		ADS	NP2TMP
		CS	NP0
		EXTEND
		MP	BP2 +1
		ADS	NP2TMP +1
		TS	L
		TCF	+2
		ADS	NP2TMP

AP2(EP)		CAE	EP		# DPXSP MULTIPLY FOR NUMERATOR COMPONENT
		EXTEND
		MP	AP2
		DAS	NP2TMP
		CAE	EP
		EXTEND
		MP	AP2 +1
		ADS	NP2TMP +1
## Page 963
		TS	L
		TCF	+2
		ADS	NP2TMP		# COMPLETED NODE NP2

NP3NODE		CS	NP0		# FORM NODE NP3....NO PAST NODES, DIRECT
		EXTEND			#      TO DPXDP MULTIPLY FOR DENOMINATOR
		MP	BP3		#      COMPONENT
		DXCH	NP3TMP
		CS	NP0 +1
		EXTEND
		MP	BP3
		ADS	NP3TMP +1
		TS	L
		TCF	+2
		ADS	NP3TMP
		CS	NP0
		EXTEND
		MP	BP3 +1
		ADS	NP3TMP +1
		TS	L
		TCF	+2
		ADS	NP3TMP

AP3(EP)		CAE	EP		# DPXSP MULTIPLY FOR NUMERATOR COMPONENT
		EXTEND
		MP	AP3
		DAS	NP3TMP
		CAE	EP
		EXTEND
		MP	AP3 +1
		ADS	NP3TMP +1
		TS	L
		TCF	+2
		ADS	NP3TMP		# COMPLETED NODE NP3, AND PITCH GEN3DAP
#					       FILTER COMPUTATIONS


		TC	POSTJUMP	# RETURN TO CSMDAP CODING FOR PITCH
		CADR	DELBARP		#      OFFSET-TRACKER-FILTER COMPUTATIONS,
#					       AND PITCH DAP COPYCYCLE.

## Page 964
# YAW GEN3DAP FILTER....

NY0NODE		EXTEND			# FORM NODE NY0....COLLECT (PAST NY1)
		DCA	NY1		#      (COMES HERE FROM REG. DAP CODING)
		DDOUBL
		DDOUBL
		DXCH	NY0

AY0(EY)		CAE	EY		# SPXSP MULTIPLY FOR NUMERATOR COMPONENT
		EXTEND			#      EY = ERRBTMP,  SP, SC.AT B-1 REVS
		MP	AY0
		DAS	NY0		# COMPLETED NODE NY0, SC.AT B+1 REVS


NYDNODE		CAE	NY0		# FORM NODE NYD....SPXDP MULTIPLY BY GAIN
		EXTEND
		MP	KYGEN3
		DXCH	NYD
		CAE	NY0 +1
		EXTEND
		MP	KYGEN3
		ZL
		LXCH	A
		DAS	NYD		# SC.AT B+4 ASCREV SINCE KYGEN3 AT B+1

		DXCH	NYD		# FIX UP SCALING
		DDOUBL
		DDOUBL
		DDOUBL
		DDOUBL
		DXCH	NYD		# COMPLETED NODE NYD, SC.AT 1ASCREV


		TC	POSTJUMP	# TRANSFER BACK TO REGULAR DAP CODING FOR
		CADR	YOFFSET		#      OUTPUT  (NYD = CMDTMP, DP)
NY1NODE		EXTEND			# FORM NODE NY1....COLLECT  (PAST NY2)
		DCA	NY2		#      (COMES HERE FROM REG. DAP CODING)
		DXCH	NY1TMP

BY1(NY0)	CS	NY0		# DPXDP MULTIPLY FOR DENOMINATOR COMPONENT
		EXTEND
		MP	BY1
		DAS	NY1TMP
		CS	NY0 +1
		EXTEND
		MP	BY1
		ADS	NY1TMP +1
		TS	L
		TCF	+2
		ADS	NY1TMP
## Page 965
		CS	NY0
		EXTEND
		MP	BY1 +1
		ADS	NY1TMP +1
		TS	L
		TCF	+2
		ADS	NY1TMP

AY1(EY)		CAE	EY		# DPXSP MULTIPLY FOR NUMERATOR COMPONENT
		EXTEND
		MP	AY1
		DAS	NY1TMP
		CAE	EY
		EXTEND
		MP	AY1 +1
		ADS	NY1TMP +1
		TS	L
		TCF	+2
		ADS	NY1TMP		# COMPLETED NODE NY1

NY2NODE		EXTEND			# DORM NODE NY2....COLLECT (PAST NY3)
		DCA	NY3
		DXCH	NY2TMP

BY2(NY0)	CS	NY0		# DPXDP MULTIPLY FOR DENOMINATOR COMPONENT
		EXTEND
		MP	BY2
		DAS	NY2TMP
		CS	NY0 +1
		EXTEND
		MP	BY2
		ADS	NY2TMP +1
		TS	L
		TCF	+2
		ADS	NY2TMP
		CS	NY0
		EXTEND
		MP	BY2 +1
		ADS	NY2TMP +1
		TS	L
		TCF	+2
		ADS	NY2TMP

AY2(EY)		CAE	EY		# DPXSP MULTIPLY FOR NUMERATOR COMPONENT
		EXTEND
		MP	AY2
		DAS	NY2TMP
		CAE	EY
		EXTEND
		MP	AY2 +1
## Page 966
		ADS	NY2TMP +1
		TS	L
		TCF	+2
		ADS	NY2TMP		# COMPLETED NODE NY2

NY3NODE		CS	NY0		# FORM NODE NY3....NO PAST NODES, DIRECT
		EXTEND			#      TO DPXDP MULTIPLY FOR DENOMINATOR
		MP	BY3		#      COMPONENT
		DXCH	NY3TMP
		CS	NY0 +1
		EXTEND
		MP	BY3
		ADS	NY3TMP +1
		TS	L
		TCF	+2
		ADS	NY3TMP
		CS	NY0
		EXTEND
		MP	BY3 +1
		ADS	NY3TMP +1
		TS	L
		TCF	+2
		ADS	NY3TMP

AY3(EY)		CAE	EY		# DPXSP MULTIPLY FOR NUMERATOR COMPONENT
		EXTEND
		MP	AY3
		DAS	NY3TMP
		CAE	EY
		EXTEND
		MP	AY3 +1
		ADS	NY3TMP +1
		TS	L
		TCF	+2
		ADS	NY3TMP		# COMPLETED NODE NY3, AND YAW GEN3DAP
#					       FILTER COMPUTATIONS


		TC	POSTJUMP	# RETURN TO CSMDAP CODING FOR YAW
		CADR	DELBARY		#      OFFSET-TRACKER-FILTER COMPUTATIONS,
#					       AND YAW DAP COPYCYCLE.
