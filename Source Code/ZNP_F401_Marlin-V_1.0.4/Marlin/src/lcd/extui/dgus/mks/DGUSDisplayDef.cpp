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

#include "../../../../inc/MarlinConfigPre.h"

#if ENABLED(DGUS_LCD_UI_MKS)

#include "DGUSDisplayDef.h"
#include "../DGUSDisplay.h"
#include "../DGUSScreenHandler.h"

#include "../../../../module/temperature.h"
#include "../../../../module/motion.h"
#include "../../../../module/planner.h"

#include "../../ui_api.h"
#include "../../../marlinui.h"

#if HAS_STEALTHCHOP
  #include "../../../../module/stepper/trinamic.h"
#endif

#if ENABLED(POWER_LOSS_RECOVERY)
  #include "../../../../feature/powerloss.h"
#endif

#include "../../../../feature/bedlevel/bedlevel.h"

bool filament_change_flg=false;


#if ENABLED(DGUS_UI_MOVE_DIS_OPTION)
  uint16_t distanceToMove = 10;
#endif
uint16_t manualE0_T_Step = 1;
uint16_t manualHotB_T_Step = 1;


uint16_t FeedratePercentageStep = 1;
uint16_t FlowPercentageStep = 1;
uint16_t FanPercentageStep = 1;

uint16_t manualMoveStep = 2;
uint16_t distanceFilament = 10;
uint16_t filamentSpeed_mm_s = 5;
uint16_t ZOffset_distanceStep = 1;
uint16_t AUTO_ZOffset_distanceStep = 0;
float ZOffset_distance = 10;//0.1;//扩大10倍
float mesh_adj_distance = 0.01;
float Z_distance = 0.1;
//extern bed_mesh_t z_values;

//struct { uint16_t h, m, s; } dgus_time;

//
// Persistent settings
//
xy_int_t mks_corner_offsets[5];   // Initialized by settings.load()
xyz_int_t mks_park_pos;           // Initialized by settings.load()
celsius_t mks_min_extrusion_temp; // Initialized by settings.load()

celsius_t mks_PLA_default_e0_temp; // Initialized by settings.load()
celsius_t mks_PLA_default_bed_temp; // Initialized by settings.load()

celsius_t mks_ABS_default_e0_temp; // Initialized by settings.load()
celsius_t mks_ABS_default_bed_temp; // Initialized by settings.load()

celsius_t mks_AL_default_e0_temp; // Initialized by settings.load()
celsius_t mks_AL_default_bed_temp; // Initialized by settings.load()

uint16_t mks_language=6; // Initialized by settings.load()
uint16_t mks_filament_det_enable = 1; // Initialized by settings.load()
extern uint8_t fan_speed_main_percent;

void MKS_reset_settings() {
  constexpr xy_int_t init_dgus_level_offsets[5] = {
    { 20, 20 }, { 20, 20 },
    { 20, 20 }, { 20, 20 },
    { X_CENTER, Y_CENTER }
  };
  mks_language_index = 0;
  COPY(mks_corner_offsets, init_dgus_level_offsets);
  mks_park_pos.set(5, 5, 5);    //1-------暂停打印头位置
  mks_min_extrusion_temp = 0;
  mks_PLA_default_e0_temp = PREHEAT_1_TEMP_HOTEND;
  mks_PLA_default_bed_temp = PREHEAT_1_TEMP_BED;
  mks_ABS_default_e0_temp = PREHEAT_2_TEMP_HOTEND;
  mks_ABS_default_bed_temp = PREHEAT_2_TEMP_BED; 
  mks_AL_default_e0_temp = PREHEAT_AL_TEMP_HOTEND;
  mks_AL_default_bed_temp = PREHEAT_AL_TEMP_BED; 
  mks_language = 6;    //6:English
  mks_filament_det_enable = 1;
}

xyz_pos_t position_before_pause;
constexpr feedRate_t park_speed_xy = TERN(NOZZLE_PARK_FEATURE, NOZZLE_PARK_XY_FEEDRATE, 100),
                     park_speed_z  = TERN(NOZZLE_PARK_FEATURE, NOZZLE_PARK_Z_FEEDRATE,    5);

void MKS_pause_print_move() {
  queue.exhaust();
  position_before_pause = current_position;

  // Save the current position, the raise amount, and 'already raised'
  TERN_(POWER_LOSS_RECOVERY, if (recovery.enabled) recovery.save(true, mks_park_pos.z, true));

  //1--------//2--------设置暂停回抽
  gcode.process_subcommands_now(PSTR("M108"));
  gcode.process_subcommands_now(PSTR("M83"));
  gcode.process_subcommands_now(PSTR("G1 E-10 F600"));
  gcode.process_subcommands_now(PSTR("G90"));



  destination.z = _MIN(current_position.z + mks_park_pos.z, Z_MAX_POS);
  prepare_internal_move_to_destination(park_speed_z);

  destination.set(X_MIN_POS + mks_park_pos.x, Y_MIN_POS + mks_park_pos.y);
  prepare_internal_move_to_destination(park_speed_xy);

  filament_change_flg = true;
}

void MKS_resume_print_move() {
  //2--------避免客户更换材料插到底的情况
  gcode.process_subcommands_now(PSTR("M108"));
  gcode.process_subcommands_now(PSTR("M83"));
  gcode.process_subcommands_now(PSTR("G1 E9 F150"));
  gcode.process_subcommands_now(PSTR("G1 E-9 F600"));
  gcode.process_subcommands_now(PSTR("G90"));


  destination.set(position_before_pause.x, position_before_pause.y);
  prepare_internal_move_to_destination(park_speed_xy);
  destination.z = position_before_pause.z;
  prepare_internal_move_to_destination(park_speed_z);

  //1--------设置暂停回抽后恢复
  gcode.process_subcommands_now(PSTR("M108"));
  gcode.process_subcommands_now(PSTR("M83"));
  gcode.process_subcommands_now(PSTR("G1 E10 F500"));
  gcode.process_subcommands_now(PSTR("G1 F2000"));//2--------避免花瓶模式速度缓慢
  gcode.process_subcommands_now(PSTR("G90"));


  TERN_(POWER_LOSS_RECOVERY, if (recovery.enabled) recovery.save(true));
}

float z_offset_add = 0;

xyz_int_t tmc_step; // = { 0, 0, 0 }

uint16_t lcd_default_light = 50;

EX_FILAMENT_DEF ex_filament;
RUNOUT_MKS_DEF runout_mks;
NOZZLE_PARK_DEF nozzle_park_mks;

const uint16_t VPList_Boot[] PROGMEM = {
  VP_MARLIN_VERSION,
  0x0000
};

#define MKSLIST_E_ITEM(N) VP_T_E##N##_Is, VP_T_E##N##_Set,

const uint16_t VPList_Main[] PROGMEM = {
  // VP_M117, for completeness, but it cannot be auto-uploaded.
  #if HAS_HOTEND
    MKSLIST_E_ITEM(0) VP_E0_STATUS,
    #if HAS_MULTI_HOTEND
      MKSLIST_E_ITEM(1) VP_E1_STATUS,
    #endif
  #endif
  #if HAS_HEATED_BED
    VP_T_Bed_Is, VP_T_Bed_Set, VP_BED_STATUS,
  #endif
  #if HAS_FAN
    VP_Fan0_Percentage, 
    VP_FAN0_STATUS,
    VP_Fan0_MAIN_Percentage,
  #endif
  VP_XPos, VP_YPos, VP_ZPos,
  VP_Fan0_Percentage,
  VP_Feedrate_Percentage,
  VP_Fan0_MAIN_Percentage,
  #if ENABLED(LCD_SET_PROGRESS_MANUALLY)
    VP_PrintProgress_Percentage,
  #endif
  0x0000
};

const uint16_t MKSList_Home[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,
  // FAN
  VP_Fan0_Percentage,
  VP_Fan0_MAIN_Percentage,

  VP_ZPos,
  // Language
  // VP_HOME_Dis,

  0x0000
};

