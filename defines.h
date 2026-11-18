//======================================================================================================================
//
//  defines.h - global defines
//
//  IBM 1620 Jr Project, Computer History Museum, 2017-2026
//
//  To recreate the experience (visual, auditory, tactile, visceral) of running historic software on a 1960s-era
//  computer.
//
//    Dave Babcock  - project lead, software, website
//   David Brock    - CHM sponsor
//   Steve Casner   - hardware, software
//     Joe Fredrick - hardware, firmware
//     Len Shustek  - CHM advisor
//     Dag Spicer   - CHM advisor
//   David Wise     - IBM 1620 expert
//
//======================================================================================================================

#ifndef DEFINES_H_
#define DEFINES_H_

// Simulator version

#define SIM_VERSION_MAJOR 1
#define SIM_VERSION_MINOR 1 

// Compile options

#define HARDWARE_TIMER  // use RPi hardware timer for simulator timing


// =================================  Basic Defines  =================================

// Hardware definitions

#define BASE_9635 0x40
#define BASE_9675 0x20
#define NUM_9635  12
#define NUM_9675  3
#define NUM_BITS  16
#define NUM_BYTES (NUM_BITS / 8)
#define NUM_PWMS  (NUM_9635 * NUM_BITS)
#define NUM_GPIOS (NUM_9675 * NUM_BITS)

#ifdef HARDWARE_TIMER
#define BCM2835_PAGE_SIZE      4096
#define BCM2835_SYSTEM_TIMER   0x3F003000
#define BCM2835_COUNTER_OFFSET 0x4
#endif

// powerState values

#define POWER_EMERGENCY_OFF -1
#define POWER_OFF            0
#define POWER_ON             1
#define POWER_READY          2
#define POWER_SHUTDOWN       3

// runState values

#define CHECK_STOP_STATE         -1
#define STOPPED_STATE             0
#define INSERT_STATE              1
#define LOAD_STATE                2
#define SINGLE_CYCLE_STATE        3
#define SINGLE_INSTRUCTION_STATE  4
#define RUNNING_STATE             5

// ioState values

#define ABORTED_IO_STATE -1
#define STOPPED_IO_STATE  0
#define RUNNING_IO_STATE  1

// I/O instruction values

#define NO_IO      0 
#define INSERT_IO  1
#define LOAD_IO    2
#define SPTY_IO    3
#define RCTY_IO    4
#define TBTY_IO    5
#define DNTY_IO    6
#define RNTY_IO    7
#define RATY_IO    8
#define WNTY_IO    9
#define WATY_IO   10
#define DNCD_IO   11
#define RNCD_IO   12 
#define RACD_IO   13 
#define WNCD_IO   14
#define WACD_IO   15

// fpMode values

#define FP_EXP_COMPARE          0
#define FP_SCAN_Q               1
#define FP_EXP_TRANSMIT         2
#define FP_SCAN_P               3
#define FP_RESULT_TRANSMIT      4
#define FP_SHIFT_RIGHT          5
#define FP_ZERO_FILL_R          6
#define FP_FRACTION_ADD         7
#define FP_EXP_MODIFY_ENTRY     8
#define FP_EXP_MODIFY           9
#define FP_EXP_MODIFY_EXIT     10
#define FP_SHIFT_R1FILL        11
#define FP_DIGIT_FORCE         12
#define FP_SCAN_ZERO           13
#define FP_SHIFT_LEFT          14
#define FP_ZERO_FILL_L         15
#define FP_EXP_ADD             16
#define FP_MULTIPLY            17
#define FP_PRODUCT_TRANSMIT    18
#define FP_FRACTION_COMPARE    19
#define FP_FALSE_TRANSMIT      20
#define FP_LOAD_DIVIDEND_ENTRY 21
#define FP_LOAD_DIVIDEND       22
#define FP_DIVIDE_ENTRY        23
#define FP_DIVIDE              24
#define FP_FSR_TRANSMIT        25
#define FP_FSR_ZERO_FILL       26
#define FP_FSL_SCAN            27
#define FP_FSL_TRANSMIT        28
#define FP_FSL_ZERO_FILL       29

// lightMode values

#define REALISTIC_LIGHTS 0
#define BRIGHT_LIGHTS    1

// testMode values

#define NO_TEST         0
#define LAMP_TEST       1
#define BRIGHTNESS_TEST 2
#define SNAKE_TEST      3
#define IBM_TEST        4

