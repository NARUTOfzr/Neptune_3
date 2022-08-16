/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "../DGUSDisplayDef.h"

#define DGUS_MKS_RUNOUT_SENSOR

#define LOGO_TIME_DELAY TERN(USE_MKS_GREEN_UI, 8000, 1500)

#if ENABLED(DGUS_MKS_RUNOUT_SENSOR)
  #define MT_DET_1_PIN     PB4
  // #define MT_DET_2_PIN     2
  #define MT_DET_PIN_STATE LOW
#endif

#define MKS_FINSH

extern celsius_t mks_PLA_default_e0_temp; // Initialized by settings.load()
extern celsius_t mks_PLA_default_bed_temp; // Initialized by settings.load()

extern celsius_t mks_ABS_default_e0_temp; // Initialized by settings.load()
extern celsius_t mks_ABS_default_bed_temp; // Initialized by settings.load()

extern celsius_t mks_AL_default_e0_temp; // Initialized by settings.load()
extern celsius_t mks_AL_default_bed_temp; // Initialized by settings.load()

extern uint16_t mks_language; // Initialized by settings.load()
extern uint16_t mks_filament_det_enable;// Initialized by settings.load()

extern uint16_t manualE0_T_Step;
extern uint16_t manualHotB_T_Step;
extern uint16_t FeedratePercentageStep;
extern uint16_t FlowPercentageStep;
extern uint16_t FanPercentageStep;

extern uint16_t manualMoveStep;
extern uint16_t distanceFilament;
extern uint16_t filamentSpeed_mm_s;
extern uint16_t ZOffset_distanceStep;
extern uint16_t AUTO_ZOffset_distanceStep;
extern float    ZOffset_distance;
extern float    mesh_adj_distance;
extern float    Z_distance;

//extern struct { uint16_t h, m, s; } dgus_time;

extern xy_int_t mks_corner_offsets[5];
extern xyz_int_t mks_park_pos;
extern celsius_t mks_min_extrusion_temp;

void MKS_reset_settings(); // Restore persistent settings to defaults

void MKS_pause_print_move();
void MKS_resume_print_move();

extern float z_offset_add;

extern xyz_int_t tmc_step;

extern uint16_t lcd_default_light;

#if X_HAS_STEALTHCHOP
  extern uint16_t tmc_x_current;
#endif
#if Y_HAS_STEALTHCHOP
  extern uint16_t tmc_y_current;
#endif
#if Z_HAS_STEALTHCHOP
  extern uint16_t tmc_z_current;
#endif
#if E0_HAS_STEALTHCHOP
  extern uint16_t tmc_e0_current;
#endif
#if E1_HAS_STEALTHCHOP
  extern uint16_t tmc_e1_current;
#endif

typedef enum {
  EX_HEATING,
  EX_HEAT_STARUS,
  EX_CHANGING,
  EX_CHANGE_STATUS,
  EX_NONE,
} EX_STATUS_DEF;

typedef struct {
  //uint8_t ex_change_flag:1;
  //uint8_t ex_heat_flag:1;
  uint8_t ex_load_unload_flag:1;  //0:unload  1:load
  EX_STATUS_DEF ex_status;
  uint32_t ex_tick_start;
  uint32_t ex_tick_end;
  uint32_t ex_speed;
  uint32_t ex_length;
  uint32_t ex_need_time;
} EX_FILAMENT_DEF;

extern EX_FILAMENT_DEF ex_filament;

typedef enum {
  UNRUNOUT_STATUS,
  RUNOUT_STATUS,
  RUNOUT_WAITTING_STATUS,
  RUNOUT_BEGIN_STATUS,
  RUNOUT_CHECK_HIGH,
  RUNOUT_CHECK_LOW,
  RUNOUT_CHECK_20S_LOW,
  RUNOUT_CHECK_20S_HIGH,
  RUNOUT_UNCKECK,
} RUNOUT_MKS_STATUS_DEF;

typedef struct {
  RUNOUT_MKS_STATUS_DEF runout_status;
  uint8_t pin_status;
  uint8_t de_count;
  uint8_t de_times;

  uint32_t runout_tick_1=0;
  uint32_t runout_tick_2=0;
  float E_pos_breakout_before=0;
  float E_pos_breakout_after=0;
  float E_pos_differ=0; 

  uint32_t beeper_tick1;
  uint32_t beeper_tick2;
  uint8_t beeper_flg;
  uint8_t beeper_cnt;
} RUNOUT_MKS_DEF;

extern RUNOUT_MKS_DEF runout_mks;

typedef struct {
  uint8_t print_pause_start_flag:1;
  uint8_t runout_flag:1;
  bool blstatus;
  uint16_t x_pos;
  uint16_t y_pos;
  uint16_t z_pos;
} NOZZLE_PARK_DEF;

extern NOZZLE_PARK_DEF nozzle_park_mks;

enum DGUSLCD_Screens : uint8_t {
  #if ENABLED(USE_MKS_GREEN_UI)

    DGUSLCD_SCREEN_BOOT                 =  33,
    DGUSLCD_SCREEN_MAIN                 =  60,
    DGUSLCD_SCREEN_STATUS               =  60,
    DGUSLCD_SCREEN_STATUS2              =  60,
    DGUSLCD_SCREEN_PREHEAT              =  18,
    DGUSLCD_SCREEN_POWER_LOSS           = 111,
    DGUSLCD_SCREEN_MANUALMOVE           = 192,
    DGUSLCD_SCREEN_UTILITY              = 120,
    DGUSLCD_SCREEN_FILAMENT_UNLOADING   = 158,
    DGUSLCD_SCREEN_SDFILELIST           =  15,
    DGUSLCD_SCREEN_SDPRINTMANIPULATION  =  15,
    DGUSLCD_SCREEN_SDPRINTTUNE          =  17,
    DGUSLCD_SCREEN_FAN_CTRL             =  94,