const uint16_t MKSList_Setting[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,
  // FAN
  VP_Fan0_Percentage,
  VP_Fan0_MAIN_Percentage,
  // Language

  0x0000
};

const uint16_t MKSList_Tool[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,
  // FAN
  VP_Fan0_Percentage,
  VP_Fan0_MAIN_Percentage,
  // Language
  
  // LCD BLK
  VP_LCD_BLK,
  0x0000
};

const uint16_t MKSList_EXTRUE[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,
  // FAN
  VP_Fan0_Percentage,
  VP_Fan0_MAIN_Percentage,

  VP_Filament_distance,
  VP_Filament_speed,
  0x0000
};

const uint16_t MKSList_LEVEL[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,
  // FAN
  VP_Fan0_Percentage,
  VP_Fan0_MAIN_Percentage,

  0x0000
};

const uint16_t MKSList_MOVE[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,
  // FAN
  VP_Fan0_Percentage,
  VP_Fan0_MAIN_Percentage,
  0x0000
};

const uint16_t MKSList_Print[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,
  // FAN
  VP_Fan0_Percentage,
  VP_Fan0_MAIN_Percentage,
  // Print Percent
  VP_PrintProgress_Percentage,
  
  VP_PrintTime,

  VP_Flowrate_E0,
  //VP_Flowrate_E1,
  VP_Feedrate_Percentage,

  VP_PrintTime_H,
  VP_PrintTime_M,
  VP_PrintTime_S,
  VP_SD_read_err,

  //VP_XPos,
  //VP_YPos,
  VP_ZPos,

  0x0000
};
const uint16_t MKSList_PrintPause[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,
  // FAN
  VP_Fan0_Percentage,
  VP_Fan0_MAIN_Percentage,
  // Print Percent
  VP_PrintProgress_Percentage_1,
  
  VP_PrintTime,

  VP_Flowrate_E0,
  //VP_Flowrate_E1,
  VP_Feedrate_Percentage,

  VP_PrintTime_H,
  VP_PrintTime_M,
  VP_PrintTime_S,

  //VP_XPos,
  //VP_YPos,
  VP_ZPos,

  0x0000
};
const uint16_t MKSList_SD_File[] PROGMEM = {
  VP_SD_FileName0, VP_SD_FileName1,
  VP_SD_FileName2, VP_SD_FileName3,
  VP_SD_FileName4, VP_SD_FileName5,
  VP_SD_FileName6, VP_SD_FileName7,
  VP_SD_FileName8, VP_SD_FileName9,

  0x0000
};

const uint16_t MKSList_TempOnly[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,
  // FAN
  VP_Fan0_Percentage,
  VP_Fan0_MAIN_Percentage,
  // LCD BLK
  VP_LCD_BLK,
  0x0000
};

const uint16_t MKSList_Pluse[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,
  // FAN
  VP_Fan0_Percentage,
  VP_Fan0_MAIN_Percentage,

  // Pluse
  VP_X_STEP_PER_MM,
  VP_Y_STEP_PER_MM,
  VP_Z_STEP_PER_MM,
  VP_E0_STEP_PER_MM,
  VP_E1_STEP_PER_MM,

  0x0000
};

const uint16_t MKSList_MaxSpeed[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,
  // FAN
  VP_Fan0_Percentage,
  VP_Fan0_MAIN_Percentage,

  // Pluse
  VP_X_MAX_SPEED,
  VP_Y_MAX_SPEED,
  VP_Z_MAX_SPEED,
  VP_E0_MAX_SPEED,
  VP_E1_MAX_SPEED,

  0x0000
};

const uint16_t MKSList_MaxAcc[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,
  // FAN
  VP_Fan0_Percentage,
  VP_Fan0_MAIN_Percentage,

  // ACC
  VP_ACC_SPEED,
  VP_X_ACC_MAX_SPEED,
  VP_Y_ACC_MAX_SPEED,
  VP_Z_ACC_MAX_SPEED,
  VP_E0_ACC_MAX_SPEED,
  VP_E1_ACC_MAX_SPEED,

  0x0000
};

const uint16_t MKSList_PID[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,
  // FAN
  VP_Fan0_Percentage,
  VP_Fan0_MAIN_Percentage,

  // PID
  VP_E0_PID_P,
  VP_E0_PID_I,
  VP_E0_PID_D,

  0x0000
};

const uint16_t MKSList_Level_Point[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,
  // FAN
  VP_Fan0_Percentage,
  VP_Fan0_MAIN_Percentage,

  // Level Point
  // VP_Level_Point_One_X,
  // VP_Level_Point_One_Y,
  // VP_Level_Point_Two_X,
  // VP_Level_Point_Two_Y,
  // VP_Level_Point_Three_X,
  // VP_Level_Point_Three_Y,
  // VP_Level_Point_Four_X,
  // VP_Level_Point_Four_Y,
  // VP_Level_Point_Five_X,
  // VP_Level_Point_Five_Y,

  0x0000
};

const uint16_t MKSList_Level_PrintConfig[] PROGMEM = {
  VP_T_E0_Set,
  VP_T_E1_Set,
  VP_T_Bed_Set,
  VP_Flowrate_E0,
  VP_Flowrate_E1,
  VP_Fan0_Percentage,
  VP_Fan0_MAIN_Percentage,
  VP_Feedrate_Percentage,

  0x0000
};

const uint16_t MKSList_PrintPauseConfig[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,

  VP_X_PARK_POS,
  VP_Y_PARK_POS,
  VP_Z_PARK_POS,

  0x0000
};

const uint16_t MKSList_MotoConfig[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,

  VP_TRAVEL_SPEED,
  VP_FEEDRATE_MIN_SPEED,
  VP_T_F_SPEED,

  0x0000
};

const uint16_t MKSList_EX_Config[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,
  VP_MIN_EX_T,VP_Min_EX_T_E,
  0x0000
};

const uint16_t MKSTMC_Config[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,
  VP_MIN_EX_T,

  VP_TMC_X_STEP,
  VP_TMC_Y_STEP,
  VP_TMC_Z_STEP,
  VP_TMC_X1_Current,
  VP_TMC_Y1_Current,
  VP_TMC_X_Current,
  VP_TMC_Y_Current,
  VP_TMC_Z_Current,
  VP_TMC_E0_Current,
  VP_TMC_E1_Current,
  VP_TMC_Z1_Current,

  0x0000
};

const uint16_t MKSAuto_Level[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,
  VP_MESH_LEVEL_POINT_DIS,
  //VP_ZPos,
  VP_LEVELING_POINT_OFFSET1,
  VP_LEVELING_POINT_OFFSET2,
  VP_LEVELING_POINT_OFFSET3,
  VP_LEVELING_POINT_OFFSET4,
  VP_LEVELING_POINT_OFFSET5,
  VP_LEVELING_POINT_OFFSET6,
  VP_LEVELING_POINT_OFFSET7,
  VP_LEVELING_POINT_OFFSET8,
  VP_LEVELING_POINT_OFFSET9,
  VP_LEVELING_POINT_OFFSET10,
  VP_LEVELING_POINT_OFFSET11,
  VP_LEVELING_POINT_OFFSET12,
  VP_LEVELING_POINT_OFFSET13,
  VP_LEVELING_POINT_OFFSET14,
  VP_LEVELING_POINT_OFFSET15,
  VP_LEVELING_POINT_OFFSET16,      
  VP_OFFSET_Z,
  0x0000
};

const uint16_t MKSOffset_Config[] PROGMEM = {
  // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  VP_OFFSET_X,
  VP_OFFSET_Y,
  VP_OFFSET_Z,
  0x0000
};

const uint16_t MKSBabyStep[] PROGMEM = {
  VP_ZOffset_DE_DIS,
  0x0000
};

const uint16_t MKPreheat[] PROGMEM = {
  VP_T_E0_Is,
  VP_T_E0_Set, 
  VP_T_Bed_Is, 
  VP_T_Bed_Set,
  0x0000
};