// marSelector values

#define MAR_MAR   -1
#define OR1_MAR    0
#define OR2_MAR    1
#define OR3_MAR    2
#define PR1_MAR    3
#define PR2_MAR    4
#define PR3_MAR    5
#define OR4_MAR    6
#define OR5_MAR    7
#define CR1_MAR    8
#define IR3_MAR    9
#define IR1_MAR   10
#define IR2_MAR   11

// Opcodes

#define FADD_OPCODE  1
#define FSUB_OPCODE  2
#define FMUL_OPCODE  3
#define FSL_OPCODE   5
#define TFL_OPCODE   6
#define BTFL_OPCODE  7
#define FSR_OPCODE   8
#define FDIV_OPCODE  9
#define AM_OPCODE   11
#define SM_OPCODE   12
#define MM_OPCODE   13
#define CM_OPCODE   14
#define TDM_OPCODE  15
#define TFM_OPCODE  16
#define BTM_OPCODE  17
#define LDM_OPCODE  18
#define DM_OPCODE   19
#define A_OPCODE    21
#define S_OPCODE    22
#define M_OPCODE    23
#define C_OPCODE    24
#define TD_OPCODE   25
#define TF_OPCODE   26
#define BT_OPCODE   27
#define LD_OPCODE   28
#define D_OPCODE    29
#define TR_OPCODE   31
#define SF_OPCODE   32
#define CF_OPCODE   33
#define K_OPCODE    34
#define DN_OPCODE   35
#define RN_OPCODE   36
#define RA_OPCODE   37
#define WN_OPCODE   38
#define WA_OPCODE   39
#define NOP_OPCODE  41
#define BB_OPCODE   42
#define BD_OPCODE   43
#define BNF_OPCODE  44
#define BNR_OPCODE  45
#define BI_OPCODE   46
#define BNI_OPCODE  47
#define H_OPCODE    48
#define B_OPCODE    49
#define BNG_OPCODE  55
#define MF_OPCODE   71
#define TNS_OPCODE  72
#define TNF_OPCODE  73

// Branch indicators

#define PROGRAM_SWITCH_1_INDICATOR     1
#define PROGRAM_SWITCH_2_INDICATOR     2
#define PROGRAM_SWITCH_3_INDICATOR     3
#define PROGRAM_SWITCH_4_INDICATOR     4
#define READ_CHECK_INDICATOR           6
#define WRITE_CHECK_INDICATOR          7
#define MAR_CHECK_INDICATOR            8
#define LAST_CARD_INDICATOR            9
#define HIGH_POSITIVE_INDICATOR       11
#define EQUAL_ZERO_INDICATOR          12
#define HP_OR_EQ_INDICATOR            13
#define ARITHMETIC_OVERFLOW_INDICATOR 14
#define EXPONENT_INDICATOR            15
#define MBR_EVEN_CHECK_INDICATOR      16
#define MBR_ODD_CHECK_INDICATOR       17
#define ANY_DATA_INDICATOR            19

// Miscellaneous definitions

#define FALSE 0
#define TRUE  1

#define OFF 0
#define ON  1

#define EVEN 0
#define ODD  1
#define ANY  2

#define MEMORY_SIZE 60000

#define CHECK_MASK      0x20
#define FLAG_MASK       0x10
#define FLAG_DIGIT_MASK 0x1F
#define DIGIT_MASK      0x0F

#define RECORD_MARK       0x0A
#define RECORD_GROUP_MARK 0x0B
#define NUMERIC_BLANK     0x0C
#define INVALID_13        0x0D
#define INVALID_14        0x0E
#define GROUP_MARK        0x0F

#ifdef HARDWARE_TIMER
#define CYCLE_TIME        20          // machine cycle time (20usec)
#else
#define CYCLE_COUNT       10          // number of machine cycles between simulator time syncs
#define CYCLE_INTERVAL    20000LL     // simulator time sync interval (20usec)
#endif
#define DISPLAY_INTERVAL  10000000LL  // display time sync interval (10msec)
#define SCR_DISPLAY_DELAY 1000000LL   // scr display delay (1msec = (2-5 degrees)/360degrees * 100msec)

#define RISE_DELTA 3   // how quickly lights can change brightness up
#define FALL_DELTA 10  // how quickly lights can change brightness down