    // for ZNP
    MKSLCD_SCREEN_BOOT                  =  33,
    MKSLCD_SCREEN_HOME                  =  60,   // MKS main page
    MKSLCD_SCREEN_SETTING               =  82,   // MKS Setting page / no wifi whit
    MKSLCD_SRCEEN_PREPARE               =  68,
    MKSLCD_AUTO_LEVEL                   =  62,
    MKSLCD_ABOUT                        =  93,
    MKSLCD_SCREEN_MOVE                  =  72,
    MKSLCD_SCREEN_PRINT                 =  78,
    MKSLCD_SCREEN_PAUSE                 =  80,
    MKSLCD_SCREEN_CHOOSE_FILE           =  76,
    MKSLCD_SCREEN_NO_CHOOSE_FILE        =  88,
    MKSLCD_SCREEN_PRINT_CONFIRM         =  90,
    MKSLCD_SCREEN_TEST                  =  91,
    MKS_LCD_PREHEAT                     =  74,
    MKS_LCD_PREHEAT_DEFAULT             =  83,
    MKS_LCD_PREHEAT_DEFAULT_PLA         =  85,
    MKS_LCD_PREHEAT_DEFAULT_ABS         =  86,
    MKS_LCD_PREHEAT_DEFAULT_AL          =  87,
    MKSLCD_SCREEN_EXTRUDE_P1            =  100,
    MKSLCD_SCREEN_FILCHANGE             =  101,
    MKSLCD_SCREEN_PRINT_STAT_STOP_POPUP =  102,
    MKSLCD_SCREEN_PRINT_PASU_STOP_POPUP =  103,
    MKSLCD_SCREEN_PrintDone             =  104,
    MKSLCD_Screen_Baby                  =  97,
    MKSLCD_Screen_LANGUAGE              =  63,
    MKSLCD_Screen_RUNOUT_POPUP          =  109,
    MKSLCD_Screen_EEPROM_POPUP          =  110,
    MKSLCD_Screen_AUTO_LEVEL_POPUP      =  112,
    MKSLCD_Screen_AUTO_LEVEL_POPUP1     =  113,
    MKSLCD_Screen_PRINT_FLAMENT_HOTB    =  114,
    MKSLCD_Screen_PRINT_FLAMENT         =  115,

    MKSLCD_Screen_PRINT_SPEED           =  108,
    MKSLCD_Screen_FLOW_SPEED            =  116,
    MKSLCD_Screen_FAN_SPEED             =  117,

    MKSLCD_SCREEM_TOOL                  =  64,   // MKS Tool page
    MKSLCD_SCREEN_EXTRUDE_P2            =  77,
    MKSLCD_SCREEN_LEVEL                 =  73,
    MKSLCD_Screen_PRINT_FLAMENT_LOAD    =  119,
    MKSLCD_Screen_PRINT_FLAMENT_UNLOAD  =  120,
    MKSLCD_Screen_EXTRUDE_HEATING_POPUP =  121,
    MKSLCD_Screen_EXTRUDE_LOAD_POPUP    =  122,
    MKSLCD_Screen_EXTRUDE_UNLOAD_POPUP  =  123,
    MKSLCD_Screen_ERR1_POPUP            =  124,
    MKSLCD_Screen_ERR2_POPUP            =  125,
    MKSLCD_Screen_ERR3_POPUP            =  126,
    MKSLCD_Screen_ERR4_POPUP            =  127,
    MKSLCD_Screen_ERR5_POPUP            =  128,
    MKSLCD_Screen_ERR6_POPUP            =  129,
    MKSLCD_Screen_ERR7_POPUP            =  130,

    //MKSLCD_SCREEN_Config                = 101,
    //MKSLCD_SCREEN_Config_MOTOR          = 103,
    //MKSLCD_SCREEN_MOTOR_PLUSE           = 104,
    //MKSLCD_SCREEN_MOTOR_SPEED           = 102,
    //MKSLCD_SCREEN_MOTOR_ACC_MAX         = 105,
    //MKSLCD_SCREEN_PRINT_CONFIG          =  200,
    //MKSLCD_SCREEN_LEVEL_DATA            = 106,
    //MKSLCD_PrintPause_SET               = 107,
    //MKSLCD_FILAMENT_DATA                =  50,
    
    //MKSLCD_PID                          = 108,
    //MKSLCD_PAUSE_SETTING_MOVE           =  98,
    //MKSLCD_PRINT_SETTING                =  96,
    //MKSLCD_PAUSE_SETTING_EX2            =  97,
    //MKSLCD_SCREEN_PRINT_CONFIRM         =  90,
    //MKSLCD_SCREEN_EX_CONFIG             = 112,
    //MKSLCD_SCREEN_EEP_Config            =  89,
    //MKSLCD_SCREEN_PrintDone             =  104,
    //MKSLCD_SCREEN_Filmout               =  105,
    //MKSLCD_SCREEN_TMC_Config            = 111,
    //MKSLCD_Screen_Offset_Config         = 109,
    //MKSLCD_Screen_PMove                 =  98,
  #else

    DGUSLCD_SCREEN_BOOT                 = 120,
    DGUSLCD_SCREEN_MAIN                 =   1,
    DGUSLCD_SCREEN_STATUS               =   1,
    DGUSLCD_SCREEN_STATUS2              =   1,
    DGUSLCD_SCREEN_PREHEAT              =  18,
    DGUSLCD_SCREEN_POWER_LOSS           = 100,
    DGUSLCD_SCREEN_MANUALMOVE           = 192,
    DGUSLCD_SCREEN_UTILITY              = 120,
    DGUSLCD_SCREEN_FILAMENT_UNLOADING   = 158,
    DGUSLCD_SCREEN_SDFILELIST           =  15,
    DGUSLCD_SCREEN_SDPRINTMANIPULATION  =  15,
    DGUSLCD_SCREEN_SDPRINTTUNE          =  17,