const uint16_t MKSFilchange[] PROGMEM = {
  VP_T_E0_Is,
  VP_T_E0_Set, 
  VP_Filament_distance,
  VP_Filament_speed,
  0x0000
};


const uint16_t MKSList_About[] PROGMEM = {
  // Marlin version
  VP_MARLIN_VERSION,
  // H43 Version
  VP_MKS_H43_VERSION,
  VP_MKS_H43_UpdataVERSION,
  UI_VERSION_TEXT,
  0x0000
};

const uint16_t MKSPrintStopPopup[] PROGMEM = {
   // E Temp
  REPEAT(EXTRUDERS, MKSLIST_E_ITEM)
  // HB Temp
  VP_T_Bed_Is, VP_T_Bed_Set,
  // FAN
  VP_Fan0_Percentage,
  VP_Fan0_MAIN_Percentage,
  // Print Percent
  //VP_PrintProgress_Percentage,

  VP_PrintTime,

  VP_PrintTime_H,
  VP_PrintTime_M,
  VP_PrintTime_S,

  VP_Fan0_Percentage,
  VP_Fan0_MAIN_Percentage,
  VP_Feedrate_Percentage,

  VP_ZPos,

  VP_SD_read_err,

  0x0000
};

const uint16_t MKSFanCTRL[] PROGMEM = {
  VP_Fan0_Percentage,
  VP_Fan0_MAIN_Percentage,
  VP_SD_read_err,
  0x0000
};
const uint16_t MKSFeedrate[] PROGMEM = {
  VP_Feedrate_Percentage,
  VP_SD_read_err,
  0x0000
};
const uint16_t MKSFlow[] PROGMEM = {
  VP_Flowrate_E0,
  VP_SD_read_err,
  0x0000
};

const uint16_t MKSList_FILAMET_HOTB_T[] PROGMEM = {
  VP_T_E0_Set,
  VP_T_Bed_Set,	
  VP_SD_read_err,
  0x0000
};
const uint16_t MKSList_FILAMET_E_T[] PROGMEM = {
  VP_T_E0_Set,
  VP_T_Bed_Set,	
  VP_SD_read_err,
  0x0000
};
const uint16_t MKSList_PrintConfirm[] = {
  VP_SD_Print_Filename,
  0x0000
};
const uint16_t MKSList_DEFAULT_PLA[] ={
	VP_DEFAULT_PLA_T_TEMP,
	VP_DEFAULT_PLA_B_TEMP,
	0x0000
};
const uint16_t MKSList_DEFAULT_ABS[] ={
	VP_DEFAULT_ABS_T_TEMP,
	VP_DEFAULT_ABS_B_TEMP,
	0x0000
};
const uint16_t MKSList_DEFAULT_AL[] ={
	VP_DEFAULT_AL_T_TEMP,
	VP_DEFAULT_AL_B_TEMP,
	0x0000
};


const uint16_t MKSheating[] PROGMEM = {
  VP_T_E0_Is,
  VP_T_E0_Set, 
  0x0000
};

const uint16_t MKSTest[] PROGMEM = {
  VP_T_E0_Is,
  VP_T_Bed_Is, 
  // VP_TEST_TEMP1,
  // VP_TEST_TEMP2,
  VP_TEST_TEMP3,
  VP_TEST_TEMP4,
  //VP_TEST_TEMP5,
  0x0000
};
// Page data updata
const struct VPMapping VPMap[] PROGMEM = {
  { MKSLCD_SCREEN_BOOT, VPList_Boot },                        // Boot Page to show logo  0
  { MKSLCD_SCREEN_HOME, MKSList_Home },                       // Home, Page 1
  { MKSLCD_SCREEN_SETTING, MKSList_Setting },                 // Setting, Page 2
  { MKSLCD_SCREEM_TOOL, MKSList_Tool },                       
  { MKSLCD_SCREEN_EXTRUDE_P1, MKSList_EXTRUE },               
  { MKSLCD_SCREEN_EXTRUDE_P2, MKSList_EXTRUE },               
  // { MKSLCD_PAUSE_SETTING_EX, MKSList_EXTRUE },                
  // { MKSLCD_PAUSE_SETTING_EX2, MKSList_EXTRUE },               
  { MKSLCD_SCREEN_LEVEL, MKSList_LEVEL },                     
  { MKSLCD_SCREEN_MOVE, MKSList_MOVE },                       
  { MKSLCD_SCREEN_PRINT, MKSList_Print },                     
  { MKSLCD_SCREEN_PAUSE, MKSList_PrintPause },                     
  { MKSLCD_SCREEN_CHOOSE_FILE, MKSList_SD_File },             
  //{ MKSLCD_SCREEN_MOTOR_PLUSE, MKSList_Pluse },               
  //{ MKSLCD_SCREEN_MOTOR_SPEED, MKSList_MaxSpeed },            
  //{ MKSLCD_SCREEN_MOTOR_ACC_MAX, MKSList_MaxAcc },            
  //{ MKSLCD_SCREEN_LEVEL_DATA, MKSList_Level_Point },          
  //{ MKSLCD_PrintPause_SET, MKSList_PrintPauseConfig },        
  //{ MKSLCD_FILAMENT_DATA, MKSList_SD_File },                  
  //{ MKSLCD_SCREEN_Config, MKSList_TempOnly },                 
  //{ MKSLCD_SCREEN_Config_MOTOR, MKSList_MotoConfig },         
  //{ MKSLCD_PID, MKSList_PID },                                
  { MKSLCD_ABOUT, MKSList_About },                            
  //{ MKSLCD_SCREEN_PRINT_CONFIG, MKSList_Level_PrintConfig },  
  //{ MKSLCD_SCREEN_EX_CONFIG, MKSList_EX_Config },             
  //{ MKSLCD_SCREEN_TMC_Config, MKSTMC_Config },                
  { MKSLCD_AUTO_LEVEL, MKSAuto_Level },      
  { MKSLCD_Screen_AUTO_LEVEL_POPUP1, MKSAuto_Level },                      
  //{ MKSLCD_Screen_Offset_Config, MKSOffset_Config },          
  //{ MKSLCD_Screen_PMove, MKSList_MOVE },                      
  { MKSLCD_Screen_Baby, MKSBabyStep },    
  { DGUSLCD_SCREEN_FAN_CTRL, MKSFanCTRL },   

  {MKSLCD_Screen_PRINT_SPEED,MKSFeedrate},
  {MKSLCD_Screen_FLOW_SPEED,MKSFlow},
  {MKSLCD_Screen_FAN_SPEED,MKSFanCTRL},

  {MKSLCD_Screen_PRINT_FLAMENT,MKSList_FILAMET_E_T},
  {MKSLCD_Screen_PRINT_FLAMENT_HOTB,MKSList_FILAMET_HOTB_T},
  {MKS_LCD_PREHEAT_DEFAULT_PLA,MKSList_DEFAULT_PLA},
  {MKS_LCD_PREHEAT_DEFAULT_ABS,MKSList_DEFAULT_ABS},
  {MKS_LCD_PREHEAT_DEFAULT_AL,MKSList_DEFAULT_AL},
  
  { MKSLCD_Screen_EXTRUDE_HEATING_POPUP, MKSheating },
  { MKS_LCD_PREHEAT, MKPreheat },                       
  { MKSLCD_SCREEN_FILCHANGE, MKSFilchange }, 
  { MKSLCD_SCREEN_PRINT_STAT_STOP_POPUP, MKSPrintStopPopup },
  { MKSLCD_SCREEN_PRINT_PASU_STOP_POPUP, MKSPrintStopPopup },                    
  //{ MKSLCD_SCREEN_PRINT_CONFIRM,MKSList_PrintConfirm},
  {MKSLCD_SCREEN_TEST, MKSTest},
  { 0, nullptr } // List is terminated with an nullptr as table entry.
};