#define CYCLES_PER_SEC    50000                  // number of machine cycles per second
#define POWER_ON_WAIT     (20 * CYCLES_PER_SEC)  // time delay between power on and power ready (20sec)
#define TY_IO_CYCLE_WAIT  5000                   // number of cpu cycles per typewriter character
#define CD_IO_CYCLE_WAIT  1                      // number of cpu cycles per punched card character

#define DB  10       // debounce interval (100msec)
#define INF INT_MAX  // max debounce interval

#define NUM_NUMERIC_CHARS    32
#define NUM_ALPHAMERIC_CHARS 256
#define NUM_CMEM_FILES       10
#define NUM_CYCLES           57
#define NUM_TRIGGERS         77
#define NUM_TRIGGER_LIGHTS   54
#define NUM_MARS             12
#define NUM_OPCYCLES         100
#define NUM_PARITY           32

#define MAX_INPUT_LINE  120
#define DIGITS_PER_LINE 20

// Miscellaneous types

typedef unsigned char bool;
typedef unsigned char memDigit;

// NULL pointer type

#if !defined(NULL)
#  define NULL (void*)0
#endif


// =================================  Console Typewriter  =================================

// Command characters

#define TY_COMMAND_MODE_0  'a'
#define TY_COMMAND_MODE_1  'b'
#define TY_COMMAND_MODE_2  'c'
#define TY_COMMAND_MODE_3  'd'
#define TY_COMMAND_PING    'g'
#define TY_COMMAND_ACK     'h'
#define TY_COMMAND_SLASH   's'
#define TY_COMMAND_UNSLASH 'u'
#define TY_COMMAND_SETUP   'y'
#define TY_COMMAND_RESET   'z'

// Protocol characters

#define PROTOCOL_SPACE              ' '
#define PROTOCOL_TAB                ':'
#define PROTOCOL_RETURN             ';'
#define PROTOCOL_BACKSPACE          '_'
#define PROTOCOL_LEFT_ARROW         '<'
#define PROTOCOL_RIGHT_ARROW        '>'
#define PROTOCOL_LEFT_MARGIN        '['
#define PROTOCOL_RIGHT_MARGIN       ']'
#define PROTOCOL_CLEAR_TAB          '`'
#define PROTOCOL_CLEAR_ALL_TABS     '\\'
#define PROTOCOL_SET_TAB            '\''
#define PROTOCOL_RELEASE_START      '#'

#define PROTOCOL_FLAG_0             'i'
#define PROTOCOL_FLAG_1             'j'
#define PROTOCOL_FLAG_2             'k'
#define PROTOCOL_FLAG_3             'l'
#define PROTOCOL_FLAG_4             'm'
#define PROTOCOL_FLAG_5             'n'
#define PROTOCOL_FLAG_6             'o'
#define PROTOCOL_FLAG_7             'p'
#define PROTOCOL_FLAG_8             'q'
#define PROTOCOL_FLAG_9             'r'
#define PROTOCOL_NUMERIC_BLANK      '@'
#define PROTOCOL_FLAG_NUMERIC_BLANK '~'
#define PROTOCOL_RECORD_MARK        '|'
#define PROTOCOL_FLAG_RECORD_MARK   '!'
#define PROTOCOL_GROUP_MARK         '}'
#define PROTOCOL_FLAG_GROUP_MARK    '"'
#define PROTOCOL_INVALID_CHARACTER  '?'

#define PROTOCOL_PAUSE              '%'
#define PROTOCOL_RESUME             '&'

// Miscellaneous definitions

#define NUM_IO_CHARS       96
#define NUM_CARD_POS       80
#define NUM_TYPEWRITER_POS 80

#define TIMING_CARRIAGE_RETURN 35000

#define POSITION_NONE           0
#define POSITION_LEFT           1
#define POSITION_RIGHT          2
#define POSITION_TAB            3
#define POSITION_RETURN         4


// =================================  Punched Card  =================================

// Command characters

#define CD_COMMAND_MANUAL           's'
#define CD_COMMAND_NOT_MANUAL       't'
#define CD_COMMAND_READ_NUMERIC     'u'
#define CD_COMMAND_READ_ALPHAMERIC  'v'
#define CD_COMMAND_WRITE_NUMERIC    'w'
#define CD_COMMAND_WRITE_ALPHAMERIC 'x'
#define CD_COMMAND_DUMP_NUMERIC     'y'
#define CD_COMMAND_RESET            'z'
#define CD_COMMAND_POWER_OFF        '>'
#define CD_COMMAND_POWER_ON         '<'
#define CD_COMMAND_REQUEST_STATUS   '#'
#define CD_COMMAND_SHUTDOWN         '_'