    MKSLCD_SCREEN_BOOT                  =   0,
    MKSLCD_SCREEN_HOME                  =   1,   // MKS main page
    MKSLCD_SCREEN_SETTING               =   2,   // MKS Setting page / no wifi whit
    MKSLCD_SCREEM_TOOL                  =   3,   // MKS Tool page
    MKSLCD_SCREEN_EXTRUDE_P1            =   4,
    MKSLCD_SCREEN_EXTRUDE_P2            =  11,
    MKSLCD_SCREEN_LEVEL                 =   5,
    MKSLCD_AUTO_LEVEL                   =  73,
    MKSLCD_SCREEN_LEVEL_PRESS           =   9,
    MKSLCD_SCREEN_MOVE                  =   6,
    MKSLCD_SCREEN_PRINT                 =   7,
    MKSLCD_SCREEN_PRINT_PRESS           =  13,
    MKSLCD_SCREEN_PAUSE                 =  26,
    MKSLCD_SCREEN_PAUSE_PRESS           =  26,
    MKSLCD_SCREEN_CHOOSE_FILE           =  15,
    MKSLCD_SCREEN_NO_CHOOSE_FILE        =  17,
    MKSLCD_SCREEN_Config                =  46,
    MKSLCD_SCREEN_Config_MOTOR          =  47,
    MKSLCD_SCREEN_MOTOR_PLUSE           =  51,
    MKSLCD_SCREEN_MOTOR_SPEED           =  55,
    MKSLCD_SCREEN_MOTOR_ACC_MAX         =  53,
    MKSLCD_SCREEN_PRINT_CONFIG          =  60,
    MKSLCD_SCREEN_LEVEL_DATA            =  48,
    MKSLCD_PrintPause_SET               =  49,
    MKSLCD_FILAMENT_DATA                =  50,
    MKSLCD_ABOUT                        =  36,
    MKSLCD_PID                          =  56,
    MKSLCD_PAUSE_SETTING_MOVE           =  58,
    MKSLCD_PAUSE_SETTING_EX             =  57,
    MKSLCD_PAUSE_SETTING_EX2            =  61,
    MKSLCD_SCREEN_NO_FILE               =  42,
    MKSLCD_SCREEN_PRINT_CONFIRM         =  43,
    MKSLCD_SCREEN_EX_CONFIG             =  65,
    MKSLCD_SCREEN_EEP_Config            =  20,
    MKSLCD_SCREEN_PrintDone             =  25,
    MKSLCD_SCREEN_TMC_Config            =  70,
    MKSLCD_Screen_Offset_Config         =  30,
    MKSLCD_Screen_PMove                 =  64,
    MKSLCD_Screen_Baby                  =  71,
  #endif

  DGUSLCD_WATCHDOG_TIMEOUT                = 170,
  //DGUSLCD_SCREEN_CONFIRM                = 240,
  DGUSLCD_SCREEN_KILL                   = 250, ///< Kill Screen. Must always be 250 (to be able to display "Error wrong LCD Version")
  //DGUSLCD_SCREEN_WAITING                = 251,
  //DGUSLCD_SCREEN_POPUP                  = 252, ///< special target, popup screen will also return this code to say "return to previous screen"
  //DGUSLCD_SCREEN_UNUSED                 = 255
};


// Place for status messages.
constexpr uint16_t VP_M117 = 0x7020;
constexpr uint8_t VP_M117_LEN = 0x20;

// Heater status
constexpr uint16_t VP_E0_STATUS = 0x3410;
constexpr uint16_t VP_E1_STATUS = 0x3412;
//constexpr uint16_t VP_E2_STATUS = 0x3414;
//constexpr uint16_t VP_E3_STATUS = 0x3416;
//constexpr uint16_t VP_E4_STATUS = 0x3418;
//constexpr uint16_t VP_E5_STATUS = 0x341A;
constexpr uint16_t VP_MOVE_OPTION = 0x3500;

// // PIDs
// constexpr uint16_t VP_E0_PID_P = 0x3700; // at the moment , 2 byte unsigned int , 0~1638.4
// constexpr uint16_t VP_E0_PID_I = 0x3702;
// constexpr uint16_t VP_E0_PID_D = 0x3704;
// constexpr uint16_t VP_E1_PID_P = 0x3706; // at the moment , 2 byte unsigned int , 0~1638.4
// constexpr uint16_t VP_E1_PID_I = 0x3708;
// constexpr uint16_t VP_E1_PID_D = 0x370A;
// constexpr uint16_t VP_BED_PID_P = 0x3710;
// constexpr uint16_t VP_BED_PID_I = 0x3712;
// constexpr uint16_t VP_BED_PID_D = 0x3714;

// Waiting screen status
constexpr uint16_t VP_WAITING_STATUS = 0x3800;

// SPs for certain variables...
// located at 0x5000 and up
// Not used yet!
// This can be used e.g to make controls / data display invisible
constexpr uint16_t SP_T_E0_Is     = 0x5000;
constexpr uint16_t SP_T_E0_Set    = 0x5010;
constexpr uint16_t SP_T_E1_Is     = 0x5020;
constexpr uint16_t SP_T_Bed_Is    = 0x5030;
constexpr uint16_t SP_T_Bed_Set   = 0x5040;

/*************************************************************************************************************************
 *************************************************************************************************************************
 *                                                 DGUS for MKS Mem layout
 ************************************************************************************************************************
 ************************************************************************************************************************/