char UI_Version_Text[] = UI_VERSION;

const char MarlinVersion[] PROGMEM = SHORT_BUILD_VERSION;
const char H43Version[] PROGMEM = "ZNP_ROBIN_NANO_DW V2.0";
const char Updata_Time[] PROGMEM = STRING_DISTRIBUTION_DATE;

const struct DGUS_VP_Variable ListOfVP[] PROGMEM = {
  // Helper to detect touch events
  VPHELPER(VP_SCREENCHANGE, nullptr, ScreenHandler.ScreenChangeHook, nullptr),
  VPHELPER(VP_SCREENCHANGE_ASK, nullptr, ScreenHandler.ScreenChangeHookIfIdle, nullptr),
  #if ENABLED(SDSUPPORT)
    VPHELPER(VP_SCREENCHANGE_WHENSD, nullptr, ScreenHandler.ScreenChangeHookIfSD, nullptr),
  #endif
  VPHELPER(VP_CONFIRMED, nullptr, ScreenHandler.ScreenConfirmedOK, nullptr),

  // Back Button
  VPHELPER(VP_BACK_PAGE, nullptr, ScreenHandler.ScreenBackChange, nullptr),
  VPHELPER(VP_TEMP_ALL_OFF, nullptr, ScreenHandler.HandleAllHeatersOff, nullptr),

  VPHELPER(VP_MOVE_X, nullptr, ScreenHandler.HandleManualMove, nullptr),
  VPHELPER(VP_MOVE_Y, nullptr, ScreenHandler.HandleManualMove, nullptr),
  VPHELPER(VP_MOVE_Z, nullptr, ScreenHandler.HandleManualMove, nullptr),
  VPHELPER(VP_HOME_ALL, nullptr, ScreenHandler.HandleManualMove, nullptr),

  VPHELPER(VP_X_HOME, nullptr, ScreenHandler.HandleManualMove, nullptr),
  VPHELPER(VP_Y_HOME, nullptr, ScreenHandler.HandleManualMove, nullptr),
  VPHELPER(VP_Z_HOME, nullptr, ScreenHandler.HandleManualMove, nullptr),

  VPHELPER(VP_MOVE_DISTANCE, &manualMoveStep, ScreenHandler.GetManualMovestep, nullptr),

  VPHELPER(VP_MOTOR_LOCK_UNLOK, nullptr, ScreenHandler.HandleManualMove, nullptr),
  VPHELPER(VP_LEVEL_POINT, nullptr, ScreenHandler.ManualAssistLeveling, nullptr),

  #if ENABLED(POWER_LOSS_RECOVERY)
    VPHELPER(VP_POWER_LOSS_RECOVERY, nullptr, ScreenHandler.HandlePowerLossRecovery, nullptr),
  #endif
  VPHELPER(VP_SETTINGS, nullptr, ScreenHandler.HandleSettings, nullptr),
  #if ENABLED(SINGLE_Z_CALIBRATION)
    VPHELPER(VP_Z_CALIBRATE, nullptr, ScreenHandler.HandleZCalibration, nullptr),
  #endif
  #if ENABLED(FIRST_LAYER_CAL)
    VPHELPER(VP_Z_FIRST_LAYER_CAL, nullptr, ScreenHandler.HandleFirstLayerCal, nullptr),
  #endif
  {.VP = VP_MARLIN_VERSION, .memadr = (void *)MarlinVersion, .size = VP_MARLIN_VERSION_LEN, .set_by_display_handler = nullptr, .send_to_display_handler = ScreenHandler.DGUSLCD_SendStringToDisplayPGM},
  // M117 LCD String (We don't need the string in memory but "just" push it to the display on demand, hence the nullptr
  {.VP = VP_M117, .memadr = nullptr, .size = VP_M117_LEN, .set_by_display_handler = nullptr, .send_to_display_handler = ScreenHandler.DGUSLCD_SendStringToDisplay},
  {.VP = VP_MKS_H43_VERSION, .memadr = (void *)H43Version, .size = VP_MKS_H43_VERSION_LEN, .set_by_display_handler = nullptr, .send_to_display_handler = ScreenHandler.DGUSLCD_SendStringToDisplayPGM},
  {.VP = VP_MKS_H43_UpdataVERSION, .memadr = (void *)Updata_Time, .size = VP_MKS_H43_VERSION_LEN, .set_by_display_handler = nullptr, .send_to_display_handler = ScreenHandler.DGUSLCD_SendStringToDisplayPGM},

  // Temperature Data
  #if HAS_HOTEND
    VPHELPER(VP_T_E0_Is, &thermalManager.temp_hotend[0].celsius, nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<0>),
    VPHELPER(VP_T_E0_Set, &thermalManager.temp_hotend[0].target, ScreenHandler.HandleTemperatureChanged, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
	  
    VPHELPER(VP_T_E0_Step, &manualE0_T_Step, ScreenHandler.GetManualE0_T_step, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
    VPHELPER(VP_T_E0_Adjust, nullptr, ScreenHandler.HandleManualE0_T, nullptr),
    VPHELPER(VP_T_Bed_Step, &manualHotB_T_Step, ScreenHandler.GetManualHotB_T_step, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
    VPHELPER(VP_T_Bed_Adjust, nullptr, ScreenHandler.HandleManualHotB_T, nullptr),
	  
    VPHELPER(VP_Flowrate_E0_Step,&FlowPercentageStep, ScreenHandler.GetFlowRateStep, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
    VPHELPER(VP_Flowrate_E0_Adjust,&planner.flow_percentage[ExtUI::extruder_t::E0], ScreenHandler.HandleFlowRateChanged,ScreenHandler.DGUSLCD_SendWordValueToDisplay),
    VPHELPER(VP_Flowrate_E0, &planner.flow_percentage[ExtUI::extruder_t::E0], nullptr, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
    
    VPHELPER(VP_EPos, &destination.e, nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
    VPHELPER(VP_MOVE_E0, nullptr, ScreenHandler.HandleManualExtrude, nullptr),
    VPHELPER(VP_E0_CONTROL, &thermalManager.temp_hotend[0].target, ScreenHandler.HandleHeaterControl, nullptr),
    VPHELPER(VP_E0_STATUS, &thermalManager.temp_hotend[0].target, nullptr, ScreenHandler.DGUSLCD_SendHeaterStatusToDisplay),
    #if ENABLED(DGUS_PREHEAT_UI)
      VPHELPER(VP_E0_BED_PREHEAT, nullptr, ScreenHandler.HandlePreheat, nullptr),
    #endif
    #if ENABLED(PIDTEMP)
      VPHELPER(VP_E0_PID_P, &thermalManager.temp_hotend[0].pid.Kp, ScreenHandler.HandleTemperaturePIDChanged, ScreenHandler.DGUSLCD_SendTemperaturePID),
      VPHELPER(VP_E0_PID_I, &thermalManager.temp_hotend[0].pid.Ki, ScreenHandler.HandleTemperaturePIDChanged, ScreenHandler.DGUSLCD_SendTemperaturePID),
      VPHELPER(VP_E0_PID_D, &thermalManager.temp_hotend[0].pid.Kd, ScreenHandler.HandleTemperaturePIDChanged, ScreenHandler.DGUSLCD_SendTemperaturePID),
      VPHELPER(VP_PID_AUTOTUNE_E0, nullptr, ScreenHandler.HandlePIDAutotune, nullptr),
      VPHELPER(UI_VERSION_TEXT, UI_Version_Text, nullptr, ScreenHandler.DGUSLCD_SendStringToDisplay),
    #endif
    #if ENABLED(DGUS_FILAMENT_LOADUNLOAD)
      VPHELPER(VP_LOAD_Filament, nullptr, ScreenHandler.MKS_FilamentLoad, nullptr),
      VPHELPER(VP_UNLOAD_Filament, nullptr, ScreenHandler.MKS_FilamentUnLoad, nullptr),
      VPHELPER(VP_Filament_distance, &distanceFilament, ScreenHandler.GetManualFilament, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
      VPHELPER(VP_Filament_speed, &filamentSpeed_mm_s, ScreenHandler.GetManualFilamentSpeed, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
      
      VPHELPER(VP_Extrude_Cancel_heating, nullptr, ScreenHandler.MKS_FilamentCancelHeating, nullptr),
      VPHELPER(VP_Extrude_Load, nullptr, ScreenHandler.MKS_Extrude_load_popup, nullptr),
      VPHELPER(VP_Extrude_Unload, nullptr, ScreenHandler.MKS_Extrude_unload_popup, nullptr),
    #endif
  #endif 
  //during print change filament
  VPHELPER(VP_E0_Print_Filament_load, nullptr, ScreenHandler.MKS_PrintFilamentLoad, nullptr),
  VPHELPER(VP_E0_Print_Filament_unload, nullptr, ScreenHandler.MKS_PrintFilamentUnLoad, nullptr),

  VPHELPER(VP_E0_Print_Filament_load_confirm, nullptr, ScreenHandler.MKS_PrintFilamentLoad_Confirm, nullptr),
  VPHELPER(VP_E0_Print_Filament_unload_confirm, nullptr, ScreenHandler.MKS_PrintFilamentUnLoad_Confirm, nullptr),
  #if HAS_MULTI_HOTEND
    VPHELPER(VP_T_E1_Is, &thermalManager.temp_hotend[1].celsius, nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<0>),
    VPHELPER(VP_T_E1_Set, &thermalManager.temp_hotend[1].target, ScreenHandler.HandleTemperatureChanged, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
    VPHELPER(VP_Flowrate_E1, &planner.flow_percentage[ExtUI::extruder_t::E1], ScreenHandler.HandleFlowRateChanged, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
    VPHELPER(VP_MOVE_E1, nullptr, ScreenHandler.HandleManualExtrude, nullptr),
    VPHELPER(VP_E1_CONTROL, &thermalManager.temp_hotend[1].target, ScreenHandler.HandleHeaterControl, nullptr),
    VPHELPER(VP_E1_STATUS, &thermalManager.temp_hotend[1].target, nullptr, ScreenHandler.DGUSLCD_SendHeaterStatusToDisplay),

    #if ENABLED(DGUS_FILAMENT_LOADUNLOAD)
      VPHELPER(VP_Filament_distance, &distanceFilament, ScreenHandler.GetManualFilament, ScreenHandler.DGUSLCD_SendFloatAsIntValueToDisplay<0>),
      VPHELPER(VP_Filament_speed, &filamentSpeed_mm_s, ScreenHandler.GetManualFilamentSpeed, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
    #endif

    #if ENABLED(PIDTEMP)
      VPHELPER(VP_PID_AUTOTUNE_E1, nullptr, ScreenHandler.HandlePIDAutotune, nullptr),
    #endif

    VPHELPER(VP_E1_FILAMENT_LOAD_UNLOAD, nullptr, ScreenHandler.HandleFilamentOption, ScreenHandler.HandleFilamentLoadUnload),
  #endif

  #if HAS_HEATED_BED
    VPHELPER(VP_T_Bed_Is, &thermalManager.temp_bed.celsius, nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<0>),
    VPHELPER(VP_T_Bed_Set, &thermalManager.temp_bed.target, ScreenHandler.HandleTemperatureChanged, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
    VPHELPER(VP_BED_CONTROL, &thermalManager.temp_bed.target, ScreenHandler.HandleHeaterControl, nullptr),
    VPHELPER(VP_BED_STATUS, &thermalManager.temp_bed.target, nullptr, ScreenHandler.DGUSLCD_SendHeaterStatusToDisplay),
    #if ENABLED(PIDTEMPBED)
      VPHELPER(VP_BED_PID_P, &thermalManager.temp_bed.pid.Kp, ScreenHandler.HandleTemperaturePIDChanged, ScreenHandler.DGUSLCD_SendTemperaturePID),
      VPHELPER(VP_BED_PID_I, &thermalManager.temp_bed.pid.Ki, ScreenHandler.HandleTemperaturePIDChanged, ScreenHandler.DGUSLCD_SendTemperaturePID),
      VPHELPER(VP_BED_PID_D, &thermalManager.temp_bed.pid.Kd, ScreenHandler.HandleTemperaturePIDChanged, ScreenHandler.DGUSLCD_SendTemperaturePID),
      VPHELPER(VP_PID_AUTOTUNE_BED, nullptr, ScreenHandler.HandlePIDAutotune, nullptr),
    #endif
  #endif

  // Fan Data
  #if HAS_FAN
  #if 0
    #define FAN_VPHELPER(N)                                                                                                                    \
     VPHELPER(VP_Fan##N##_Percentage, &thermalManager.fan_speed[N], ScreenHandler.DGUSLCD_SetUint8, ScreenHandler.DGUSLCD_SendFanToDisplay), \
     VPHELPER(VP_FAN##N##_CONTROL, &thermalManager.fan_speed[N], ScreenHandler.HandleFanControl, nullptr),                               \
     VPHELPER(VP_FAN##N##_STATUS, &thermalManager.fan_speed[N], nullptr, ScreenHandler.DGUSLCD_SendFanStatusToDisplay),
    REPEAT(FAN_COUNT, FAN_VPHELPER)
  #endif
  #endif
   VPHELPER(VP_Fan0_Percentage_step,&FanPercentageStep,ScreenHandler.GetFanPercentageStep, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
   VPHELPER(VP_Fan0_Percentage_Adjust,&thermalManager.fan_speed[0],ScreenHandler.HandleFanPercentageAdjust,ScreenHandler.DGUSLCD_SendFanToDisplay),
   VPHELPER(VP_Fan0_Percentage, &thermalManager.fan_speed[0], nullptr, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
   VPHELPER(VP_Fan0_MAIN_Percentage, &fan_speed_main_percent, nullptr, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  //  VPHELPER(VP_Fan0_Percentage, &feedrate_percentage, nullptr, ScreenHandler.DGUSLCD_SendWordValueToDisplay),

  // Feedrate
   VPHELPER(VP_Feedrate_Percentage_step,&FeedratePercentageStep,ScreenHandler.GetFeedratePercentageStep, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
   VPHELPER(VP_Feedrate_Percentage_Adjust,&feedrate_percentage,ScreenHandler.HandleFeedratePercentageAdjust, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
   VPHELPER(VP_Feedrate_Percentage, &feedrate_percentage, nullptr, ScreenHandler.DGUSLCD_SendWordValueToDisplay),

  VPHELPER(VP_PLA_ABS_SET, nullptr, ScreenHandler.HandleSetPLAorABS_MKS, nullptr),
  VPHELPER(VP_DEFAULT_PLA_T_TEMP,&mks_PLA_default_e0_temp,ScreenHandler.GetDefault_Temp_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  VPHELPER(VP_DEFAULT_PLA_B_TEMP,&mks_PLA_default_bed_temp,ScreenHandler.GetDefault_Temp_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  VPHELPER(VP_DEFAULT_ABS_T_TEMP,&mks_ABS_default_e0_temp,ScreenHandler.GetDefault_Temp_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  VPHELPER(VP_DEFAULT_ABS_B_TEMP,&mks_ABS_default_bed_temp,ScreenHandler.GetDefault_Temp_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  VPHELPER(VP_DEFAULT_AL_T_TEMP,&mks_AL_default_e0_temp,ScreenHandler.GetDefault_Temp_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  VPHELPER(VP_DEFAULT_AL_B_TEMP,&mks_AL_default_bed_temp,ScreenHandler.GetDefault_Temp_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  VPHELPER(VP_DEFAULT_TEMP_SAVE,nullptr,ScreenHandler.HandleSetPLAorABS_SAVE,nullptr),

  VPHELPER(VP_ZNP_LANGUAGE,&mks_language,ScreenHandler.GetLanguage_MKS,ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  VPHELPER(VP_ZNP_LANGUAGE_SAVE,nullptr,ScreenHandler.HandleLanguage_SAVE,nullptr),

  VPHELPER(VP_Fan0_Percentage_pic, nullptr, ScreenHandler.HandleSetFan_MKS, nullptr),

  // Position Data
  VPHELPER(VP_XPos, &current_position.x, nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
  VPHELPER(VP_YPos, &current_position.y, nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
  VPHELPER(VP_ZPos, &current_position.z, nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),

  // Level Point Set
  // VPHELPER(VP_Level_Point_One_X, &mks_corner_offsets[0].x, ScreenHandler.HandleChangeLevelPoint_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  // VPHELPER(VP_Level_Point_One_Y, &mks_corner_offsets[0].y, ScreenHandler.HandleChangeLevelPoint_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  // VPHELPER(VP_Level_Point_Two_X, &mks_corner_offsets[1].x, ScreenHandler.HandleChangeLevelPoint_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  // VPHELPER(VP_Level_Point_Two_Y, &mks_corner_offsets[1].y, ScreenHandler.HandleChangeLevelPoint_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  // VPHELPER(VP_Level_Point_Three_X, &mks_corner_offsets[2].x, ScreenHandler.HandleChangeLevelPoint_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  // VPHELPER(VP_Level_Point_Three_Y, &mks_corner_offsets[2].y, ScreenHandler.HandleChangeLevelPoint_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  // VPHELPER(VP_Level_Point_Four_X, &mks_corner_offsets[3].x, ScreenHandler.HandleChangeLevelPoint_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  // VPHELPER(VP_Level_Point_Four_Y, &mks_corner_offsets[3].y, ScreenHandler.HandleChangeLevelPoint_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  // VPHELPER(VP_Level_Point_Five_X, &mks_corner_offsets[4].x, ScreenHandler.HandleChangeLevelPoint_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  // VPHELPER(VP_Level_Point_Five_Y, &mks_corner_offsets[4].y, ScreenHandler.HandleChangeLevelPoint_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),

  VPHELPER(VP_LEVELING_POINT_OFFSET1, &z_values[0][0], nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
  VPHELPER(VP_LEVELING_POINT_OFFSET2, &z_values[0][1], nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
  VPHELPER(VP_LEVELING_POINT_OFFSET3, &z_values[0][2], nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
  VPHELPER(VP_LEVELING_POINT_OFFSET4, &z_values[0][3], nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
  VPHELPER(VP_LEVELING_POINT_OFFSET5, &z_values[1][0], nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
  VPHELPER(VP_LEVELING_POINT_OFFSET6, &z_values[1][1], nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
  VPHELPER(VP_LEVELING_POINT_OFFSET7, &z_values[1][2], nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
  VPHELPER(VP_LEVELING_POINT_OFFSET8, &z_values[1][3], nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
  VPHELPER(VP_LEVELING_POINT_OFFSET9, &z_values[2][0], nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
  VPHELPER(VP_LEVELING_POINT_OFFSET10, &z_values[2][1], nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
  VPHELPER(VP_LEVELING_POINT_OFFSET11, &z_values[2][2], nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
  VPHELPER(VP_LEVELING_POINT_OFFSET12, &z_values[2][3], nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
  VPHELPER(VP_LEVELING_POINT_OFFSET13, &z_values[3][0], nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
  VPHELPER(VP_LEVELING_POINT_OFFSET14, &z_values[3][1], nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
  VPHELPER(VP_LEVELING_POINT_OFFSET15, &z_values[3][2], nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
  VPHELPER(VP_LEVELING_POINT_OFFSET16, &z_values[3][3], nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),  

  // Print Progress
  VPHELPER(VP_PrintProgress_Percentage, nullptr, nullptr, ScreenHandler.DGUSLCD_SendPrintProgressToDisplay),
  VPHELPER(VP_PrintProgress_Percentage_1, nullptr, nullptr, ScreenHandler.DGUSLCD_SendPrintProgressToDisplay),
  // LCD Control
  VPHELPER(VP_LCD_BLK, &lcd_default_light, ScreenHandler.LCD_BLK_Adjust, ScreenHandler.DGUSLCD_SendWordValueToDisplay),

  // SD File - Back
  VPHELPER(VP_SD_FileSelect_Back, nullptr, ScreenHandler.SD_FileBack, nullptr),

  // Print Time
  VPHELPER_STR(VP_PrintTime, nullptr, VP_PrintTime_LEN, nullptr, ScreenHandler.DGUSLCD_SendPrintTimeToDisplay_MKS),

  #if ENABLED(PRINTCOUNTER)
    VPHELPER_STR(VP_PrintAccTime, nullptr, VP_PrintAccTime_LEN, nullptr, ScreenHandler.DGUSLCD_SendPrintAccTimeToDisplay),
    VPHELPER_STR(VP_PrintsTotal, nullptr, VP_PrintsTotal_LEN, nullptr, ScreenHandler.DGUSLCD_SendPrintsTotalToDisplay),
  #endif

  VPHELPER(VP_X_STEP_PER_MM, &planner.settings.axis_steps_per_mm[X_AXIS], ScreenHandler.HandleStepPerMMChanged_MKS, ScreenHandler.DGUSLCD_SendFloatAsIntValueToDisplay<0>),
  VPHELPER(VP_Y_STEP_PER_MM, &planner.settings.axis_steps_per_mm[Y_AXIS], ScreenHandler.HandleStepPerMMChanged_MKS, ScreenHandler.DGUSLCD_SendFloatAsIntValueToDisplay<0>),
  VPHELPER(VP_Z_STEP_PER_MM, &planner.settings.axis_steps_per_mm[Z_AXIS], ScreenHandler.HandleStepPerMMChanged_MKS, ScreenHandler.DGUSLCD_SendFloatAsIntValueToDisplay<0>),

  VPHELPER(VP_X_MAX_SPEED, &planner.settings.max_feedrate_mm_s[X_AXIS], ScreenHandler.HandleMaxSpeedChange_MKS, ScreenHandler.DGUSLCD_SendFloatAsIntValueToDisplay<0>),
  VPHELPER(VP_Y_MAX_SPEED, &planner.settings.max_feedrate_mm_s[Y_AXIS], ScreenHandler.HandleMaxSpeedChange_MKS, ScreenHandler.DGUSLCD_SendFloatAsIntValueToDisplay<0>),
  VPHELPER(VP_Z_MAX_SPEED, &planner.settings.max_feedrate_mm_s[Z_AXIS], ScreenHandler.HandleMaxSpeedChange_MKS, ScreenHandler.DGUSLCD_SendFloatAsIntValueToDisplay<0>),

  #if HAS_HOTEND
    VPHELPER(VP_E0_MAX_SPEED, &planner.settings.max_feedrate_mm_s[E_AXIS_N(0)], ScreenHandler.HandleExtruderMaxSpeedChange_MKS, ScreenHandler.DGUSLCD_SendFloatAsIntValueToDisplay<0>),
    #if HAS_MULTI_HOTEND
      VPHELPER(VP_E1_MAX_SPEED, &planner.settings.max_feedrate_mm_s[E_AXIS_N(1)], ScreenHandler.HandleExtruderMaxSpeedChange_MKS, ScreenHandler.DGUSLCD_SendFloatAsIntValueToDisplay<0>),
    #endif
  #endif

  VPHELPER(VP_X_ACC_MAX_SPEED, (uint16_t *)&planner.settings.max_acceleration_mm_per_s2[X_AXIS], ScreenHandler.HandleMaxAccChange_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  VPHELPER(VP_Y_ACC_MAX_SPEED, (uint16_t *)&planner.settings.max_acceleration_mm_per_s2[Y_AXIS], ScreenHandler.HandleMaxAccChange_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  VPHELPER(VP_Z_ACC_MAX_SPEED, (uint16_t *)&planner.settings.max_acceleration_mm_per_s2[Z_AXIS], ScreenHandler.HandleMaxAccChange_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),

  #if HAS_HOTEND
    VPHELPER(VP_E0_ACC_MAX_SPEED, (uint16_t *)&planner.settings.max_acceleration_mm_per_s2[E_AXIS_N(0)], ScreenHandler.HandleExtruderAccChange_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
    #if HAS_MULTI_HOTEND
      VPHELPER(VP_E1_ACC_MAX_SPEED, (uint16_t *)&planner.settings.max_acceleration_mm_per_s2[E_AXIS_N(1)], ScreenHandler.HandleExtruderAccChange_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
    #endif
  #endif

  VPHELPER(VP_TRAVEL_SPEED, (uint16_t *)&planner.settings.travel_acceleration, ScreenHandler.HandleTravelAccChange_MKS, ScreenHandler.DGUSLCD_SendFloatAsIntValueToDisplay<0>),
  VPHELPER(VP_FEEDRATE_MIN_SPEED, (uint16_t *)&planner.settings.min_feedrate_mm_s, ScreenHandler.HandleFeedRateMinChange_MKS, ScreenHandler.DGUSLCD_SendFloatAsIntValueToDisplay<0>),
  VPHELPER(VP_T_F_SPEED, (uint16_t *)&planner.settings.min_travel_feedrate_mm_s, ScreenHandler.HandleMin_T_F_MKS, ScreenHandler.DGUSLCD_SendFloatAsIntValueToDisplay<0>),
  VPHELPER(VP_ACC_SPEED, (uint16_t *)&planner.settings.acceleration, ScreenHandler.HandleAccChange_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),

  VPHELPER(VP_X_PARK_POS, &mks_park_pos.x, ScreenHandler.GetParkPos_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  VPHELPER(VP_Y_PARK_POS, &mks_park_pos.y, ScreenHandler.GetParkPos_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  VPHELPER(VP_Z_PARK_POS, &mks_park_pos.z, ScreenHandler.GetParkPos_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),

  #if ENABLED(PREVENT_COLD_EXTRUSION)
    VPHELPER(VP_MIN_EX_T, &thermalManager.extrude_min_temp, ScreenHandler.HandleGetExMinTemp_MKS, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  #endif

  #if ENABLED(SENSORLESS_HOMING)  // TMC SENSORLESS Setting
    #if X_HAS_STEALTHCHOP
      VPHELPER(VP_TMC_X_STEP, &tmc_step.x, ScreenHandler.TMC_ChangeConfig, ScreenHandler.DGUSLCD_SendTMCStepValue),
    #endif
    #if Y_HAS_STEALTHCHOP
      VPHELPER(VP_TMC_Y_STEP, &tmc_step.y, ScreenHandler.TMC_ChangeConfig, ScreenHandler.DGUSLCD_SendTMCStepValue),
    #endif
    #if Z_HAS_STEALTHCHOP
      VPHELPER(VP_TMC_Z_STEP, &tmc_step.z, ScreenHandler.TMC_ChangeConfig, ScreenHandler.DGUSLCD_SendTMCStepValue),
    #endif
  #endif

  #if HAS_TRINAMIC_CONFIG   // TMC Current Setting
    #if AXIS_IS_TMC(X)
      VPHELPER(VP_TMC_X_Current, &stepperX.val_mA, ScreenHandler.TMC_ChangeConfig, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
    #endif
    #if AXIS_IS_TMC(Y)
      VPHELPER(VP_TMC_Y_Current, &stepperY.val_mA, ScreenHandler.TMC_ChangeConfig, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
    #endif
    #if AXIS_IS_TMC(Z)
      VPHELPER(VP_TMC_Z_Current, &stepperZ.val_mA, ScreenHandler.TMC_ChangeConfig, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
    #endif
    #if AXIS_IS_TMC(E0)
      VPHELPER(VP_TMC_E0_Current, &stepperE0.val_mA, ScreenHandler.TMC_ChangeConfig, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
    #endif
    #if AXIS_IS_TMC(E1)
      VPHELPER(VP_TMC_E1_Current, &stepperE1.val_mA, ScreenHandler.TMC_ChangeConfig, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
    #endif
    #if AXIS_IS_TMC(X2)
      VPHELPER(VP_TMC_X1_Current, &stepperX2.val_mA, ScreenHandler.TMC_ChangeConfig, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
    #endif
    #if AXIS_IS_TMC(Y2)
      VPHELPER(VP_TMC_Y1_Current, &stepperY2.val_mA, ScreenHandler.TMC_ChangeConfig, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
    #endif
    #if AXIS_IS_TMC(Z2)
      VPHELPER(VP_TMC_Z1_Current, &stepperZ2.val_mA, ScreenHandler.TMC_ChangeConfig, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
    #endif
  #endif

  VPHELPER(VP_EEPROM_CTRL, nullptr, ScreenHandler.EEPROM_CTRL, nullptr),
  VPHELPER(VP_LEVEL_BUTTON, nullptr, ScreenHandler.Level_Ctrl_MKS, nullptr),
  VPHELPER(VP_LANGUAGE_CHANGE, nullptr, ScreenHandler.LanguageChange_MKS, nullptr),
  //VPHELPER(VP_Prepare_page, nullptr, ScreenHandler.Perph_page_MKS, nullptr),
  //VPHELPER(VP_FAN_page, nullptr, ScreenHandler.fan_page_MKS, nullptr),
  VPHELPER(VP_PRINT_SET, nullptr, ScreenHandler.print_set_page_MKS, nullptr),
  VPHELPER(VP_PREPARE_PAGE_SET, nullptr, ScreenHandler.PREPARE_PAGE_SET, nullptr),
  VPHELPER(VP_SETTING_PAGE_SET, nullptr, ScreenHandler.SETTING_PAGE_JUMP, nullptr),
  VPHELPER(VP_AUTO_LEVEL_SET, nullptr, ScreenHandler.AUTO_LEVEL_POPUP, nullptr),
  VPHELPER(VP_FAN_ON_OFF_VAL, nullptr, ScreenHandler.SET_FAN_ON_OFF_HANDLER, nullptr),
  VPHELPER(VP_PRINT_SETTINGS, nullptr, ScreenHandler.PRINT_SETTING_HANDLER, nullptr),
  VPHELPER(VP_SETTING_TEMP_PAGE_SET, nullptr, ScreenHandler.SETTING_TEMPER_PAGE_JUMP, nullptr),
  VPHELPER(VP_SETTING_FILAMENT_DET, nullptr, ScreenHandler.SET_FILAMENT_DET_HANDLER, nullptr),

  VPHELPER(VP_RUNOUT_Confirm, nullptr, ScreenHandler.Filament_Runout_Comfirm, nullptr),

  
  VPHELPER(VP_SD_Print_LiveAdjustZ_Confirm, nullptr, ScreenHandler.ZoffsetConfirm, nullptr),

  VPHELPER(VP_ZOffset_Distance,&ZOffset_distanceStep ,ScreenHandler.GetZoffsetDistance, ScreenHandler.DGUSLCD_SendWordValueToDisplay),//打印界面下的zoffset的步长
  VPHELPER(VP_MESH_LEVEL_ADJUST, &AUTO_ZOffset_distanceStep, ScreenHandler.MeshLevelDistanceConfig, ScreenHandler.DGUSLCD_SendWordValueToDisplay),//自动调平界面下的步长
  VPHELPER(VP_MESH_LEVEL_POINT, nullptr, ScreenHandler.MeshLevel,nullptr),

  #if ENABLED(PREVENT_COLD_EXTRUSION)
    VPHELPER(VP_Min_EX_T_E, &thermalManager.extrude_min_temp, ScreenHandler.GetMinExtrudeTemp, ScreenHandler.DGUSLCD_SendWordValueToDisplay),
  #endif

  VPHELPER(VP_AutoTurnOffSw, nullptr, ScreenHandler.GetTurnOffCtrl, nullptr),

  #if HAS_HOTEND
    VPHELPER(VP_E0_STEP_PER_MM, &planner.settings.axis_steps_per_mm[E_AXIS_N(0)], ScreenHandler.HandleStepPerMMExtruderChanged_MKS, ScreenHandler.DGUSLCD_SendFloatAsIntValueToDisplay<0>),
    #if HAS_MULTI_HOTEND
      VPHELPER(VP_E1_STEP_PER_MM, &planner.settings.axis_steps_per_mm[E_AXIS_N(1)], ScreenHandler.HandleStepPerMMExtruderChanged_MKS, ScreenHandler.DGUSLCD_SendFloatAsIntValueToDisplay<0>),
    #endif
  #endif

  // SDCard File listing

  #if ENABLED(SDSUPPORT)
    VPHELPER(VP_SD_FileSelected, nullptr, ScreenHandler.DGUSLCD_SD_FileSelected, nullptr),
    VPHELPER(VP_SD_ScrollEvent, nullptr, ScreenHandler.DGUSLCD_SD_ScrollFilelist, nullptr),
    VPHELPER(VP_SD_FileSelectConfirm, nullptr, ScreenHandler.DGUSLCD_SD_StartPrint, nullptr),
    VPHELPER(VP_SD_Print_Again, nullptr, ScreenHandler.DGUSLCD_SD_PrintAgain, nullptr),

    VPHELPER_STR(VP_SD_FileName0, nullptr, VP_SD_FileName_LEN, nullptr, ScreenHandler.DGUSLCD_SD_SendFilename),
    VPHELPER_STR(VP_SD_FileName1, nullptr, VP_SD_FileName_LEN, nullptr, ScreenHandler.DGUSLCD_SD_SendFilename),
    VPHELPER_STR(VP_SD_FileName2, nullptr, VP_SD_FileName_LEN, nullptr, ScreenHandler.DGUSLCD_SD_SendFilename),
    VPHELPER_STR(VP_SD_FileName3, nullptr, VP_SD_FileName_LEN, nullptr, ScreenHandler.DGUSLCD_SD_SendFilename),
    VPHELPER_STR(VP_SD_FileName4, nullptr, VP_SD_FileName_LEN, nullptr, ScreenHandler.DGUSLCD_SD_SendFilename),
    VPHELPER_STR(VP_SD_FileName5, nullptr, VP_SD_FileName_LEN, nullptr, ScreenHandler.DGUSLCD_SD_SendFilename),
    VPHELPER_STR(VP_SD_FileName6, nullptr, VP_SD_FileName_LEN, nullptr, ScreenHandler.DGUSLCD_SD_SendFilename),
    VPHELPER_STR(VP_SD_FileName7, nullptr, VP_SD_FileName_LEN, nullptr, ScreenHandler.DGUSLCD_SD_SendFilename),
    VPHELPER_STR(VP_SD_FileName8, nullptr, VP_SD_FileName_LEN, nullptr, ScreenHandler.DGUSLCD_SD_SendFilename),
    VPHELPER_STR(VP_SD_FileName9, nullptr, VP_SD_FileName_LEN, nullptr, ScreenHandler.DGUSLCD_SD_SendFilename),
    VPHELPER(VP_SD_ResumePauseAbort, nullptr, ScreenHandler.DGUSLCD_SD_ResumePauseAbort, nullptr),
    VPHELPER(VP_SD_AbortPrintConfirmed, nullptr, ScreenHandler.DGUSLCD_SD_ReallyAbort, nullptr),
    VPHELPER(VP_SD_Print_Setting, nullptr, ScreenHandler.DGUSLCD_SD_PrintTune, nullptr),
    #if ENABLED(BABYSTEPPING)
      VPHELPER(VP_SD_Print_LiveAdjustZ, nullptr, ScreenHandler.HandleLiveAdjustZ, ScreenHandler.DGUSLCD_SendFloatAsIntValueToDisplay<2>),
      //VPHELPER(VP_ZOffset_DE_DIS, &z_offset_add, nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
      VPHELPER(VP_ZOffset_DE_DIS, &probe.offset.z, nullptr, ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
    #endif
    #if HAS_BED_PROBE
      VPHELPER(VP_OFFSET_X, &probe.offset.x, ScreenHandler.GetOffsetValue,ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
      VPHELPER(VP_OFFSET_Y, &probe.offset.y, ScreenHandler.GetOffsetValue,ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
      //VPHELPER(VP_OFFSET_Z, &probe.offset.z, ScreenHandler.GetOffsetValue,ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
      VPHELPER(VP_OFFSET_Z, &probe.offset.z, nullptr,ScreenHandler.DGUSLCD_SendFloatAsLongValueToDisplay<2>),
    #endif
  #else
    VPHELPER(VP_SD_FileSelected, nullptr, ScreenHandler.PrintReturn, nullptr),
  #endif

  #if ENABLED(DGUS_UI_WAITING)
    VPHELPER(VP_WAITING_STATUS, nullptr, nullptr, ScreenHandler.DGUSLCD_SendWaitingStatusToDisplay),
  #endif

  VPHELPER(VP_TEST_TEMP1, nullptr, nullptr, ScreenHandler.DGUSLCD_SendStringToDisplay_Language_MKS),
  VPHELPER(VP_TEST_TEMP2, nullptr, nullptr, ScreenHandler.DGUSLCD_SendStringToDisplay_Language_MKS),
  VPHELPER(VP_TEST_TEMP3, nullptr, nullptr, ScreenHandler.DGUSLCD_SendStringToDisplay_Language_MKS),
  VPHELPER(VP_TEST_TEMP4, nullptr, nullptr, ScreenHandler.DGUSLCD_SendStringToDisplay_Language_MKS),

  // Messages for the User, shared by the popup and the kill screen. They can't be autouploaded as we do not buffer content.
  //{.VP = VP_MSGSTR1, .memadr = nullptr, .size = VP_MSGSTR1_LEN, .set_by_display_handler = nullptr, .send_to_display_handler = ScreenHandler.DGUSLCD_SendStringToDisplayPGM},
  //{.VP = VP_MSGSTR2, .memadr = nullptr, .size = VP_MSGSTR2_LEN, .set_by_display_handler = nullptr, .send_to_display_handler = ScreenHandler.DGUSLCD_SendStringToDisplayPGM},
  //{.VP = VP_MSGSTR3, .memadr = nullptr, .size = VP_MSGSTR3_LEN, .set_by_display_handler = nullptr, .send_to_display_handler = ScreenHandler.DGUSLCD_SendStringToDisplayPGM},
  //{.VP = VP_MSGSTR4, .memadr = nullptr, .size = VP_MSGSTR4_LEN, .set_by_display_handler = nullptr, .send_to_display_handler = ScreenHandler.DGUSLCD_SendStringToDisplayPGM},

  {.VP = VP_MSGSTR1, .memadr = nullptr, .size = VP_MSGSTR1_LEN, .set_by_display_handler = nullptr, .send_to_display_handler = ScreenHandler.DGUSLCD_SendStringToDisplay_Language_MKS},
  {.VP = VP_MSGSTR2, .memadr = nullptr, .size = VP_MSGSTR2_LEN, .set_by_display_handler = nullptr, .send_to_display_handler = ScreenHandler.DGUSLCD_SendStringToDisplay_Language_MKS},
  {.VP = VP_MSGSTR3, .memadr = nullptr, .size = VP_MSGSTR3_LEN, .set_by_display_handler = nullptr, .send_to_display_handler = ScreenHandler.DGUSLCD_SendStringToDisplay_Language_MKS},
  {.VP = VP_MSGSTR4, .memadr = nullptr, .size = VP_MSGSTR4_LEN, .set_by_display_handler = nullptr, .send_to_display_handler = ScreenHandler.DGUSLCD_SendStringToDisplay_Language_MKS},

  VPHELPER(0, 0, 0, 0) // must be last entry.
};

#endif // DGUS_LCD_UI_MKS