// Status characters

#define CD_STATUS_READER_READY     'a'
#define CD_STATUS_READER_NOT_READY 'b'
#define CD_STATUS_READER_BUSY      '\\'
#define CD_STATUS_READER_NOT_BUSY  '['
#define CD_STATUS_READER_CHECK     ';'
#define CD_STATUS_READER_LAST_CARD 'e'
#define CD_STATUS_READER_LOAD      'f'
#define CD_STATUS_PUNCH_READY      'c'
#define CD_STATUS_PUNCH_NOT_READY  'd'
#define CD_STATUS_PUNCH_BUSY       '^'
#define CD_STATUS_PUNCH_NOT_BUSY   '{'
#define CD_STATUS_PUNCH_CHECK      ';'

// Miscellaneous definitions

#define IBM1620_PORT_NUMBER 1620

// Protocol actions

#define ACTION_IGNORE            0
#define ACTION_ACCEPT            1
#define ACTION_READER_READY      2
#define ACTION_READER_NOT_READY  3
#define ACTION_READER_BUSY       4
#define ACTION_READER_NOT_BUSY   5
#define ACTION_READER_CHECK      6
#define ACTION_READER_LAST_CARD  7
#define ACTION_READER_LOAD       8
#define ACTION_PUNCH_READY       9
#define ACTION_PUNCH_NOT_READY  10
#define ACTION_PUNCH_BUSY       11
#define ACTION_PUNCH_NOT_BUSY   12
#define ACTION_PUNCH_CHECK      13
#define ACTION_REQUEST_STATUS   14
#define ACTION_ERROR            15



// =================================  Machine Triggers  =================================

// INSTRUCTION AND EXECUTE CYCLES

#define RUN_TRIGGER         0
#define START_CTRL_TRIGGER  1
#define INSERT_2_TRIGGER    2
#define DISP_ADDR_3_TRIGGER 3
#define SAVE_2_TRIGGER      4

// CONTROL GATES

#define DIV_1_CYCLE_TRIGGER   5
#define DIV_ADD_TRIGGER       6
#define DIV_LAST_CYC_TRIGGER  7
#define OFLO_UNFLO_TRIGGER    8
#define OFLO_CORR_TRIGGER     9

#define RE_COMP_TRIGGER      10
#define CARRY_IN_TRIGGER     11
#define CARRY_OUT_TRIGGER    12
#define CYCLE_CTRL_TRIGGER   13
#define GR_50_TRIGGER        14

#define REC_CTRL_TRIGGER     15
#define TC_1_TRIGGER         16
#define IA_TRIGGER           17
#define DIGIT_TRIGGER        18

#define DIV_SIGN_TRIGGER     19
#define UNFLO_ZERO_TRIGGER   20
#define FP_EXIT_TRIGGER      21

#define FIRST_CYCLE_TRIGGER  22
#define FIELD_MK_1_TRIGGER   23
#define FIELD_MK_2_TRIGGER   24
#define REC_MARK_TRIGGER     25
#define BRCH_TRIGGER         26

#define A_ADV_TRIGGER        27
#define DECR_TRIGGER         28
#define INCR_TRIGGER         29
#define PLUS_2_TRIGGER       30
#define BYPASS_TRIGGER       31

#define ODD_TRIGGER          32
#define I_TIME_TRIGGER       33
#define E_TIME_TRIGGER       34
#define HP_TRIGGER           35
#define EZ_TRIGGER           36

// INPUT-OUTPUT

#define MEM_OFLOW_TRIGGER  37
#define STOP_TRIGGER       38
#define LAST_CARD_TRIGGER  39

#define DISCN_GATE_TRIGGER 40
#define RSP_GATE_TRIGGER   41
#define RW_CALL_TRIGGER    42
#define IO_EXIT_TRIGGER    43
#define IO_HOLD_TRIGGER    44

#define PCH_FEED_TRIGGER   45
#define IO_FLAG_TRIGGER    46
#define IO_SYNC_TRIGGER    47
#define RDR_FEED_TRIGGER   48

// PARITY

#define MAR_CHK_TRIGGER 49

// I/O