#if ENABLED(MKS_FINSH)
  /* -------------------------------0x1000-0x1FFF------------------------------- */
  constexpr uint16_t VP_MSGSTR1                       = 0x1100;
  constexpr uint8_t  VP_MSGSTR1_LEN                   = 0x20;  // might be more place for it...
  constexpr uint16_t VP_MSGSTR2                       = 0x1140;
  constexpr uint8_t  VP_MSGSTR2_LEN                   = 0x20;
  constexpr uint16_t VP_MSGSTR3                       = 0x1180;
  constexpr uint8_t  VP_MSGSTR3_LEN                   = 0x20;
  constexpr uint16_t VP_MSGSTR4                       = 0x11C0;
  constexpr uint8_t  VP_MSGSTR4_LEN                   = 0x20;

  constexpr uint16_t VP_MARLIN_VERSION                = 0x1A00;
  constexpr uint8_t VP_MARLIN_VERSION_LEN             = 16;       // there is more space on the display, if needed.


  constexpr uint16_t VP_SCREENCHANGE_ASK              = 0x1500;
  constexpr uint16_t VP_SCREENCHANGE                  = 0x1501;   // Key-Return button to new menu pressed. Data contains target screen in low byte and info in high byte.
  constexpr uint16_t VP_TEMP_ALL_OFF                  = 0x1502;   // Turn all heaters off. Value arbitrary ;)=
  constexpr uint16_t VP_SCREENCHANGE_WHENSD           = 0x1503;   // "Print" Button touched -- go only there if there is an SD Card.
  constexpr uint16_t VP_CONFIRMED                     = 0x1510;   // OK on confirm screen.

  constexpr uint16_t VP_BACK_PAGE                     = 0x1600;
  constexpr uint16_t VP_SETTINGS                      = 0x1620;
  // Power loss recovery
  constexpr uint16_t VP_POWER_LOSS_RECOVERY           = 0x1680;
  /* -------------------------------0x2000-0x2FFF------------------------------- */
  // Temperatures.
  constexpr uint16_t VP_T_E0_Is                       = 0x2000;   // 4 Byte Integer
  constexpr uint16_t VP_T_E0_Set                      = 0x2004;   // 2 Byte Integer
  constexpr uint16_t VP_T_E1_Is                       = 0x2008;   // 4 Byte Integer
  constexpr uint16_t VP_T_E1_Set                      = 0x200B;   // 2 Byte Integer
  constexpr uint16_t VP_T_E2_Is                       = 0x2010;   // 4 Byte Integer
  constexpr uint16_t VP_T_E2_Set                      = 0x2014;   // 2 Byte Integer
  constexpr uint16_t VP_T_E3_Is                       = 0x2018;   // 4 Byte Integer
  constexpr uint16_t VP_T_E3_Set                      = 0x201B;   // 2 Byte Integer
  constexpr uint16_t VP_T_E4_Is                       = 0x2020;   // 4 Byte Integer
  constexpr uint16_t VP_T_E4_Set                      = 0x2024;   // 2 Byte Integer
  constexpr uint16_t VP_T_E5_Is                       = 0x2028;   // 4 Byte Integer
  constexpr uint16_t VP_T_E5_Set                      = 0x202B;   // 2 Byte Integer
  constexpr uint16_t VP_T_E6_Is                       = 0x2030;   // 4 Byte Integer
  constexpr uint16_t VP_T_E6_Set                      = 0x2034;   // 2 Byte Integer
  constexpr uint16_t VP_T_E7_Is                       = 0x2038;   // 4 Byte Integer
  constexpr uint16_t VP_T_E7_Set                      = 0x203B;   // 2 Byte Integer

  constexpr uint16_t VP_T_Bed_Is                      = 0x2040;   // 4 Byte Integer
  constexpr uint16_t VP_T_Bed_Set                     = 0x2044;   // 2 Byte Integer

  constexpr uint16_t VP_Min_EX_T_E                    = 0x2100;
  
  constexpr uint16_t VP_T_E0_Step                     = 0x2104;   // 2 Byte Integer
  constexpr uint16_t VP_T_E0_Adjust                   = 0x2106;   // 2 Byte Integer
  constexpr uint16_t VP_T_Bed_Step                    = 0x2108;   // 2 Byte Integer
  constexpr uint16_t VP_T_Bed_Adjust                  = 0x210a;   // 2 Byte Integer

  
  constexpr uint16_t VP_Flowrate_E0                   = 0x2200;   // 2 Byte Integer
  constexpr uint16_t VP_Flowrate_E1                   = 0x2202;   // 2 Byte Integer
  constexpr uint16_t VP_Flowrate_E2                   = 0x2204;
  constexpr uint16_t VP_Flowrate_E3                   = 0x2206;
  constexpr uint16_t VP_Flowrate_E4                   = 0x2208;
  constexpr uint16_t VP_Flowrate_E5                   = 0x220A;
  constexpr uint16_t VP_Flowrate_E6                   = 0x220C;
  constexpr uint16_t VP_Flowrate_E7                   = 0x220E;

  constexpr uint16_t VP_Flowrate_E0_Adjust            = 0x2220;   // 2 Byte Integer
  constexpr uint16_t VP_Flowrate_E0_Step              = 0x2222;   // 2 Byte Integer

  // Move
  constexpr uint16_t VP_MOVE_X                        = 0x2300;
  constexpr uint16_t VP_MOVE_Y                        = 0x2302;
  constexpr uint16_t VP_MOVE_Z                        = 0x2304;
  constexpr uint16_t VP_MOVE_E0                       = 0x2310;
  constexpr uint16_t VP_MOVE_E1                       = 0x2312;
  constexpr uint16_t VP_MOVE_E2                       = 0x2314;
  constexpr uint16_t VP_MOVE_E3                       = 0x2316;
  constexpr uint16_t VP_MOVE_E4                       = 0x2318;
  constexpr uint16_t VP_MOVE_E5                       = 0x231A;
  constexpr uint16_t VP_MOVE_E6                       = 0x231C;
  constexpr uint16_t VP_MOVE_E7                       = 0x231E;
  constexpr uint16_t VP_HOME_ALL                      = 0x2320;
  constexpr uint16_t VP_MOTOR_LOCK_UNLOK              = 0x2330;
  constexpr uint16_t VP_MOVE_DISTANCE                 = 0x2334;
  constexpr uint16_t VP_X_HOME                        = 0x2336;
  constexpr uint16_t VP_Y_HOME                        = 0x2338;
  constexpr uint16_t VP_Z_HOME                        = 0x233A;

  // Fan Control Buttons , switch between "off" and "on"
  constexpr uint16_t VP_FAN0_CONTROL                  = 0x2350;
  constexpr uint16_t VP_FAN1_CONTROL                  = 0x2352;
  constexpr uint16_t VP_FAN2_CONTROL                  = 0x2354;
  constexpr uint16_t VP_FAN3_CONTROL                  = 0x2356;
  constexpr uint16_t VP_FAN4_CONTROL                  = 0x2358;
  constexpr uint16_t VP_FAN5_CONTROL                  = 0x235A;

  constexpr uint16_t VP_LANGUAGE_CHANGE               = 0x2380;
  constexpr uint16_t VP_LANGUAGE_CHANGE1              = 0x2382;
  constexpr uint16_t VP_LANGUAGE_CHANGE2              = 0x2384;
  constexpr uint16_t VP_LANGUAGE_CHANGE3              = 0x2386;
  constexpr uint16_t VP_LANGUAGE_CHANGE4              = 0x2388;
  constexpr uint16_t VP_LANGUAGE_CHANGE5              = 0x238A;

  // LEVEL
  constexpr uint16_t VP_LEVEL_POINT                   = 0x2400;
  constexpr uint16_t VP_MESH_LEVEL_POINT              = 0x2410;
  constexpr uint16_t VP_MESH_LEVEL_ADJUST             = 0x2412;
  constexpr uint16_t VP_MESH_LEVEL_DIP                = 0x2414;
  constexpr uint16_t VP_MESH_LEVEL_POINT_X            = 0x2416;
  constexpr uint16_t VP_MESH_LEVEL_POINT_Y            = 0x2418;
  constexpr uint16_t VP_LEVEL_BUTTON                  = 0x2420;
  constexpr uint16_t VP_MESH_LEVEL_POINT_DIS          = 0x2422;
  constexpr uint16_t VP_MESH_LEVEL_BACK               = 0x2424;

  constexpr uint16_t VP_AUTO_LEVEL_SET                = 0x2426;

  constexpr uint16_t VP_E0_FILAMENT_LOAD_UNLOAD       = 0x2500;
  constexpr uint16_t VP_E1_FILAMENT_LOAD_UNLOAD       = 0x2504;
  constexpr uint16_t VP_LOAD_Filament                 = 0x2508;
  // constexpr uint16_t VP_LOAD_UNLOAD_Cancle            = 0x250A;
  constexpr uint16_t VP_UNLOAD_Filament               = 0x250B;
  constexpr uint16_t VP_Extrude_Cancel_heating        = 0x2510;   //cancel heating
  constexpr uint16_t VP_Extrude_Load                  = 0x2512;   //
  constexpr uint16_t VP_Extrude_Unload                = 0x2514;   //

  constexpr uint16_t VP_Filament_distance             = 0x2600;
  constexpr uint16_t VP_Filament_speed                = 0x2604;
  constexpr uint16_t VP_MIN_EX_T                      = 0x2606;

  constexpr uint16_t VP_E1_Filament_distance          = 0x2614;
  constexpr uint16_t VP_E1_Filament_speed             = 0x2616;
  constexpr uint16_t VP_E1_MIN_EX_T                   = 0x2618;
  
  constexpr uint16_t VP_E0_Print_Filament_load	      = 0x2620;
  constexpr uint16_t VP_E0_Print_Filament_unload	  = 0x2622;

  constexpr uint16_t VP_E0_Print_Filament_load_confirm	 = 0x2624;
  constexpr uint16_t VP_E0_Print_Filament_unload_confirm = 0x2626;


  constexpr uint16_t VP_RUNOUT_Confirm                = 0x2630;
  //constexpr uint16_t VP_FILAMENT_DET_ENABLE           = 0x2632;

  constexpr uint16_t VP_Fan0_Percentage               = 0x2700;   // 2 Byte Integer (0..100)
  constexpr uint16_t VP_Fan1_Percentage               = 0x2702;   // 2 Byte Integer (0..100)
  constexpr uint16_t VP_Fan2_Percentage               = 0x2704;   // 2 Byte Integer (0..100)
  constexpr uint16_t VP_Fan3_Percentage               = 0x2706;   // 2 Byte Integer (0..100)

  constexpr uint16_t VP_Feedrate_Percentage           = 0x2708;   // 2 Byte Integer (0..100)
  constexpr uint16_t VP_Feedrate_Percentage_Adjust    = 0x270A;   // 2 Byte Integer (0..100)
  constexpr uint16_t VP_Feedrate_Percentage_step      = 0x270C;   // 2 Byte Integer (0..100)

  // Fan status
  constexpr uint16_t VP_FAN0_STATUS                   = 0x2710;
  constexpr uint16_t VP_FAN1_STATUS                   = 0x2712;
  constexpr uint16_t VP_FAN2_STATUS                   = 0x2714;
  constexpr uint16_t VP_FAN3_STATUS                   = 0x2716;

  constexpr uint16_t VP_Fan0_Percentage_Adjust        = 0x2718;   // 2 Byte Integer (0..100)
  constexpr uint16_t VP_Fan0_Percentage_step          = 0x271A;   // 2 Byte Integer (0..100)

  constexpr uint16_t VP_Fan0_Percentage_pic           = 0x2730;

  // Step per mm
  constexpr uint16_t VP_X_STEP_PER_MM                 = 0x2900;   // at the moment , 2 byte unsigned int , 0~1638.4
  constexpr uint16_t VP_Y_STEP_PER_MM                 = 0x2904;
  constexpr uint16_t VP_Z_STEP_PER_MM                 = 0x2908;
  constexpr uint16_t VP_E0_STEP_PER_MM                = 0x2910;
  constexpr uint16_t VP_E1_STEP_PER_MM                = 0x2912;
  constexpr uint16_t VP_E2_STEP_PER_MM                = 0x2914;
  constexpr uint16_t VP_E3_STEP_PER_MM                = 0x2916;
  constexpr uint16_t VP_E4_STEP_PER_MM                = 0x2918;
  constexpr uint16_t VP_E5_STEP_PER_MM                = 0x291A;
  constexpr uint16_t VP_E6_STEP_PER_MM                = 0x291C;
  constexpr uint16_t VP_E7_STEP_PER_MM                = 0x291E;

  constexpr uint16_t VP_X_MAX_SPEED                   = 0x2A00;
  constexpr uint16_t VP_Y_MAX_SPEED                   = 0x2A04;
  constexpr uint16_t VP_Z_MAX_SPEED                   = 0x2A08;
  constexpr uint16_t VP_E0_MAX_SPEED                  = 0x2A0C;
  constexpr uint16_t VP_E1_MAX_SPEED                  = 0x2A10;

  constexpr uint16_t VP_X_ACC_MAX_SPEED               = 0x2A28;
  constexpr uint16_t VP_Y_ACC_MAX_SPEED               = 0x2A2C;
  constexpr uint16_t VP_Z_ACC_MAX_SPEED               = 0x2A30;
  constexpr uint16_t VP_E0_ACC_MAX_SPEED              = 0x2A34;
  constexpr uint16_t VP_E1_ACC_MAX_SPEED              = 0x2A38;

  constexpr uint16_t VP_TRAVEL_SPEED                  = 0x2A3C;
  constexpr uint16_t VP_FEEDRATE_MIN_SPEED            = 0x2A40;
  constexpr uint16_t VP_T_F_SPEED                     = 0x2A44;
  constexpr uint16_t VP_ACC_SPEED                     = 0x2A48;

  constexpr uint16_t VP_PLA_ABS_SET                   = 0x2A50;

  constexpr uint16_t VP_PRINT_SET_BACK                = 0x2A54;
  constexpr uint16_t VP_PRINT_SET                     = 0x2A56;

  constexpr uint16_t VP_PRINT_SET_PWR                 = 0x2A58;
  constexpr uint16_t VP_PRINT_SET_PWR_VAL             = 0x2A5A;
  constexpr uint16_t VP_PRINT_SET_SPEED               = 0x2A5C;
  constexpr uint16_t VP_PRINT_SET_SPEED_VAL           = 0x2A5E;

  constexpr uint16_t VP_PRINT_SET_babystep_distan     = 0x2A60;

  constexpr uint16_t VP_DEFAULT_PLA_T_TEMP            = 0x2A62;
  constexpr uint16_t VP_DEFAULT_PLA_B_TEMP            = 0x2A64;
  constexpr uint16_t VP_DEFAULT_ABS_T_TEMP            = 0x2A66;
  constexpr uint16_t VP_DEFAULT_ABS_B_TEMP            = 0x2A68;  
  constexpr uint16_t VP_DEFAULT_TEMP_SAVE             = 0x2A6A;
  constexpr uint16_t VP_DEFAULT_AL_T_TEMP             = 0x2A6C;
  constexpr uint16_t VP_DEFAULT_AL_B_TEMP             = 0x2A6E;  

  constexpr uint16_t VP_Prepare_page                  = 0x2B00;
  constexpr uint16_t VP_FAN_page                      = 0x2B02;

  constexpr uint16_t VP_PREPARE_PAGE_SET              = 0x2B06;

  constexpr uint16_t VP_SETTING_PAGE_SET              = 0x2B0A;
  constexpr uint16_t VP_SETTING_TEMP_PAGE_SET         = 0x2B0C;
  constexpr uint16_t VP_FAN_ON_OFF                    = 0x2B0D;
  constexpr uint16_t VP_FAN_ON_OFF_VAL                = 0x2B0F;
  constexpr uint16_t VP_PRINT_SETTINGS                = 0x2B12;
  constexpr uint16_t VP_SETTING_PLA_DEFAULT_PAGE_SET  = 0x2B14;
  constexpr uint16_t VP_SETTING_ABS_DEFAULT_PAGE_SET  = 0x2B16;
  constexpr uint16_t VP_SETTING_FILAMENT_DET          = 0x2B28;

  constexpr uint16_t VP_LEVELING_POINT_OFFSET1         = 0x2C00;
  constexpr uint16_t VP_LEVELING_POINT_OFFSET2         = 0x2C02;
  constexpr uint16_t VP_LEVELING_POINT_OFFSET3         = 0x2C04;
  constexpr uint16_t VP_LEVELING_POINT_OFFSET4         = 0x2C06;
  constexpr uint16_t VP_LEVELING_POINT_OFFSET5         = 0x2C08;
  constexpr uint16_t VP_LEVELING_POINT_OFFSET6         = 0x2C0A;
  constexpr uint16_t VP_LEVELING_POINT_OFFSET7         = 0x2C0C;
  constexpr uint16_t VP_LEVELING_POINT_OFFSET8         = 0x2C0E;
  constexpr uint16_t VP_LEVELING_POINT_OFFSET9         = 0x2C10;
  constexpr uint16_t VP_LEVELING_POINT_OFFSET10         = 0x2C12;
  constexpr uint16_t VP_LEVELING_POINT_OFFSET11         = 0x2C14;
  constexpr uint16_t VP_LEVELING_POINT_OFFSET12         = 0x2C16;
  constexpr uint16_t VP_LEVELING_POINT_OFFSET13         = 0x2C18;
  constexpr uint16_t VP_LEVELING_POINT_OFFSET14         = 0x2C1A;
  constexpr uint16_t VP_LEVELING_POINT_OFFSET15         = 0x2C1C;
  constexpr uint16_t VP_LEVELING_POINT_OFFSET16         = 0x2C1E;

  /* -------------------------------0x3000-0x3FFF------------------------------- */
  // Buttons on the SD-Card File listing.
  constexpr uint16_t VP_SD_ScrollEvent                = 0x3020; // Data: 0 for "up a directory", numbers are the amount to scroll, e.g -1 one up, 1 one down
  constexpr uint16_t VP_SD_FileSelected               = 0x3022; // Number of file field selected.
  constexpr uint16_t VP_SD_FileSelectConfirm          = 0x3024; // (This is a virtual VP and emulated by the Confirm Screen when a file has been confirmed)
  constexpr uint16_t VP_SD_ResumePauseAbort           = 0x3026; // Resume(Data=0), Pause(Data=1), Abort(Data=2) SD Card prints
  constexpr uint16_t VP_SD_AbortPrintConfirmed        = 0x3028; // Abort print confirmation (virtual, will be injected by the confirm dialog)
  constexpr uint16_t VP_SD_Print_Setting              = 0x3040;
  constexpr uint16_t VP_SD_Print_LiveAdjustZ          = 0x3050; // Data: 0 down, 1 up
  constexpr uint16_t VP_SD_Print_LiveAdjustZ_Confirm  = 0x3060;
  constexpr uint16_t VP_ZOffset_Distance              = 0x3070;
  constexpr uint16_t VP_ZOffset_DE_DIS                = 0x3080;
  constexpr uint16_t VP_SD_FileSelect_Back            = 0x3082;
  constexpr uint16_t VP_SD_Print_Again                = 0x3084;

  // SDCard File Listing
  constexpr uint16_t VP_SD_FileName_LEN = 32;                   // LEN is shared for all entries.
  constexpr uint16_t DGUS_SD_FILESPERSCREEN = 5;               // FIXME move that info to the display and read it from there.
  constexpr uint16_t VP_SD_FileName0                  = 0x3100;
  constexpr uint16_t VP_SD_FileName1                  = 0x3120;
  constexpr uint16_t VP_SD_FileName2                  = 0x3140;
  constexpr uint16_t VP_SD_FileName3                  = 0x3160;
  constexpr uint16_t VP_SD_FileName4                  = 0x3180;
  constexpr uint16_t VP_SD_FileName5                  = 0x31A0;
  constexpr uint16_t VP_SD_FileName6                  = 0x31C0;
  constexpr uint16_t VP_SD_FileName7                  = 0x31E0;
  constexpr uint16_t VP_SD_FileName8                  = 0x3200;
  constexpr uint16_t VP_SD_FileName9                  = 0x3220;

  constexpr uint16_t VP_SD_Print_ProbeOffsetZ         = 0x32A0;
  constexpr uint16_t VP_SD_Print_Baby                 = 0x32B0;
  constexpr uint16_t VP_SD_Print_Filename             = 0x32C0;
  
  constexpr uint16_t VP_SD_read_err                   = 0x32E0;



  // X Y Z Point
  constexpr uint16_t VP_XPos = 0x3300;  // 4 Byte Fixed point number; format xxx.yy
  constexpr uint16_t VP_YPos = 0x3302;  // 4 Byte Fixed point number; format xxx.yy
  constexpr uint16_t VP_ZPos = 0x3304;  // 4 Byte Fixed point number; format xxx.yy
  constexpr uint16_t VP_EPos = 0x3306;  // 4 Byte Fixed point number; format xxx.yy

  // Print
  constexpr uint16_t VP_PrintProgress_Percentage      = 0X3330;//0x3330; // 2 Byte Integer (0..100)
  constexpr uint16_t VP_PrintProgress_Percentage_1    = 0X3332;//0x3330; // 2 Byte Integer (0..100)
  constexpr uint16_t VP_PrintTime                     = 0x3340;
  constexpr uint16_t VP_PrintTime_LEN                 = 32;
  constexpr uint16_t VP_PrintAccTime                  = 0x3360;
  constexpr uint16_t VP_PrintAccTime_LEN              = 32;
  constexpr uint16_t VP_PrintsTotal                   = 0x3380;
  constexpr uint16_t VP_PrintsTotal_LEN               = 16;

  constexpr uint16_t VP_File_Pictutr0                 = 0x3400;
  constexpr uint16_t VP_File_Pictutr1                 = 0x3402;
  constexpr uint16_t VP_File_Pictutr2                 = 0x3404;
  constexpr uint16_t VP_File_Pictutr3                 = 0x3406;
  constexpr uint16_t VP_File_Pictutr4                 = 0x3408;
  constexpr uint16_t VP_File_Pictutr5                 = 0x340A;
  constexpr uint16_t VP_File_Pictutr6                 = 0x340C;
  constexpr uint16_t VP_File_Pictutr7                 = 0x340E;
  constexpr uint16_t VP_File_Pictutr8                 = 0x3410;
  constexpr uint16_t VP_File_Pictutr9                 = 0x3412;

  constexpr uint16_t VP_BED_STATUS                    = 0x341C;

  constexpr uint16_t VP_TMC_X_STEP                    = 0x3430;
  constexpr uint16_t VP_TMC_Y_STEP                    = 0x3432;
  constexpr uint16_t VP_TMC_Z_STEP                    = 0x3434;

  constexpr uint16_t VP_TMC_X1_Current                = 0x3436;
  constexpr uint16_t VP_TMC_Y1_Current                = 0x3438;
  constexpr uint16_t VP_TMC_X_Current                 = 0x343A;
  constexpr uint16_t VP_TMC_Y_Current                 = 0x343C;
  constexpr uint16_t VP_TMC_Z_Current                 = 0x343E;
  constexpr uint16_t VP_TMC_E0_Current                = 0x3440;
  constexpr uint16_t VP_TMC_E1_Current                = 0x3442;
  constexpr uint16_t VP_TMC_Z1_Current                = 0x3444;

  constexpr uint16_t VP_PrintTime_H                   = 0x3500;
  constexpr uint16_t VP_PrintTime_M                   = 0x3502;
  constexpr uint16_t VP_PrintTime_S                   = 0x3504;

  // PIDs
  constexpr uint16_t VP_E0_PID_P = 0x3700; // at the moment , 2 byte unsigned int , 0~1638.4
  constexpr uint16_t VP_E0_PID_I = 0x3702;
  constexpr uint16_t VP_E0_PID_D = 0x3704;
  constexpr uint16_t VP_E1_PID_P = 0x3706; // at the moment , 2 byte unsigned int , 0~1638.4
  constexpr uint16_t VP_E1_PID_I = 0x3708;
  constexpr uint16_t VP_E1_PID_D = 0x370A;
  constexpr uint16_t VP_BED_PID_P = 0x3710;
  constexpr uint16_t VP_BED_PID_I = 0x3712;
  constexpr uint16_t VP_BED_PID_D = 0x3714;

  constexpr uint16_t VP_EEPROM_CTRL = 0x3720;

  constexpr uint16_t VP_OFFSET_X = 0x3724;
  constexpr uint16_t VP_OFFSET_Y = 0x3728;
  constexpr uint16_t VP_OFFSET_Z = 0x372B;

  // PID autotune
  constexpr uint16_t VP_PID_AUTOTUNE_E0 = 0x3800;
  constexpr uint16_t VP_PID_AUTOTUNE_E1 = 0x3802;
  constexpr uint16_t VP_PID_AUTOTUNE_E2 = 0x3804;
  constexpr uint16_t VP_PID_AUTOTUNE_E3 = 0x3806;
  constexpr uint16_t VP_PID_AUTOTUNE_E4 = 0x3808;
  constexpr uint16_t VP_PID_AUTOTUNE_E5 = 0x380A;
  constexpr uint16_t VP_PID_AUTOTUNE_BED = 0x380C;
  // Calibrate Z
  constexpr uint16_t VP_Z_CALIBRATE = 0x3810;

  constexpr uint16_t VP_AutoTurnOffSw = 0x3812;
  constexpr uint16_t VP_LCD_BLK = 0x3814;

  constexpr uint16_t VP_X_PARK_POS = 0x3900;
  constexpr uint16_t VP_Y_PARK_POS = 0x3902;
  constexpr uint16_t VP_Z_PARK_POS = 0x3904;

  /* -------------------------------0x4000-0x4FFF------------------------------- */
  // Heater Control Buttons , triged between "cool down" and "heat PLA" state
  constexpr uint16_t VP_E0_CONTROL = 0x4010;
  constexpr uint16_t VP_E1_CONTROL = 0x4012;
  //constexpr uint16_t VP_E2_CONTROL = 0x2214;
  //constexpr uint16_t VP_E3_CONTROL = 0x2216;
  //constexpr uint16_t VP_E4_CONTROL = 0x2218;
  //constexpr uint16_t VP_E5_CONTROL = 0x221A;
  constexpr uint16_t VP_BED_CONTROL = 0x401C;

  // Preheat
  constexpr uint16_t VP_E0_BED_PREHEAT = 0x4020;
  constexpr uint16_t VP_E1_BED_PREHEAT = 0x4022;

  //constexpr uint16_t VP_E2_BED_PREHEAT = 0x4024;
  //constexpr uint16_t VP_E3_BED_PREHEAT = 0x4026;
  //constexpr uint16_t VP_E4_BED_PREHEAT = 0x4028;
  //constexpr uint16_t VP_E5_BED_PREHEAT = 0x402A;

  // Filament load and unload
  // constexpr uint16_t VP_E0_FILAMENT_LOAD_UNLOAD = 0x4030;
  // constexpr uint16_t VP_E1_FILAMENT_LOAD_UNLOAD = 0x4032;

  // Settings store , reset

  // Level data
  // constexpr uint16_t VP_Level_Point_One_X              = 0x4100;
  // constexpr uint16_t VP_Level_Point_One_Y              = 0x4102;
  // constexpr uint16_t VP_Level_Point_Two_X              = 0x4104;
  // constexpr uint16_t VP_Level_Point_Two_Y              = 0x4106;
  // constexpr uint16_t VP_Level_Point_Three_X            = 0x4108;
  // constexpr uint16_t VP_Level_Point_Three_Y            = 0x410A;
  // constexpr uint16_t VP_Level_Point_Four_X             = 0x410C;
  // constexpr uint16_t VP_Level_Point_Four_Y             = 0x410E;
  // constexpr uint16_t VP_Level_Point_Five_X             = 0x4110;
  // constexpr uint16_t VP_Level_Point_Five_Y             = 0x4112;


  /* H43 Version */
  constexpr uint16_t VP_MKS_H43_VERSION                = 0x4A00;   // MKS H43 V1.0.0
  constexpr uint16_t VP_MKS_H43_VERSION_LEN            = 16;
  constexpr uint16_t VP_MKS_H43_UpdataVERSION          = 0x4A10;   // MKS H43 V1.0.0
  constexpr uint16_t VP_MKS_H43_UpdataVERSION_LEN      = 16;

  constexpr uint16_t VP_ZNP_LANGUAGE                = 0x4B00;   // 
  constexpr uint16_t VP_ZNP_LANGUAGE_SAVE           = 0x4B02;
  constexpr uint16_t VP_ZNP_INFI                    = 0x4B20;   // 
  constexpr uint16_t VP_ZNP_TEXT_CHAR				        = 0x4B30;	  //
  constexpr uint16_t VP_ZNP_HEATING_CHAR				    = 0x4B40;	  //32 bytes
  constexpr uint16_t VP_ZNP_HEATING_T_CHAR				    = 0x4B60;	  //2 bytes
  /* -------------------------------0x5000-0xFFFF------------------------------- */
  constexpr uint16_t VP_PRINT_Dis                     = 0x5000;
  constexpr uint16_t VP_PREPARE_Dis                   = 0x5020;
  constexpr uint16_t VP_SETTING_Dis                   = 0x5040;
  constexpr uint16_t VP_LVEVL_Dis                     = 0x5060;

  constexpr uint16_t VP_preheat_Dis                   = 0x5080;
  constexpr uint16_t VP_move_Dis                      = 0x50A0;
  constexpr uint16_t VP_extrude_Dis                   = 0x50C0;
  constexpr uint16_t VP_Filchange_Dis                 = 0x50E0;
  constexpr uint16_t VP_fan_Dis                       = 0x5140;
  constexpr uint16_t VP_More_Dis                      = 0x5100;
  
  constexpr uint16_t VP_About_Dis                     = 0x5200;

  constexpr uint16_t VP_COM_BACK_Dis                  = 0x8300;
  constexpr uint16_t VP_COM_YES_Dis                   = 0x8320;
  constexpr uint16_t VP_COM_NO_Dis                    = 0x8340;
  constexpr uint16_t VP_COM_RP_Dis                    = 0x8360;
  constexpr uint16_t VP_PRINT_COMFIRM_Dis             = 0x8400;
  constexpr uint16_t VP_PRINT_FINISH_Dis              = 0x8420;
  constexpr uint16_t VP_PRINT_FINISH_Dis_1            = 0x8440;



  #if BOTH(MKS_TEST, SDSUPPORT)
  constexpr uint16_t VP_TEST_TEMP1                    = 0x6000;
  constexpr uint16_t VP_TEST_TEMP2                    = 0x6010;
  constexpr uint16_t VP_TEST_TEMP3                    = 0x6020;
  constexpr uint16_t VP_TEST_TEMP4                    = 0x6030;

  #endif

  constexpr uint16_t UI_VERSION_TEXT                  = 0x6050;
  constexpr uint16_t VP_Fan0_MAIN_Percentage          = 0x6070;
#endif // MKS_FINSH