#define RD_CHK_TRIGGER 50
#define WR_CHK_TRIGGER 51

// O'FLOW

#define EXP_CHK_TRIGGER   52
#define ARITH_CHK_TRIGGER 53

// Internal Triggers (not displayed)

#define I_CYCLE_ENTRY_TRIGGER   54
#define E_CYCLE_ENTRY_TRIGGER   55
#define SAVE_CTRL_TRIGGER       56
#define BT_BTM_BTFL_TRIGGER     57
#define INSERT_TRIGGER          58
#define MULTIPLY_UNITS_TRIGGER  59
#define NEXT_IA_TRIGGER         60
#define P_Q_TRIGGER             61
#define SCAN_MINUS_TRIGGER      62
#define SCAN_Q_EXIT_TRIGGER     63
#define D_GT_99_TRIGGER         64
#define CNTR_COMP_TRIGGER       65
#define HI_ORDER_ZERO_TRIGGER   66
#define NORM_SHIFT_R_TRIGGER    67
#define SIG_DIGIT_TRIGGER       68
#define FORCE_FLAG_1_TRIGGER    69
#define HP_AUX_TRIGGER          70
#define L1_MODULO_TRIGGER       71
#define L2_MODULO_TRIGGER       72
#define SCAN_Q_LATCH            73
#define EXP_MODIFY_REQD_TRIGGER 74
#define MULT_CORRECT_TRIGGER    75
#define PE_ZERO_TRIGGER         76


// =================================  Hardware Offsets  =================================

// INSTRUCTION AND EXECUTE CYCLE (pwm)

#define I1_LIGHT          168
#define I2_LIGHT          149
#define I3_LIGHT          146
#define I4_LIGHT          140
#define I5_LIGHT          158

#define I6_LIGHT          183
#define I7_LIGHT          145
#define I8_LIGHT          153
#define T1_LIGHT          143
#define T2_LIGHT          162

#define E11_LIGHT         138
#define E12_LIGHT         161
#define E13_LIGHT         137
#define E14_LIGHT         136
#define E15_LIGHT         165

#define E16_LIGHT         157
#define E17_LIGHT         129
#define E18_LIGHT         164
#define E19_LIGHT         177
#define E20_LIGHT         166

#define E21_LIGHT         142
#define E22_LIGHT         131
#define E23_LIGHT         132
//      E24_LIGHT <not wired>
//      E25_LIGHT <not wired>

#define E26_LIGHT         187
#define E27_LIGHT         156
#define E28_LIGHT         151
#define E29_LIGHT         148
#define E30_LIGHT         155

#define E31_LIGHT         154
#define E32_LIGHT         167
#define E33_LIGHT         159
#define E34_LIGHT         144
#define E35_LIGHT         134

#define E36_LIGHT         163
#define E37_LIGHT         139
#define E38_LIGHT         130
#define E39_LIGHT         171
#define E40_LIGHT         160

#define E41_LIGHT         141
#define E42_LIGHT         135
#define E43_LIGHT         133
#define E44_LIGHT         169
#define E45_LIGHT         147

#define E46_LIGHT         189
#define E47_LIGHT         181
#define E48_LIGHT         152
#define E49_LIGHT         185
#define E50_LIGHT         175

#define E51_LIGHT         191
#define E52_LIGHT         150
#define E53_LIGHT         170
#define E54_LIGHT         173
#define E55_LIGHT         179

#define RUN_LIGHT         180
#define START_CTRL_LIGHT  174
#define INSERT_2_LIGHT    190
#define DISP_ADR_3_LIGHT  182
#define SAVE_2_LIGHT      188

// CONTROL GATES (pwm)

#define DIV_1_CYCLE_LIGHT  121
#define DIV_ADD_LIGHT      119
#define DIV_LAST_CYC_LIGHT 123
#define OFLO_UNFLO_LIGHT   178
#define OFLO_CORR_LIGHT    172

#define RE_COMP_LIGHT      176
#define CARRY_IN_LIGHT     125
#define CARRY_OUT_LIGHT    186
#define CYCLE_CTRL_LIGHT   127
#define GR_50_LIGHT        184

#define REC_CTRL_LIGHT       1
#define TC_1_LIGHT           7
#define IA_LIGHT             9
//      SET_00080_LIGHT <not wired>
#define DIGIT_LIGHT         16

#define DIV_SIGN_LIGHT       6
#define UNFLO_ZERO_LIGHT     2
#define FP_EXIT_LIGHT       14
//      VRC_GATE_LIGHT <not wired>
//      END_COMPR_LIGHT <not wired>

#define FIRST_CYCLE_LIGHT   12
#define FIELD_MK_1_LIGHT    19
#define FIELD_MK_2_LIGHT    13
#define REC_MARK_LIGHT       4
#define BRCH_LIGHT          15

#define A_ADV_LIGHT         21
#define DECR_LIGHT           5
#define INCR_LIGHT           3
#define PLUS_2_LIGHT        17
#define BYPASS_LIGHT        22

#define ODD_LIGHT           11
#define I_TIME_LIGHT         8
#define E_TIME_LIGHT        10
#define HP_LIGHT            18
#define EZ_LIGHT            20

// INPUT-OUTPUT (pwm)

#define MEM_OFLOW_LIGHT  113
#define STOP_LIGHT        55
//      UNLABELED_LIGHT <not wired>
#define LAST_CARD_LIGHT   53
//      UNLABELED_LIGHT <not wired>

#define DISCN_GATE_LIGHT 124
#define RSP_GATE_LIGHT    42
#define RW_CALL_LIGHT     46
#define IO_EXIT_LIGHT     52
#define IO_HOLD_LIGHT    122

//      UNLABELED_LIGHT <not wired>
#define PCH_FEED_LIGHT   114
#define IO_FLAG_LIGHT    117
#define IO_SYNC_LIGHT     45
#define RDR_FEED_LIGHT   126

// OPERATION REGISTER (pwm)

#define OR_C0_LIGHT  38
#define OR_80_LIGHT 106
#define OR_40_LIGHT 102
#define OR_20_LIGHT  27
#define OR_10_LIGHT 104

#define OR_0C_LIGHT  44
#define OR_08_LIGHT 120
#define OR_04_LIGHT  82
#define OR_02_LIGHT  26
#define OR_01_LIGHT 118

// MULTIPLIER / QUOTIENT (pwm)

#define MQ_C_LIGHT  40
#define MQ_8_LIGHT  34
#define MQ_4_LIGHT 115
#define MQ_2_LIGHT  50
#define MQ_1_LIGHT  23

// SENSE AND BRANCH (pwm)

#define SAB_C0_LIGHT  36
#define SAB_80_LIGHT  67
#define SAB_40_LIGHT  65
#define SAB_20_LIGHT  71
#define SAB_10_LIGHT  33

#define SAB_0C_LIGHT  47
#define SAB_08_LIGHT  43
#define SAB_04_LIGHT 111
#define SAB_02_LIGHT  79
#define SAB_01_LIGHT  96

// MEMORY BUFFER REGISTER (pwm)

#define MBR_EC_LIGHT 109
#define MBR_EF_LIGHT  51
#define MBR_E8_LIGHT  37
#define MBR_E4_LIGHT  30
#define MBR_E2_LIGHT 103
#define MBR_E1_LIGHT  73

#define MBR_OC_LIGHT  93
#define MBR_OF_LIGHT 100
#define MBR_O8_LIGHT 112
#define MBR_O4_LIGHT  86
#define MBR_O2_LIGHT  94
#define MBR_O1_LIGHT  25

// MEMORY DATA REGISTER (pwm)

#define MDR_C_LIGHT  81
#define MDR_F_LIGHT  24
#define MDR_8_LIGHT 110
#define MDR_4_LIGHT  48
#define MDR_2_LIGHT  98
#define MDR_1_LIGHT  88

// DIGIT REGISTER (pwm)

#define DR_C0_LIGHT  39
//      UNLABELED_LIGHT <not wired>
#define DR_80_LIGHT 116
#define DR_40_LIGHT  29
#define DR_20_LIGHT  92
#define DR_10_LIGHT  90

#define DR_0C_LIGHT  28
//      UNLABELED_LIGHT <not wired>
#define DR_08_LIGHT 107
#define DR_04_LIGHT 105
#define DR_02_LIGHT  32
#define DR_01_LIGHT  76

// MEMORY ADDRESS REGISTER (pwm)

#define MAR_C0000_LIGHT 101
#define MAR_80000_LIGHT  69
#define MAR_40000_LIGHT  97
#define MAR_20000_LIGHT  85
#define MAR_10000_LIGHT  74

#define MAR_0C000_LIGHT  49
#define MAR_08000_LIGHT  70
#define MAR_04000_LIGHT  72
#define MAR_02000_LIGHT  89
#define MAR_01000_LIGHT  77

#define MAR_00C00_LIGHT  78
#define MAR_00800_LIGHT 108
#define MAR_00400_LIGHT  87
#define MAR_00200_LIGHT  75
#define MAR_00100_LIGHT  66

#define MAR_000C0_LIGHT  41
#define MAR_00080_LIGHT  84
#define MAR_00040_LIGHT  83
#define MAR_00020_LIGHT  91
#define MAR_00010_LIGHT  68

#define MAR_0000C_LIGHT  31
#define MAR_00008_LIGHT  35
#define MAR_00004_LIGHT  95
#define MAR_00002_LIGHT  80
#define MAR_00001_LIGHT  99

// MEMORY ADDRESS REGISTER DISPLAY SELECTOR (gpio)

#define OR1_SELECTOR   42
#define OR2_SELECTOR   40
#define OR3_SELECTOR   38
#define PR1_SELECTOR   36
#define PR2_SELECTOR   34
#define PR3_SELECTOR   32
#define OR4_SELECTOR   47
#define OR5_SELECTOR   45
#define CR1_SELECTOR   43
#define IR3_SELECTOR   27
#define IR1_SELECTOR   46
#define IR2_SELECTOR   44

// Spare 1 Toggle Switch (gpio)

//      UNLABELED_LIGHT <not wired>
//      UNLABELED_LIGHT <not wired>
//      UNLABELED_LIGHT <not wired>
#define SPARE_1_SWITCH 31

// Spare 2 Toggle Switch (gpio)

//      UNLABELED_LIGHT <not wired>
//      UNLABELED_LIGHT <not wired>
//      UNLABELED_LIGHT <not wired>
#define SPARE_2_SWITCH 16

// PARITY (pwm, gpio)

#define MAR_CHK_LIGHT      63
//      MBR_E_CHK_LIGHT <56>
//      MBR_O_CHK_LIGHT <62>
#define PARITY_STOP_SWITCH 18

// I/O (pwm, gpio)

//      UNLABELED_LIGHT <61>
#define RD_CHK_LIGHT   57
#define WR_CHK_LIGHT   54
#define IO_STOP_SWITCH 20

// O'FLOW (pwm, gpio)

//      UNLABELED_LIGHT <60>
#define EXP_CHK_LIGHT     58
#define ARITH_CHK_LIGHT   59
#define OFLOW_STOP_SWITCH 22

// PROGRAM SWITCHES (gpio)

#define PROGRAM_1_SWITCH 30
#define PROGRAM_2_SWITCH 28
#define PROGRAM_3_SWITCH 24
#define PROGRAM_4_SWITCH 26

// Status Lights (gpio)

// Values are negated so SetLight() can distinguish these from pwm lights
#define POWER_ON_LIGHT       -17
#define POWER_READY_LIGHT    -19
#define THERMAL_LIGHT        -21
#define PUNCH_NO_FEED_LIGHT  -23
#define READER_NO_FEED_LIGHT -25
#define SAVE_LIGHT            -1
#define INSERT_LIGHT          -3
//      UNLABELED_LIGHT <not wired>
#define AUTOMATIC_LIGHT       -5
#define MANUAL_LIGHT          -7
//      UNLABELED_LIGHT <not wired>
#define CHECK_STOP_LIGHT      -9
//      UNLABELED_LIGHT <not wired>

// Control Switches (gpio)

#define EMERGENCY_OFF_SWITCH    29
#define POWER_SWITCH            11
#define RESET_SWITCH            13
//      UNLABELED_SWITCH <not wired>
//      UNLABELED_SWITCH <not wired>
#define DISPLAY_MAR_SWITCH      15
#define SAVE_SWITCH              0
#define INSERT_SWITCH            2
#define RELEASE_SWITCH           4
#define START_SWITCH             6
#define STOP_SIE_SWITCH          8  // no = 8, nc = 10
#define INSTANT_STOP_SCE_SWITCH 12  // no = 12, nc = 14
//      UNLABELED_SWITCH <not wired>

// Daughterboard Test Lights (pwm)

#define BOARD_0_TEST_LIGHT   0
#define BOARD_1_TEST_LIGHT  64
#define BOARD_2_TEST_LIGHT 128

#endif /* DEFINES_H_ */
