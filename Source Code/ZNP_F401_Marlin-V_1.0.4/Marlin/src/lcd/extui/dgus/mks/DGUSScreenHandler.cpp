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

#include "../DGUSScreenHandler.h"

#include "../../../../inc/MarlinConfig.h"

#include "../../../../MarlinCore.h"
#include "../../../../module/settings.h"
#include "../../../../module/temperature.h"
#include "../../../../module/motion.h"
#include "../../../../module/planner.h"
#include "../../../../module/printcounter.h"
#include "../../../../gcode/gcode.h"
#include "mks_hardware.h"
#if HAS_STEALTHCHOP
  #include "../../../../module/stepper/trinamic.h"
  #include "../../../../module/stepper/indirection.h"
#endif
#include "../../../../module/probe.h"

#if ENABLED(POWER_LOSS_RECOVERY)
  #include "../../../../feature/powerloss.h"
#endif

#if ENABLED(SDSUPPORT)
  static ExtUI::FileList filelist;
#endif

#define PRINT_FILAMENT_LENGTH   370             //mm
#define PRINT_FILAMENT_SPEED    720             //mm/min


extern uint8_t Leveling_done_flg;
uint8_t Leveling_done_zhome_flg = 0;

extern bool filament_change_flg;
bool loadfilament_confirm_flg=false;
bool unloadfilament_confirm_flg=false;
bool Filament_heating_flg=false;

bool recovery_flg;
uint32_t recovery_ms=0;

bool DGUSAutoTurnOff = false;
uint8_t mks_language_index; // Initialized by settings.load()

// endianness swap
uint32_t swap32(const uint32_t value) { return (value & 0x000000FFU) << 24U | (value & 0x0000FF00U) << 8U | (value & 0x00FF0000U) >> 8U | (value & 0xFF000000U) >> 24U; }

#if 0
void DGUSScreenHandler::sendinfoscreen_ch_mks(const uint16_t *line1, const uint16_t *line2, const uint16_t *line3, const uint16_t *line4) {
  dgusdisplay.WriteVariable(VP_MSGSTR1, line1, 32, true);
  dgusdisplay.WriteVariable(VP_MSGSTR2, line2, 32, true);
  dgusdisplay.WriteVariable(VP_MSGSTR3, line3, 32, true);
  dgusdisplay.WriteVariable(VP_MSGSTR4, line4, 32, true);
}

void DGUSScreenHandler::sendinfoscreen_en_mks(const char *line1, const char *line2, const char *line3, const char *line4) {
  dgusdisplay.WriteVariable(VP_MSGSTR1, line1, 32, true);
  dgusdisplay.WriteVariable(VP_MSGSTR2, line2, 32, true);
  dgusdisplay.WriteVariable(VP_MSGSTR3, line3, 32, true);
  dgusdisplay.WriteVariable(VP_MSGSTR4, line4, 32, true);
}

void DGUSScreenHandler::sendinfoscreen_mks(const void *line1, const void *line2, const void *line3, const void *line4, uint16_t language) {
  if (language == MKS_English)
    DGUSScreenHandler::sendinfoscreen_en_mks((char *)line1, (char *)line2, (char *)line3, (char *)line4);
  else if (language == MKS_SimpleChinese)
    DGUSScreenHandler::sendinfoscreen_ch_mks((uint16_t *)line1, (uint16_t *)line2, (uint16_t *)line3, (uint16_t *)line4);
}

#endif

void DGUSScreenHandler::DGUSLCD_SendFanToDisplay(DGUS_VP_Variable &var) {
  if (var.memadr) {
    //DEBUG_ECHOPGM(" DGUS_LCD_SendWordValueToDisplay ", var.VP);
    //DEBUG_ECHOLNPGM(" data ", *(uint16_t *)var.memadr);
    uint16_t tmp = *(uint8_t *) var.memadr; // +1 -> avoid rounding issues for the display.

    uint16_t tmp1 = tmp;
    DEBUG_ECHOLNPGM("FAN value set:", tmp1);
    tmp = map(tmp, 0, 255, 0, 100);

    dgusdisplay.WriteVariable(var.VP, tmp);

    //tmp1 = map(tmp1, 0, 255, 0, 10);

    //dgusdisplay.WriteVariable(VP_Fan0_Percentage_pic, tmp1);
  }
}

void DGUSScreenHandler::DGUSLCD_SendBabyStepToDisplay_MKS(DGUS_VP_Variable &var) {
  float value = current_position.z;
  DEBUG_ECHOLNPGM(" >> ", value, 6);
  value *= cpow(10, 2);
  dgusdisplay.WriteVariable(VP_SD_Print_Baby, (uint16_t)value);
}

void DGUSScreenHandler::DGUSLCD_SendPrintTimeToDisplay_MKS(DGUS_VP_Variable &var) {
  duration_t elapsed = print_job_timer.duration();
  uint32_t time = elapsed.value;
  dgusdisplay.WriteVariable(VP_PrintTime_H, uint16_t(time / 3600));
  dgusdisplay.WriteVariable(VP_PrintTime_M, uint16_t(time % 3600 / 60));
  dgusdisplay.WriteVariable(VP_PrintTime_S, uint16_t((time % 3600) % 60));
}

void DGUSScreenHandler::DGUSLCD_SetUint8(DGUS_VP_Variable &var, void *val_ptr) {
  if (var.memadr) {
    const uint16_t value = swap16(*(uint16_t*)val_ptr);
    DEBUG_ECHOLNPGM("FAN value get:", value);
    *(uint8_t*)var.memadr = map(constrain(value, 0, 255), 0, 255, 0, 255);
    DEBUG_ECHOLNPGM("FAN value change:", *(uint8_t*)var.memadr);
  }
}

void DGUSScreenHandler::DGUSLCD_SendGbkToDisplay(DGUS_VP_Variable &var) {
  DEBUG_ECHOLNPGM(" data ", *(uint16_t *)var.memadr);
  uint16_t *tmp = (uint16_t*) var.memadr;
  dgusdisplay.WriteVariable(var.VP, tmp, var.size, true);
}

void DGUSScreenHandler::DGUSLCD_SendStringToDisplay_Language_MKS(DGUS_VP_Variable &var) {
  if (mks_language_index == MKS_English) {
    char *tmp = (char*) var.memadr;
    dgusdisplay.WriteVariable(var.VP, tmp, var.size, true);
  }
  else if (mks_language_index == MKS_SimpleChinese) {
    uint16_t *tmp = (uint16_t *)var.memadr;
    dgusdisplay.WriteVariable(var.VP, tmp, var.size, true);
  }
}

void DGUSScreenHandler::DGUSLCD_SendTMCStepValue(DGUS_VP_Variable &var) {
  #if ENABLED(SENSORLESS_HOMING)
    #if X_HAS_STEALTHCHOP
      tmc_step.x = stepperX.homing_threshold();
      dgusdisplay.WriteVariable(var.VP, *(int16_t*)var.memadr);
    #endif
    #if Y_HAS_STEALTHCHOP
      tmc_step.y = stepperY.homing_threshold();
      dgusdisplay.WriteVariable(var.VP, *(int16_t*)var.memadr);
    #endif
    #if Z_HAS_STEALTHCHOP
      tmc_step.z = stepperZ.homing_threshold();
      dgusdisplay.WriteVariable(var.VP, *(int16_t*)var.memadr);
    #endif
  #endif
}

#if ENABLED(SDSUPPORT)

  void DGUSScreenHandler::DGUSLCD_SD_FileSelected(DGUS_VP_Variable &var, void *val_ptr) {
    uint16_t touched_nr = (int16_t)swap16(*(uint16_t*)val_ptr) + top_file;
    
    if (touched_nr != 0x0F && touched_nr > filelist.count()) return;

    if (!filelist.seek(touched_nr) && touched_nr != 0x0F) return;

    // if (touched_nr == 0x0F) {
    //   if (filelist.isAtRootDir()){
    //     GotoScreen(DGUSLCD_SCREEN_MAIN);
    //   }
    //   else
    //     filelist.upDir();
    //   return;
    // }

    if (filelist.isDir()) {
      filelist.changeDir(filelist.filename());
      top_file = 0;
      ForceCompleteUpdate();
      return;
    }

    #if ENABLED(DGUS_PRINT_FILENAME)
      // Send print filename
      dgusdisplay.WriteVariable(VP_SD_Print_Filename, filelist.filename(), VP_SD_FileName_LEN, true);
    #endif

    // Setup Confirmation screen
    file_to_print = touched_nr;
    GotoScreen(MKSLCD_SCREEN_PRINT_CONFIRM);
  }

  void DGUSScreenHandler::DGUSLCD_SD_StartPrint(DGUS_VP_Variable &var, void *val_ptr) {
    const uint16_t value = swap16(*(uint16_t*)val_ptr);
    switch(value)
    {
      case 0:
        if (!filelist.seek(file_to_print)) return;
        ExtUI::printFile(filelist.shortFilename());
        GotoScreen(MKSLCD_SCREEN_PRINT);
        z_offset_add = 0;
      break;
      case 1:
        GotoScreen(MKSLCD_SCREEN_CHOOSE_FILE);
      break;
    }
  }
  void DGUSScreenHandler::DGUSLCD_SD_PrintAgain(DGUS_VP_Variable &var, void *val_ptr) {
    const uint16_t value = swap16(*(uint16_t*)val_ptr);
    switch(value)
    {
      case 0:
        if (!filelist.seek(file_to_print)) return;
        ExtUI::printFile(filelist.shortFilename());
        GotoScreen(MKSLCD_SCREEN_PRINT);
        z_offset_add = 0;
      break;
      case 1:
        // settings.save();
        GotoScreen(DGUSLCD_SCREEN_MAIN);
	      queue.inject_P(PSTR("M500"));
      break;
    }
  }
  void DGUSScreenHandler::DGUSLCD_SD_ResumePauseAbort(DGUS_VP_Variable &var, void *val_ptr) {

    if (!ExtUI::isPrintingFromMedia()) return; // avoid race condition when user stays in this menu and printer finishes.
    switch (swap16(*(uint16_t*)val_ptr)) {
      case 0: { // Resume

        auto cs = getCurrentScreen();
        //if (runout_mks.runout_status != RUNOUT_WAITTING_STATUS && runout_mks.runout_status != UNRUNOUT_STATUS) {
		if (runout_mks.runout_status == RUNOUT_STATUS) {
		  if (cs == MKSLCD_SCREEN_PRINT || cs == MKSLCD_SCREEN_PAUSE)
            GotoScreen(MKSLCD_SCREEN_PAUSE);
          return;
        }
        else
          runout_mks.runout_status = UNRUNOUT_STATUS;

        GotoScreen(MKSLCD_SCREEN_PRINT);

        if (ExtUI::isPrintingFromMediaPaused()) {
          nozzle_park_mks.print_pause_start_flag = 0;
          nozzle_park_mks.blstatus = true;
          ExtUI::resumePrint();
        }
      } break;

      case 1: // Pause

        GotoScreen(MKSLCD_SCREEN_PAUSE);
        if (!ExtUI::isPrintingFromMediaPaused()) {
          nozzle_park_mks.print_pause_start_flag = 1;
          nozzle_park_mks.blstatus = true;
          ExtUI::pausePrint();
          //ExtUI::mks_pausePrint();
        }
        break;
      case 2: // Abort
        //HandleUserConfirmationPopUp(VP_SD_AbortPrintConfirmed, nullptr, PSTR("Abort printing"), filelist.filename(), PSTR("?"), true, true, false, true);
        GotoScreen(MKSLCD_SCREEN_PRINT_STAT_STOP_POPUP);
        break;
    }
  }

  void DGUSScreenHandler::DGUSLCD_SD_SendFilename(DGUS_VP_Variable& var) {
    uint16_t target_line = (var.VP - VP_SD_FileName0) / VP_SD_FileName_LEN;
    if (target_line > DGUS_SD_FILESPERSCREEN) return;
    char tmpfilename[VP_SD_FileName_LEN + 1] = "";
    var.memadr = (void*)tmpfilename;

    uint16_t dir_icon_val = 25;
    if (filelist.seek(top_file + target_line)) {
      snprintf_P(tmpfilename, VP_SD_FileName_LEN, PSTR("%s%c"), filelist.filename(), filelist.isDir() ? '/' : 0); // snprintf_P(tmpfilename, VP_SD_FileName_LEN, PSTR("%s"), filelist.filename());
      //dir_icon_val = filelist.isDir() ? 0 : 1;
      dir_icon_val = filelist.isDir() ? 2 : 1;
    }
    DGUSLCD_SendStringToDisplay(var);

    dgusdisplay.WriteVariable(VP_File_Pictutr0 + target_line * 2, dir_icon_val);
  }

  void DGUSScreenHandler::SDCardInserted() {
    top_file = 0;
    filelist.refresh();
    // auto cs = getCurrentScreen();
    // if (cs == DGUSLCD_SCREEN_MAIN || cs == DGUSLCD_SCREEN_STATUS)
    //  GotoScreen(MKSLCD_SCREEN_CHOOSE_FILE);
  }

  void DGUSScreenHandler::SDCardRemoved() {
    if (current_screen == DGUSLCD_SCREEN_SDFILELIST
        || (/*current_screen == DGUSLCD_SCREEN_CONFIRM && */(ConfirmVP == VP_SD_AbortPrintConfirmed || ConfirmVP == VP_SD_FileSelectConfirm))
        || current_screen == DGUSLCD_SCREEN_SDPRINTMANIPULATION
    ) filelist.refresh();
  }

  void DGUSScreenHandler::SDPrintingFinished() {
    if (DGUSAutoTurnOff) {
      queue.exhaust();
      gcode.process_subcommands_now(F("M81"));
    }
    GotoScreen(MKSLCD_SCREEN_PrintDone);
  }

#else
  void DGUSScreenHandler::PrintReturn(DGUS_VP_Variable& var, void *val_ptr) {
    uint16_t value = swap16(*(uint16_t*)val_ptr);
    if (value == 0x0F) GotoScreen(DGUSLCD_SCREEN_MAIN);
  }
#endif // SDSUPPORT

void DGUSScreenHandler::ScreenChangeHook(DGUS_VP_Variable &var, void *val_ptr) {
  uint8_t *tmp = (uint8_t*)val_ptr;

  // The keycode in target is coded as <from-frame><to-frame>, so 0x0100A means
  // from screen 1 (main) to 10 (temperature). DGUSLCD_SCREEN_POPUP is special,
  // meaning "return to previous screen"
  DGUSLCD_Screens target = (DGUSLCD_Screens)tmp[1];

  DEBUG_ECHOLNPGM("\n DEBUG target", target);

  // when the dgus had reboot, it will enter the DGUSLCD_SCREEN_MAIN page,
  // so user can change any page to use this function, an it will check
  // if robin nano is printing. when it is, dgus will enter the printing
  // page to continue print;
  //
  //if (printJobOngoing() || printingIsPaused()) {
  //  if (target == MKSLCD_PAUSE_SETTING_MOVE || target == MKSLCD_PAUSE_SETTING_EX
  //    || target == MKSLCD_SCREEN_PRINT || target == MKSLCD_SCREEN_PAUSE
  //  ) {
  //  }
  //  else
  //    GotoScreen(MKSLCD_SCREEN_PRINT);
  // return;
  //}
#if 0
  if (target == DGUSLCD_SCREEN_POPUP) {
    SetupConfirmAction(ExtUI::setUserConfirmed);

    // Special handling for popup is to return to previous menu
    if (current_screen == DGUSLCD_SCREEN_POPUP && confirm_action_cb) confirm_action_cb();
   
    PopToOldScreen();
    return;
  }
#endif
  UpdateNewScreen(target);

  #ifdef DEBUG_DGUSLCD
    if (!DGUSLCD_FindScreenVPMapList(target)) DEBUG_ECHOLNPGM("WARNING: No screen Mapping found for ", target);
  #endif
}

void DGUSScreenHandler::ScreenBackChange(DGUS_VP_Variable &var, void *val_ptr) {
  const uint16_t target = swap16(*(uint16_t *)val_ptr);
  DEBUG_ECHOLNPGM(" back = 0x%x", target);
  switch (target) {
  }
}

void DGUSScreenHandler::ZoffsetConfirm(DGUS_VP_Variable &var, void *val_ptr) {
  settings.save();
  if (printJobOngoing())
    GotoScreen(MKSLCD_SCREEN_PRINT);
  else if (print_job_timer.isPaused)
    GotoScreen(MKSLCD_SCREEN_PAUSE);
}

void DGUSScreenHandler::GetTurnOffCtrl(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("GetTurnOffCtrl\n");
  const uint16_t value = swap16(*(uint16_t *)val_ptr);
  switch (value) {
    case 0 ... 1: DGUSAutoTurnOff = (bool)value; break;
    default: break;
  }
}

void DGUSScreenHandler::GetMinExtrudeTemp(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("GetMinExtrudeTemp");
  const uint16_t value = swap16(*(uint16_t *)val_ptr);
  TERN_(PREVENT_COLD_EXTRUSION, thermalManager.extrude_min_temp = value);
  mks_min_extrusion_temp = value;
  settings.save();
}

void DGUSScreenHandler::GetZoffsetDistance(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("GetZoffsetDistance");
  const uint16_t value = swap16(*(uint16_t *)val_ptr);
  float val_distance = 0;
  switch (value) {
    // case 0: val_distance = 0.01; break;
    // case 1: val_distance = 0.1; break;
    // case 2: val_distance = 0.5; break;
    // case 3: val_distance = 1; break;
    // default: val_distance = 0.01; break;
    case 0: val_distance = 1; break;
    case 1: val_distance = 10; break;
    case 2: val_distance = 50; break;
    case 3: val_distance = 100; break;
    default: val_distance = 1; break;    
  }
  ZOffset_distance = val_distance;
  ZOffset_distanceStep = value;
}

void DGUSScreenHandler::GetFeedratePercentageStep(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("\nGetFeedratePercentageStep");
  *(uint16_t *)var.memadr = swap16(*(uint16_t *)val_ptr);
}

void DGUSScreenHandler::HandleFeedratePercentageAdjust(DGUS_VP_Variable &var, void *val_ptr) {
    DEBUG_ECHOLNPGM("HandleFeedratePercentageAdjust");
    uint16_t FeedratePercentageStep_temp;

    switch(FeedratePercentageStep)
    {
      case 0:
          FeedratePercentageStep_temp = 1;
      break;
      case 1:
          FeedratePercentageStep_temp = 5;
      break;
      case 2:
          FeedratePercentageStep_temp = 10;
      break;
      default:
          FeedratePercentageStep_temp = 1;
      break;
    }
    uint16_t flag = swap16(*(uint16_t*)val_ptr);
    switch (flag) {
      case 0:
          feedrate_percentage -= FeedratePercentageStep_temp;
        break;
      case 1:
          feedrate_percentage += FeedratePercentageStep_temp;
        break;
      case 2:
          feedrate_percentage = 100;
        break;
      default:
        break;
    }
    
    ForceCompleteUpdate();
  }

void DGUSScreenHandler::GetFanPercentageStep(DGUS_VP_Variable &var, void *val_ptr) {
  // DEBUG_ECHOLNPGM("GetFanPercentageStep");
  // const uint16_t value = swap16(*(uint16_t *)val_ptr);
  // uint16_t val_distance = 0;
  // switch (value) {
  //   case 0: val_distance = 1; break;
  //   case 1: val_distance = 5; break;
  //   case 2: val_distance = 10; break;
  //   default: val_distance = 1; break;
  // }
  // FanPercentageStep = val_distance;
  DEBUG_ECHOLNPGM("\nGetFanPercentageStep");
  *(uint16_t *)var.memadr = swap16(*(uint16_t *)val_ptr);  
}

uint8_t fan_speed_main_percent = 0;
void DGUSScreenHandler::HandleFanPercentageAdjust(DGUS_VP_Variable &var, void *val_ptr) {
    DEBUG_ECHOLNPGM("HandleFanPercentageAdjust");

    int16_t fan_speed_temp = thermalManager.fan_speed[0];
    int16_t FanPercentageStep_temp;
    switch (FanPercentageStep) {
      case 0: FanPercentageStep_temp = 1; break;
      case 1: FanPercentageStep_temp = 5; break;
      case 2: FanPercentageStep_temp = 10; break;
      default: FanPercentageStep_temp = 1; break;
    }
    //uint16_t step = map(FanPercentageStep_temp, 0,100,0, 255);
    uint16_t flag = swap16(*(uint16_t*)val_ptr);
    switch (flag) {
      case 0:
          fan_speed_temp -= FanPercentageStep_temp;
          if(fan_speed_temp<=0)fan_speed_temp=0;
          thermalManager.fan_speed[0] = fan_speed_temp;
        break;
      case 1:
          fan_speed_temp += FanPercentageStep_temp;
          if(fan_speed_temp>=255)fan_speed_temp=255;
          thermalManager.fan_speed[0] = fan_speed_temp;
        break;
      case 2:
          thermalManager.fan_speed[0] = 255;
        break;
      default:
        break;
    }
    
    ForceCompleteUpdate();
    fan_speed_main_percent = (thermalManager.fan_speed[0] * 100) / 255;
  }

void DGUSScreenHandler::GetManualMovestep(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("\nGetManualMovestep");
  *(uint16_t *)var.memadr = swap16(*(uint16_t *)val_ptr);
}

void DGUSScreenHandler::GetManualE0_T_step(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("\nGetManualE0_T_step");
  *(uint16_t *)var.memadr = swap16(*(uint16_t *)val_ptr);
}

void DGUSScreenHandler::GetManualHotB_T_step(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("\nGetManualHotB_T_step");
  *(uint16_t *)var.memadr = swap16(*(uint16_t *)val_ptr);
}

void DGUSScreenHandler::EEPROM_CTRL(DGUS_VP_Variable &var, void *val_ptr) {
  const uint16_t eep_flag = swap16(*(uint16_t *)val_ptr);
  // switch (eep_flag) {
  //   case 0:
  //     settings.save();
  //     settings.load(); // load eeprom data to check the data is right
  //     GotoScreen(MKSLCD_SCREEN_EEP_Config);
  //     break;

  //   case 1:
  //     settings.reset();
  //     GotoScreen(MKSLCD_SCREEN_EEP_Config);
  //     break;

  //   default: break;
  // }
  switch(eep_flag) {
    case 0: settings.reset(); GotoScreen(MKSLCD_SCREEN_SETTING); break;
    case 1: GotoScreen(MKSLCD_SCREEN_SETTING);break;
  }
}

void DGUSScreenHandler::Z_offset_select(DGUS_VP_Variable &var, void *val_ptr) {
  const uint16_t z_value = swap16(*(uint16_t *)val_ptr);
  switch (z_value) {
    case 0: Z_distance = 0.01; break;
    case 1: Z_distance = 0.1; break;
    case 2: Z_distance = 0.5; break;
    default: Z_distance = 1; break;
  }
}

void DGUSScreenHandler::GetOffsetValue(DGUS_VP_Variable &var, void *val_ptr) {

  #if HAS_BED_PROBE
    int32_t value = swap32(*(int32_t *)val_ptr);
    float Offset = value / 100.0f;
    DEBUG_ECHOLNPGM("\nget int6 offset >> ", value, 6);
  #endif

  switch (var.VP) {
    case VP_OFFSET_X: TERN_(HAS_BED_PROBE, probe.offset.x = Offset); break;
    case VP_OFFSET_Y: TERN_(HAS_BED_PROBE, probe.offset.y = Offset); break;
    case VP_OFFSET_Z: TERN_(HAS_BED_PROBE, probe.offset.z = Offset); break;
    default: break;
  }
  settings.save();
}

uint8_t prepare_from = 0;
void DGUSScreenHandler::Perph_page_MKS(DGUS_VP_Variable &var, void *val_ptr) {
    #if 0
    const uint16_t value = swap16(*(uint16_t *)val_ptr);

    switch(value) {
      case 0: 
        if(prepare_from == 0) GotoScreen(MKSLCD_SRCEEN_PREPARE);
        else GotoScreen(MKSLCD_PRINT_SETTING);
      break;

      case 1:
        prepare_from = 0;
        GotoScreen(MKS_LCD_PREHEAT);
      break;

      case 2:
        prepare_from  = 1;
        GotoScreen(MKS_LCD_PREHEAT);
      break;
    }
    #endif
}

uint8_t fan_from = 0;
void DGUSScreenHandler::fan_page_MKS(DGUS_VP_Variable &var, void *val_ptr) {
    #if 0
    const uint16_t value = swap16(*(uint16_t *)val_ptr);
    switch(value) {
      case 0: 
        if(fan_from == 0) GotoScreen(MKSLCD_SRCEEN_PREPARE);
        else GotoScreen(MKSLCD_PRINT_SETTING);
      break;

      case 1:
        fan_from = 0;
        GotoScreen(DGUSLCD_SCREEN_FAN_CTRL);
      break;

      case 2:
        fan_from  = 1;
        GotoScreen(DGUSLCD_SCREEN_FAN_CTRL);
      break;
    }
    #endif
}

uint8_t print_set_from = 0;
void DGUSScreenHandler::print_set_page_MKS(DGUS_VP_Variable &var, void *val_ptr) {
    const uint16_t value = swap16(*(uint16_t *)val_ptr);
    switch(value) {
      case 0: 
        print_set_from = 0;
        GotoScreen(MKSLCD_Screen_PRINT_FLAMENT);
      break;

      case 1:
        print_set_from = 1;
        GotoScreen(MKSLCD_Screen_PRINT_FLAMENT);
      break;

      // case 2:
      //   if(print_set_from == 0) GotoScreen(MKSLCD_SCREEN_PRINT);
      //   else GotoScreen(MKSLCD_SCREEN_PAUSE);
      // break;
    }
    ForceCompleteUpdate();
}

void DGUSScreenHandler::PREPARE_PAGE_SET(DGUS_VP_Variable &var, void *val_ptr) {
    const uint16_t value = swap16(*(uint16_t *)val_ptr);
    switch(value) {
      case 0: GotoScreen(MKSLCD_SCREEN_MOVE);
      break;

      case 1: GotoScreen(MKS_LCD_PREHEAT);
      break;

      case 2: GotoScreen(MKSLCD_SCREEN_EXTRUDE_P1);
      break;

      // back
      case 4:  GotoScreen(MKSLCD_SCREEN_HOME);
      break; 
    }
    ForceCompleteUpdate();
}

void DGUSScreenHandler::SETTING_PAGE_JUMP(DGUS_VP_Variable &var, void *val_ptr) {

    const uint16_t value = swap16(*(uint16_t *)val_ptr);

    switch(value) {
      case 0: GotoScreen(DGUSLCD_SCREEN_MAIN); break;
      case 1: GotoScreen(MKSLCD_Screen_LANGUAGE); break;
      case 2: GotoScreen(MKS_LCD_PREHEAT_DEFAULT);  break;
      case 3: queue.inject_P(PSTR("M84"));  break; 
      // case 4: GotoScreen(DGUSLCD_SCREEN_MAIN);  break; 
      case 5: GotoScreen(MKSLCD_Screen_EEPROM_POPUP);  break; 
      case 6: GotoScreen(MKSLCD_ABOUT);  break; 
      case 7: GotoScreen(MKSLCD_SCREEN_SETTING);  break; 
    }
    ForceCompleteUpdate();
}

void DGUSScreenHandler::SETTING_TEMPER_PAGE_JUMP(DGUS_VP_Variable &var, void *val_ptr) {

    const uint16_t value = swap16(*(uint16_t *)val_ptr);

    switch(value) {
      case 0: GotoScreen(MKSLCD_SCREEN_SETTING); break;
      case 1: GotoScreen(MKS_LCD_PREHEAT_DEFAULT_PLA); break;
      case 2: GotoScreen(MKS_LCD_PREHEAT_DEFAULT_ABS);  break;
      case 3: GotoScreen(MKS_LCD_PREHEAT_DEFAULT_AL);  break;
      case 4: GotoScreen(MKS_LCD_PREHEAT_DEFAULT);  break;
    }
    ForceCompleteUpdate();
}

uint8_t is_need_update = 0; 

uint8_t level_temp_check() {
  
  if(is_need_update == 1) {

    if((thermalManager.temp_hotend[0].celsius) >= mks_AL_default_e0_temp && (thermalManager.temp_bed.celsius >= mks_AL_default_bed_temp)) 
    {
      is_need_update = 0;
      return 1;
    }else {
      return 0;
    }
  }else{
    return 0;
  }
}

void DGUSScreenHandler::AUTO_LEVEL_POPUP(DGUS_VP_Variable &var, void *val_ptr) {
    const uint16_t value = swap16(*(uint16_t *)val_ptr);

    switch(value) {
      case 0: 
        GotoScreen(MKSLCD_Screen_AUTO_LEVEL_POPUP1); 
        thermalManager.temp_hotend[0].target = mks_AL_default_e0_temp;
        thermalManager.temp_bed.target = mks_AL_default_bed_temp;
        is_need_update = 1;
      break;   // 进入弹窗
      case 1: GotoScreen(DGUSLCD_SCREEN_MAIN); break; // 取消返回
      case 2: GotoScreen(MKSLCD_Screen_AUTO_LEVEL_POPUP); break;
      case 3: break;
    }
    ForceCompleteUpdate();
}

uint16_t fan_icon_val = 1; // 1: is default is off;
extern uint8_t fan_speed_main_percent;
void DGUSScreenHandler::SET_FAN_ON_OFF_HANDLER(DGUS_VP_Variable &var, void *val_ptr) {
    const uint16_t value = swap16(*(uint16_t *)val_ptr);
     
    switch(value) {
      case 0:   
        thermalManager.fan_speed[0] = 255;
      break;

      case 1:
        thermalManager.fan_speed[0] = 0;
      break;
    }
    ForceCompleteUpdate();
    fan_speed_main_percent = (thermalManager.fan_speed[0] * 100) / 255;
}

void DGUSScreenHandler::SET_FILAMENT_DET_HANDLER(DGUS_VP_Variable &var, void *val_ptr) {
    const uint16_t value = swap16(*(uint16_t *)val_ptr);
     
    switch(value) {
      case 0:   
        mks_filament_det_enable = 0;
        queue.inject_P(PSTR("M500"));
      break;

      case 1:
        mks_filament_det_enable = 1;
        queue.inject_P(PSTR("M500"));
      break;
    }
    ForceCompleteUpdate();
}

void DGUSScreenHandler::PRINT_SETTING_HANDLER(DGUS_VP_Variable &var, void *val_ptr) {
    const uint16_t value = swap16(*(uint16_t *)val_ptr);
     
    switch(value) {
      case 0:   
        // if( )   // 判断是否打印中
        if(print_set_from == 0) GotoScreen(MKSLCD_SCREEN_PRINT);
        else GotoScreen(MKSLCD_SCREEN_PAUSE);
      break;
        
      case 1:
        GotoScreen(MKSLCD_Screen_PRINT_FLAMENT);
      break;

      case 2: 
        GotoScreen(MKSLCD_Screen_PRINT_SPEED); 
      break;

      case 3: 
        GotoScreen(MKSLCD_Screen_Baby);
      break;

      case 4: 
        GotoScreen(MKSLCD_Screen_PRINT_SPEED); 
      break;    
      case 5: 
        GotoScreen(MKSLCD_Screen_FLOW_SPEED); 
      break;        
      case 6: 
        GotoScreen(MKSLCD_Screen_FAN_SPEED); 
      break; 

      case 7: 
        GotoScreen(MKSLCD_Screen_PRINT_FLAMENT); 
      break; 	
      case 8: 
        GotoScreen(MKSLCD_Screen_PRINT_FLAMENT_HOTB); 
      break;	  
    }
    ForceCompleteUpdate();
}


void DGUSScreenHandler::LanguageChange_MKS(DGUS_VP_Variable &var, void *val_ptr) {
  const uint16_t lag_flag = swap16(*(uint16_t *)val_ptr);
  switch (lag_flag) {
    case MKS_SimpleChinese:
      DGUS_LanguageDisplay(MKS_SimpleChinese);
      mks_language_index = MKS_SimpleChinese;
      dgusdisplay.MKS_WriteVariable(VP_LANGUAGE_CHANGE1, MKS_Language_Choose);
      dgusdisplay.MKS_WriteVariable(VP_LANGUAGE_CHANGE2, MKS_Language_NoChoose);
      settings.save();
      break;
    case MKS_English:
      DGUS_LanguageDisplay(MKS_English);
      mks_language_index = MKS_English;
      dgusdisplay.MKS_WriteVariable(VP_LANGUAGE_CHANGE1, MKS_Language_NoChoose);
      dgusdisplay.MKS_WriteVariable(VP_LANGUAGE_CHANGE2, MKS_Language_Choose);
      settings.save();
      break;
    default: break;
  }
}

#if ENABLED(MESH_BED_LEVELING)
  uint8_t mesh_point_count = GRID_MAX_POINTS;
#endif

void DGUSScreenHandler::Level_Ctrl_MKS(DGUS_VP_Variable &var, void *val_ptr) {
  char buf[30] = {0};
  const uint16_t lev_but = swap16(*(uint16_t *)val_ptr);
  #if ENABLED(MESH_BED_LEVELING)
    auto cs = getCurrentScreen();
  #endif
  switch (lev_but) {
    case 0:
      #if ENABLED(AUTO_BED_LEVELING_BILINEAR)

        static uint8_t a_first_level = 1;
        if (a_first_level == 1) {
          a_first_level = 0;
          queue.enqueue_now_P(G28_STR);
        }

      GotoScreen(MKSLCD_AUTO_LEVEL);

      // queue.inject_P(PSTR("M109 S140"));

      sprintf(buf, "M109 S%d", mks_AL_default_e0_temp);
      gcode.process_subcommands_now(PSTR(buf));

      //2-------G29中已开启G28   
      queue.inject_P(PSTR("G29"));

      #elif ENABLED(MESH_BED_LEVELING)
        mesh_point_count = GRID_MAX_POINTS;

        if (mks_language_index == MKS_English) {
          const char level_buf_en[] = "Start Level";
          dgusdisplay.WriteVariable(VP_AutoLevel_1_Dis, level_buf_en, 32, true);
        }
        else if (mks_language_index == MKS_SimpleChinese) {
          const uint16_t level_buf_ch[] = {0xAABF, 0xBCCA, 0xF7B5, 0xBDC6, 0x2000};
          dgusdisplay.WriteVariable(VP_AutoLevel_1_Dis, level_buf_ch, 32, true);
        }

        cs = getCurrentScreen();
        if (cs != MKSLCD_AUTO_LEVEL) GotoScreen(MKSLCD_AUTO_LEVEL);
      #else

        GotoScreen(MKSLCD_SCREEN_LEVEL);

      #endif
      break;

    case 1:
      soft_endstop._enabled = true;
      GotoScreen(MKSLCD_SCREEM_TOOL);
      break;

    default: break;
  }
}

void DGUSScreenHandler::MeshLevelDistanceConfig(DGUS_VP_Variable &var, void *val_ptr) {
  const uint16_t mesh_dist = swap16(*(uint16_t *)val_ptr);
  switch (mesh_dist) {
    case 0: mesh_adj_distance = 0.01; break;
    case 1: mesh_adj_distance = 0.1; break;
    case 2: mesh_adj_distance = 1; break;
    default: mesh_adj_distance = 0.1; break;
  }
  AUTO_ZOffset_distanceStep = mesh_dist;
}

void DGUSScreenHandler::MeshLevel(DGUS_VP_Variable &var, void *val_ptr) {
  char Ebuf[50];
  // #if ENABLED(MESH_BED_LEVELING)  AUTO_BED_LEVELING_BILINEAR
  #if ANY(MESH_BED_LEVELING, AUTO_BED_LEVELING_BILINEAR)
    const uint16_t mesh_value = swap16(*(uint16_t *)val_ptr);
    // static uint8_t a_first_level = 1;
    char cmd_buf[30];
    float offset = mesh_adj_distance;
    int16_t integer, Deci, Deci2;

    if (!queue.ring_buffer.empty()) return;

    switch (mesh_value) {
      case 0:
        offset = mesh_adj_distance;
        integer = offset; // get int
        Deci = (offset * 10);
        Deci = Deci % 10;
        Deci2 = offset * 100;
        Deci2 = Deci2 % 10;
        soft_endstop._enabled = false;
        //queue.enqueue_now(F("G91"));
        //snprintf_P(cmd_buf, 30, PSTR("G1 Z%d.%d%d"), integer, Deci, Deci2);
        snprintf_P(cmd_buf, 30, PSTR("M290 Z%d.%d%d"), integer, Deci, Deci2);
        queue.enqueue_one_now(cmd_buf);
        //queue.enqueue_now(F("G90"));
        //soft_endstop._enabled = true;

        break;

      case 1:
        offset = mesh_adj_distance;
        integer = offset;       // get int
        Deci = (offset * 10);
        Deci = Deci % 10;
        Deci2 = offset * 100;
        Deci2 = Deci2 % 10;
        soft_endstop._enabled = false;
        //queue.enqueue_now(F("G91"));
        //snprintf_P(cmd_buf, 30, PSTR("G1 Z-%d.%d%d"), integer, Deci, Deci2);
        snprintf_P(cmd_buf, 30, PSTR("M290 Z-%d.%d%d"), integer, Deci, Deci2);
        queue.enqueue_one_now(cmd_buf);
        //queue.enqueue_now(F("G90"));

        break;
#if ENABLED(MESH_BED_LEVELING)
      case 2:
        if (mesh_point_count == GRID_MAX_POINTS) { // The first point

          queue.enqueue_now(F("G28"));
          queue.enqueue_now(F("G29S1"));
          mesh_point_count--;

          if (mks_language_index == MKS_English) {
            const char level_buf_en1[] = "Next Point";
            dgusdisplay.WriteVariable(VP_AutoLevel_1_Dis, level_buf_en1, 32, true);
          }
          else if (mks_language_index == MKS_SimpleChinese) {
            const uint16_t level_buf_ch1[] = {0xC2CF, 0xBBD2, 0xE3B5, 0x2000};
            dgusdisplay.WriteVariable(VP_AutoLevel_1_Dis, level_buf_ch1, 32, true);
          }
        }
        else if (mesh_point_count > 1) {                              // 倒数第二个点
          queue.enqueue_now(F("G29S2"));
          mesh_point_count--;
          if (mks_language_index == MKS_English) {
            const char level_buf_en2[] = "Next Point";
            dgusdisplay.WriteVariable(VP_AutoLevel_1_Dis, level_buf_en2, 32, true);
          }
          else if (mks_language_index == MKS_SimpleChinese) {
            const uint16_t level_buf_ch2[] = {0xC2CF, 0xBBD2, 0xE3B5, 0x2000};
            dgusdisplay.WriteVariable(VP_AutoLevel_1_Dis, level_buf_ch2, 32, true);
          }
        }
        else if (mesh_point_count == 1) {
          queue.enqueue_now(F("G29S2"));
          mesh_point_count--;
          if (mks_language_index == MKS_English) {
            const char level_buf_en2[] = "Level Finsh";
            dgusdisplay.WriteVariable(VP_AutoLevel_1_Dis, level_buf_en2, 32, true);
          }
          else if (mks_language_index == MKS_SimpleChinese) {
            const uint16_t level_buf_ch2[] = {0xF7B5, 0xBDC6, 0xEACD, 0xC9B3, 0x2000};
            dgusdisplay.WriteVariable(VP_AutoLevel_1_Dis, level_buf_ch2, 32, true);
          }
          settings.save();
        }
        else if (mesh_point_count == 0) {
          mesh_point_count = GRID_MAX_POINTS;
          soft_endstop._enabled = true;
          settings.save();
          GotoScreen(MKSLCD_SCREEM_TOOL);
        }
        break;
      #endif
      case 3: 
      //if(Leveling_done_zhome_flg==1)
      if(Leveling_done_flg==1)
      {
        //Leveling_done_zhome_flg = 0;
        Leveling_done_flg = 0;     
        //queue.inject_P(PSTR("M500"));
        settings.save();
        #if 0
          ZERO(Ebuf);
          sprintf(Ebuf,"G1 E85 F%d", filamentSpeed_mm_s * 60);
          gcode.process_subcommands_now(PSTR(Ebuf));
          planner.synchronize();
        #endif
        queue.inject_P(PSTR("G28XY"));
        thermalManager.temp_hotend[0].target = 0;
        thermalManager.temp_bed.target = 0; 
        GotoScreen(MKSLCD_SCREEN_HOME);
      }
      break;
      default:
        break;
    }
  #endif // MESH_BED_LEVELING
}

void DGUSScreenHandler::SD_FileBack(DGUS_VP_Variable &var, void *val_ptr) {
  //GotoScreen(MKSLCD_SCREEN_HOME);
    uint16_t touched_nr = swap16(*(uint16_t*)val_ptr) ;
    
    if (touched_nr == 0x0F) {
      if (filelist.isAtRootDir()){
        GotoScreen(DGUSLCD_SCREEN_MAIN);
      }
      else
        filelist.upDir();
    }
}
void DGUSScreenHandler::Filament_Runout_Comfirm(DGUS_VP_Variable&, void*) {
  GotoScreen(MKSLCD_SCREEN_PAUSE);
  runout_mks.beeper_flg = 0;
}

void DGUSScreenHandler::LCD_BLK_Adjust(DGUS_VP_Variable &var, void *val_ptr) {
  const uint16_t lcd_value = swap16(*(uint16_t *)val_ptr);

  lcd_default_light = constrain(lcd_value, 10, 100);

  const uint16_t lcd_data[2] = { lcd_default_light, lcd_default_light };
  dgusdisplay.WriteVariable(0x0082, &lcd_data, 5, true);
}

void DGUSScreenHandler::ManualAssistLeveling(DGUS_VP_Variable &var, void *val_ptr) {
  const int16_t point_value = swap16(*(uint16_t *)val_ptr);

  // Insist on leveling first time at this screen
  static bool first_level_flag = false;
  if (!first_level_flag || point_value == 0x0001) {
    queue.enqueue_now_P(G28_STR);
    first_level_flag = true;
  }

  constexpr uint16_t level_speed = 1500;

  auto enqueue_corner_move = [](int16_t lx, int16_t ly, uint16_t fr) {
    char buf_level[32];
    sprintf_P(buf_level, "G0X%dY%dF%d", lx, ly, fr);
    queue.enqueue_one_now(buf_level);
  };

  if (WITHIN(point_value, 0x0001, 0x0005))
    queue.enqueue_now(F("G1Z10"));

  switch (point_value) {
    case 0x0001:
      enqueue_corner_move(X_MIN_POS + ABS(mks_corner_offsets[0].x),
                          Y_MIN_POS + ABS(mks_corner_offsets[0].y), level_speed);
      queue.enqueue_now(F("G28Z"));
      break;
    case 0x0002:
      enqueue_corner_move(X_MAX_POS - ABS(mks_corner_offsets[1].x),
                          Y_MIN_POS + ABS(mks_corner_offsets[1].y), level_speed);
      break;
    case 0x0003:
      enqueue_corner_move(X_MAX_POS - ABS(mks_corner_offsets[2].x),
                          Y_MAX_POS - ABS(mks_corner_offsets[2].y), level_speed);
      break;
    case 0x0004:
      enqueue_corner_move(X_MIN_POS + ABS(mks_corner_offsets[3].x),
                          Y_MAX_POS - ABS(mks_corner_offsets[3].y), level_speed);
      break;
    case 0x0005:
      enqueue_corner_move(ABS(mks_corner_offsets[4].x),
                          ABS(mks_corner_offsets[4].y), level_speed);
      break;
  }

  if (WITHIN(point_value, 0x0002, 0x0005)) {
    //queue.enqueue_now(F("G28Z"));
    queue.enqueue_now(F("G1Z-10"));
  }
}

#define mks_min(a, b) ((a) < (b)) ? (a) : (b)
#define mks_max(a, b) ((a) > (b)) ? (a) : (b)
void DGUSScreenHandler::TMC_ChangeConfig(DGUS_VP_Variable &var, void *val_ptr) {
  #if EITHER(HAS_TRINAMIC_CONFIG, HAS_STEALTHCHOP)
    const uint16_t tmc_value = swap16(*(uint16_t*)val_ptr);
  #endif

  switch (var.VP) {
    case VP_TMC_X_STEP:
      #if USE_SENSORLESS
        #if X_HAS_STEALTHCHOP
          stepperX.homing_threshold(mks_min(tmc_value, 255));
          settings.save();
          //tmc_step.x = stepperX.homing_threshold();
        #endif
      #endif
      break;
    case VP_TMC_Y_STEP:
      #if USE_SENSORLESS
        #if Y_HAS_STEALTHCHOP
          stepperY.homing_threshold(mks_min(tmc_value, 255));
          settings.save();
          //tmc_step.y = stepperY.homing_threshold();
        #endif
      #endif
      break;
    case VP_TMC_Z_STEP:
      #if USE_SENSORLESS
        #if Z_HAS_STEALTHCHOP
          stepperZ.homing_threshold(mks_min(tmc_value, 255));
          settings.save();
          //tmc_step.z = stepperZ.homing_threshold();
        #endif
      #endif
      break;
    case VP_TMC_X_Current:
      #if AXIS_IS_TMC(X)
        stepperX.rms_current(tmc_value);
        settings.save();
      #endif
      break;
    case VP_TMC_X1_Current:
      #if AXIS_IS_TMC(X2)
        stepperX2.rms_current(tmc_value);
        settings.save();
      #endif
      break;
    case VP_TMC_Y_Current:
      #if AXIS_IS_TMC(Y)
        stepperY.rms_current(tmc_value);
        settings.save();
      #endif
      break;
    case VP_TMC_Y1_Current:
      #if AXIS_IS_TMC(X2)
        stepperY2.rms_current(tmc_value);
        settings.save();
      #endif
      break;
    case VP_TMC_Z_Current:
      #if AXIS_IS_TMC(Z)
        stepperZ.rms_current(tmc_value);
        settings.save();
      #endif
      break;
    case VP_TMC_Z1_Current:
      #if AXIS_IS_TMC(Z2)
        stepperZ2.rms_current(tmc_value);
        settings.save();
      #endif
      break;
    case VP_TMC_E0_Current:
      #if AXIS_IS_TMC(E0)
        stepperE0.rms_current(tmc_value);
        settings.save();
      #endif
      break;
    case VP_TMC_E1_Current:
      #if AXIS_IS_TMC(E1)
        stepperE1.rms_current(tmc_value);
        settings.save();
      #endif
      break;

    default:
      break;
  }
  #if USE_SENSORLESS
    TERN_(X_HAS_STEALTHCHOP, tmc_step.x = stepperX.homing_threshold());
    TERN_(Y_HAS_STEALTHCHOP, tmc_step.y = stepperY.homing_threshold());
    TERN_(Z_HAS_STEALTHCHOP, tmc_step.z = stepperZ.homing_threshold());
  #endif
}

void DGUSScreenHandler::HandleManualMove(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("HandleManualMove");

  int16_t movevalue = swap16(*(uint16_t*)val_ptr);

  // Choose Move distance
       if (manualMoveStep == 0x01) manualMoveStep =   10;
  else if (manualMoveStep == 0x02) manualMoveStep =  100;
  else if (manualMoveStep == 0x03) manualMoveStep = 1000;

  DEBUG_ECHOLNPGM("QUEUE LEN:", queue.length);

  if (!print_job_timer.isPaused() && !queue.ring_buffer.empty())
    return;

  char axiscode;
  unsigned int speed = 1500; // FIXME: get default feedrate for manual moves, don't hardcode.

  switch (var.VP) { // switch X Y Z or Home
    default: return;
    case VP_MOVE_X:
      DEBUG_ECHOLNPGM("X Move");
      axiscode = 'X';
      if (!ExtUI::canMove(ExtUI::axis_t::X)) goto cannotmove;
      break;

    case VP_MOVE_Y:
      DEBUG_ECHOLNPGM("Y Move");
      axiscode = 'Y';
      if (!ExtUI::canMove(ExtUI::axis_t::Y)) goto cannotmove;
      break;

    case VP_MOVE_Z:
      DEBUG_ECHOLNPGM("Z Move");
      axiscode = 'Z';
      speed = 300; // default to 5mm/s
      if (!ExtUI::canMove(ExtUI::axis_t::Z)) goto cannotmove;
      break;

    case VP_MOTOR_LOCK_UNLOK:
      DEBUG_ECHOLNPGM("Motor Unlock");
      movevalue = 5;
      axiscode  = '\0';
      // return ;
      break;

    case VP_HOME_ALL: // only used for homing
      DEBUG_ECHOLNPGM("Home all");
      axiscode  = '\0';
      movevalue = 0; // ignore value sent from display, this VP is _ONLY_ for homing.
      //return;
      break;

    case VP_X_HOME:
      DEBUG_ECHOLNPGM("X Home");
      axiscode  = 'X';
      movevalue = 0;
      break;

    case VP_Y_HOME:
      DEBUG_ECHOLNPGM("Y Home");
      axiscode  = 'Y';
      movevalue = 0;
      break;

    case VP_Z_HOME:
      DEBUG_ECHOLNPGM("Z Home");
      axiscode  = 'Z';
      movevalue = 0;
      break;
  }

  DEBUG_ECHOPGM("movevalue = ", movevalue);
  if (movevalue != 0 && movevalue != 5) { // get move distance
    switch (movevalue) {
      case 0x0001: movevalue =  manualMoveStep; break;
      case 0x0002: movevalue = -manualMoveStep; break;
      default:     movevalue = 0; break;
    }
  }

  if (!movevalue) {
    // homing
    DEBUG_ECHOPGM(" homing ", AS_CHAR(axiscode));
    // char buf[6] = "G28 X";
    // buf[4] = axiscode;

    char buf[6];
    sprintf(buf, "G28 %c", axiscode);
    //DEBUG_ECHOPGM(" ", buf);
    queue.enqueue_one_now(buf);
    //DEBUG_ECHOLNPGM(" ✓");
    ForceCompleteUpdate();
    return;
  }
  else if (movevalue == 5) {
    DEBUG_ECHOPGM("send M84");
    char buf[6];
    snprintf_P(buf,6,PSTR("M84 %c"), axiscode);
    queue.enqueue_one_now(buf);
    ForceCompleteUpdate();
    return;
  }
  else {
    // movement
    DEBUG_ECHOPGM(" move ", AS_CHAR(axiscode));
    bool old_relative_mode = relative_mode;

    if (!relative_mode) {
      //DEBUG_ECHOPGM(" G91");
      queue.enqueue_now(F("G91"));
      //DEBUG_ECHOPGM(" ✓ ");
    }
    char buf[32]; // G1 X9999.99 F12345
    // unsigned int backup_speed = MMS_TO_MMM(feedrate_mm_s);
    char sign[] = "\0";
    int16_t value = movevalue / 100;
    if (movevalue < 0) { value = -value; sign[0] = '-'; }
    int16_t fraction = ABS(movevalue) % 100;
    snprintf_P(buf, 32, PSTR("G0 %c%s%d.%02d F%d"), axiscode, sign, value, fraction, speed);
    queue.enqueue_one_now(buf);

    //if (backup_speed != speed) {
    //  snprintf_P(buf, 32, PSTR("G0 F%d"), backup_speed);
    //  queue.enqueue_one_now(buf);
    //  //DEBUG_ECHOPGM(" ", buf);
    //}

    //while (!enqueue_and_echo_command(buf)) idle();
    //DEBUG_ECHOLNPGM(" ✓ ");
    if (!old_relative_mode) {
      //DEBUG_ECHOPGM("G90");
      //queue.enqueue_now(F("G90"));
      queue.enqueue_now(F("G90"));
      //DEBUG_ECHOPGM(" ✓ ");
    }
  }

  ForceCompleteUpdate();
  DEBUG_ECHOLNPGM("manmv done.");
  return;

  cannotmove:
    DEBUG_ECHOLNPGM(" cannot move ", AS_CHAR(axiscode));
    return;
}

void DGUSScreenHandler::GetParkPos_MKS(DGUS_VP_Variable &var, void *val_ptr) {
  const int16_t value_pos = swap16(*(int16_t*)val_ptr);

  switch (var.VP) {
    case VP_X_PARK_POS: mks_park_pos.x = value_pos; break;
    case VP_Y_PARK_POS: mks_park_pos.y = value_pos; break;
    case VP_Z_PARK_POS: mks_park_pos.z = value_pos; break;
    default: break;
  }
  skipVP = var.VP; // don't overwrite value the next update time as the display might autoincrement in parallel
}

void DGUSScreenHandler::HandleChangeLevelPoint_MKS(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("HandleChangeLevelPoint_MKS");

  const int16_t value_raw = swap16(*(int16_t*)val_ptr);
  DEBUG_ECHOLNPGM("value_raw:", value_raw);

  *(int16_t*)var.memadr = value_raw;

  settings.save();
  skipVP = var.VP; // don't overwrite value the next update time as the display might autoincrement in parallel
}

void DGUSScreenHandler::HandleManualE0_T(DGUS_VP_Variable &var, void *val_ptr) 
{
	DEBUG_ECHOLNPGM("HandleManualE0");

    const int16_t value = swap16(*(int16_t*)val_ptr);
    int16_t manualE0_T_Step_temp;
    // Choose E0_T  STEP
    if (manualE0_T_Step == 0x00) manualE0_T_Step_temp =   1;
    else if (manualE0_T_Step == 0x01) manualE0_T_Step_temp =  5;
    else if (manualE0_T_Step == 0x02) manualE0_T_Step_temp = 10;	
	
	switch(value)
	{
		case 0:
			thermalManager.temp_hotend[0].target -= manualE0_T_Step_temp;
			if(thermalManager.temp_hotend[0].target<=0)
				thermalManager.temp_hotend[0].target = 0;
			break;
		case 1:
			thermalManager.temp_hotend[0].target += manualE0_T_Step_temp;
			if(thermalManager.temp_hotend[0].target>=HEATER_0_MAXTEMP)
				thermalManager.temp_hotend[0].target = HEATER_0_MAXTEMP;
			break;
	    case 2:
			thermalManager.temp_hotend[0].target = 0;
			break;
		default:
			break;
	}
}
void DGUSScreenHandler::HandleManualHotB_T(DGUS_VP_Variable &var, void *val_ptr) 
{
	DEBUG_ECHOLNPGM("HandleManualHotB_T");

    const int16_t value = swap16(*(int16_t*)val_ptr);
    int16_t manualHotB_T_Step_temp;
    // Choose HotB_T  STEP
    if (manualHotB_T_Step == 0x00) manualHotB_T_Step_temp =   1;
    else if (manualHotB_T_Step == 0x01) manualHotB_T_Step_temp =  5;
    else if (manualHotB_T_Step == 0x02) manualHotB_T_Step_temp = 10;	
	
	switch(value)
	{
		case 0:
			thermalManager.temp_bed.target -= manualHotB_T_Step_temp;
			if(thermalManager.temp_bed.target<=0)
				thermalManager.temp_bed.target = 0;
			break;
		case 1:
			thermalManager.temp_bed.target += manualHotB_T_Step_temp;
			if(thermalManager.temp_bed.target>=BED_MAXTEMP)
				thermalManager.temp_bed.target = BED_MAXTEMP;
			break;
	    case 2:
			thermalManager.temp_bed.target = 0;
			break;
		default:
			break;
	}
}

void DGUSScreenHandler::HandleSetFan_MKS(DGUS_VP_Variable &var, void *val_ptr) {

  DEBUG_ECHOLNPGM("HandleSetFan_MKS");

  const int16_t value = swap16(*(int16_t*)val_ptr);
  uint16_t get_fan_speed = 0;

  switch(value) {

    case 0x00: get_fan_speed = 25;  break;
    case 0x01: get_fan_speed = 51;  break;
    case 0x02: get_fan_speed = 76;  break;
    case 0x03: get_fan_speed = 102; break;
    case 0x04: get_fan_speed = 128; break;
    case 0x05: get_fan_speed = 153; break;
    case 0x06: get_fan_speed = 179; break;
    case 0x07: get_fan_speed = 204; break;
    case 0x08: get_fan_speed = 230; break;
    case 0x09: get_fan_speed = 255; break;
  }

  // thermalManager.set_fan_speed(0, value);
  thermalManager.set_fan_speed(0, get_fan_speed);
}
void DGUSScreenHandler::GetLanguage_MKS(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("\nGetDefault_Temp_MKS");
  *(uint16_t *)var.memadr = swap16(*(uint16_t *)val_ptr);
}

void DGUSScreenHandler::GetDefault_Temp_MKS(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("\nGetDefault_Temp_MKS");
  *(uint16_t *)var.memadr = swap16(*(uint16_t *)val_ptr);
}

void DGUSScreenHandler::HandleSetPLAorABS_SAVE(DGUS_VP_Variable &var, void *val_ptr) {
  const int16_t value = swap16(*(int16_t*)val_ptr);

  DEBUG_ECHOLNPGM("value:", value);

  switch(value) {
    // set pla 
    case 0: 
	    //settings.save();
	    queue.inject_P(PSTR("M500"));
      GotoScreen(MKS_LCD_PREHEAT_DEFAULT);
    break;

    // set abs
    case 1: 
		//settings.save();
		queue.inject_P(PSTR("M500"));
    GotoScreen(MKS_LCD_PREHEAT_DEFAULT);
    break;
    case 2: 
		//settings.save();
		queue.inject_P(PSTR("M500"));
    GotoScreen(MKS_LCD_PREHEAT_DEFAULT);
    break;
  }

}
void DGUSScreenHandler::HandleLanguage_SAVE(DGUS_VP_Variable &var, void *val_ptr) {
	queue.inject_P(PSTR("M500"));
  GotoScreen(MKSLCD_SCREEN_SETTING);
}

void DGUSScreenHandler::HandleSetPLAorABS_MKS(DGUS_VP_Variable &var, void *val_ptr) {
  const int16_t value = swap16(*(int16_t*)val_ptr);

  celsius_t  e_temp = 0;
  celsius_t  b_temp = 0;

  DEBUG_ECHOLNPGM("value:", value);

  switch(value) {
    // set pla 
    case 1: 
      e_temp = mks_PLA_default_e0_temp;
#if HAS_HEATED_BED
      b_temp = mks_PLA_default_bed_temp;
#endif
    break;

    // set abs
    case 2: 
      e_temp = mks_ABS_default_e0_temp;
#if HAS_HEATED_BED
      b_temp = mks_ABS_default_bed_temp;
#endif
    break;

    case 3: 
      thermalManager.temp_hotend[0].target = 0;
      return;
    break;

    case 4: 
      thermalManager.temp_bed.target = 0;
      return;
    break;
  }

  thermalManager.temp_hotend[0].target = e_temp;
#if HAS_HEATED_BED
  thermalManager.temp_bed.target = b_temp;
#endif

}


void DGUSScreenHandler::HandleStepPerMMChanged_MKS(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("HandleStepPerMMChanged_MKS");

  const uint16_t value_raw = swap16(*(uint16_t*)val_ptr);
  const float value = (float)value_raw;

  DEBUG_ECHOLNPGM("value_raw:", value_raw);
  DEBUG_ECHOLNPGM("value:", value);

  ExtUI::axis_t axis;
  switch (var.VP) {
    default: return;
    case VP_X_STEP_PER_MM: axis = ExtUI::axis_t::X; break;
    case VP_Y_STEP_PER_MM: axis = ExtUI::axis_t::Y; break;
    case VP_Z_STEP_PER_MM: axis = ExtUI::axis_t::Z; break;
  }
  ExtUI::setAxisSteps_per_mm(value, axis);
  DEBUG_ECHOLNPGM("value_set:", ExtUI::getAxisSteps_per_mm(axis));
  settings.save();
  skipVP = var.VP; // don't overwrite value the next update time as the display might autoincrement in parallel
}

void DGUSScreenHandler::HandleStepPerMMExtruderChanged_MKS(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("HandleStepPerMMExtruderChanged_MKS");

  const uint16_t value_raw = swap16(*(uint16_t*)val_ptr);
  const float value = (float)value_raw;

  DEBUG_ECHOLNPGM("value_raw:", value_raw);
  DEBUG_ECHOLNPGM("value:", value);

  ExtUI::extruder_t extruder;
  switch (var.VP) {
    default: return;
    #if HAS_HOTEND
      case VP_E0_STEP_PER_MM: extruder = ExtUI::extruder_t::E0; break;
    #endif
    #if HAS_MULTI_HOTEND
      case VP_E1_STEP_PER_MM: extruder = ExtUI::extruder_t::E1; break;
    #endif
  }
  ExtUI::setAxisSteps_per_mm(value, extruder);
  DEBUG_ECHOLNPGM("value_set:", ExtUI::getAxisSteps_per_mm(extruder));
  settings.save();
  skipVP = var.VP; // don't overwrite value the next update time as the display might autoincrement in parallel
}

void DGUSScreenHandler::HandleMaxSpeedChange_MKS(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("HandleMaxSpeedChange_MKS");

  const uint16_t value_raw = swap16(*(uint16_t*)val_ptr);
  const float value = (float)value_raw;

  DEBUG_ECHOLNPGM("value_raw:", value_raw);
  DEBUG_ECHOLNPGM("value:", value);

  ExtUI::axis_t axis;
  switch (var.VP) {
    case VP_X_MAX_SPEED: axis = ExtUI::axis_t::X; break;
    case VP_Y_MAX_SPEED: axis = ExtUI::axis_t::Y; break;
    case VP_Z_MAX_SPEED: axis = ExtUI::axis_t::Z; break;
    default: return;
  }
  ExtUI::setAxisMaxFeedrate_mm_s(value, axis);
  DEBUG_ECHOLNPGM("value_set:", ExtUI::getAxisMaxFeedrate_mm_s(axis));
  settings.save();
  skipVP = var.VP; // don't overwrite value the next update time as the display might autoincrement in parallel
}

void DGUSScreenHandler::HandleExtruderMaxSpeedChange_MKS(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("HandleExtruderMaxSpeedChange_MKS");

  const uint16_t value_raw = swap16(*(uint16_t*)val_ptr);
  const float value = (float)value_raw;

  DEBUG_ECHOLNPGM("value_raw:", value_raw);
  DEBUG_ECHOLNPGM("value:", value);

  ExtUI::extruder_t extruder;
  switch (var.VP) {
    default: return;
      #if HAS_HOTEND
        case VP_E0_MAX_SPEED: extruder = ExtUI::extruder_t::E0; break;
      #endif
      #if HAS_MULTI_HOTEND
      #endif
    case VP_E1_MAX_SPEED: extruder = ExtUI::extruder_t::E1; break;
  }
  ExtUI::setAxisMaxFeedrate_mm_s(value, extruder);
  DEBUG_ECHOLNPGM("value_set:", ExtUI::getAxisMaxFeedrate_mm_s(extruder));
  settings.save();
  skipVP = var.VP; // don't overwrite value the next update time as the display might autoincrement in parallel
}

void DGUSScreenHandler::HandleMaxAccChange_MKS(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("HandleMaxAccChange_MKS");

  const uint16_t value_raw = swap16(*(uint16_t*)val_ptr);
  const float value = (float)value_raw;

  DEBUG_ECHOLNPGM("value_raw:", value_raw);
  DEBUG_ECHOLNPGM("value:", value);

  ExtUI::axis_t axis;
  switch (var.VP) {
    default: return;
    case VP_X_ACC_MAX_SPEED: axis = ExtUI::axis_t::X;  break;
    case VP_Y_ACC_MAX_SPEED: axis = ExtUI::axis_t::Y;  break;
    case VP_Z_ACC_MAX_SPEED: axis = ExtUI::axis_t::Z;  break;
  }
  ExtUI::setAxisMaxAcceleration_mm_s2(value, axis);
  DEBUG_ECHOLNPGM("value_set:", ExtUI::getAxisMaxAcceleration_mm_s2(axis));
  settings.save();
  skipVP = var.VP; // don't overwrite value the next update time as the display might autoincrement in parallel
}

void DGUSScreenHandler::HandleExtruderAccChange_MKS(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("HandleExtruderAccChange_MKS");

  uint16_t value_raw = swap16(*(uint16_t*)val_ptr);
  DEBUG_ECHOLNPGM("value_raw:", value_raw);
  float value = (float)value_raw;
  ExtUI::extruder_t extruder;
  switch (var.VP) {
    default: return;
    #if HAS_HOTEND
      case VP_E0_ACC_MAX_SPEED: extruder = ExtUI::extruder_t::E0; settings.load(); break;
    #endif
    #if HAS_MULTI_HOTEND
      case VP_E1_ACC_MAX_SPEED: extruder = ExtUI::extruder_t::E1; settings.load(); break;
    #endif
  }
  DEBUG_ECHOLNPGM("value:", value);
  ExtUI::setAxisMaxAcceleration_mm_s2(value, extruder);
  DEBUG_ECHOLNPGM("value_set:", ExtUI::getAxisMaxAcceleration_mm_s2(extruder));
  settings.save();
  skipVP = var.VP; // don't overwrite value the next update time as the display might autoincrement in parallel
}

void DGUSScreenHandler::HandleTravelAccChange_MKS(DGUS_VP_Variable &var, void *val_ptr) {
  uint16_t value_travel = swap16(*(uint16_t*)val_ptr);
  planner.settings.travel_acceleration = (float)value_travel;
  skipVP = var.VP; // don't overwrite value the next update time as the display might autoincrement in parallel
}

void DGUSScreenHandler::HandleFeedRateMinChange_MKS(DGUS_VP_Variable &var, void *val_ptr) {
  uint16_t value_t = swap16(*(uint16_t*)val_ptr);
  planner.settings.min_feedrate_mm_s = (float)value_t;
  skipVP = var.VP; // don't overwrite value the next update time as the display might autoincrement in parallel
}

void DGUSScreenHandler::HandleMin_T_F_MKS(DGUS_VP_Variable &var, void *val_ptr) {
  uint16_t value_t_f = swap16(*(uint16_t*)val_ptr);
  planner.settings.min_travel_feedrate_mm_s = (float)value_t_f;
  skipVP = var.VP; // don't overwrite value the next update time as the display might autoincrement in parallel
}

void DGUSScreenHandler::HandleAccChange_MKS(DGUS_VP_Variable &var, void *val_ptr) {
  uint16_t value_acc = swap16(*(uint16_t*)val_ptr);
  planner.settings.acceleration = (float)value_acc;
  skipVP = var.VP; // don't overwrite value the next update time as the display might autoincrement in parallel
}

#if ENABLED(PREVENT_COLD_EXTRUSION)
  void DGUSScreenHandler::HandleGetExMinTemp_MKS(DGUS_VP_Variable &var, void *val_ptr) {
    const uint16_t value_ex_min_temp = swap16(*(uint16_t*)val_ptr);
    thermalManager.extrude_min_temp = value_ex_min_temp;
    skipVP = var.VP; // don't overwrite value the next update time as the display might autoincrement in parallel
  }
#endif

#if HAS_PID_HEATING
  void DGUSScreenHandler::HandleTemperaturePIDChanged(DGUS_VP_Variable &var, void *val_ptr) {
    const uint16_t rawvalue = swap16(*(uint16_t*)val_ptr);
    DEBUG_ECHOLNPGM("V1:", rawvalue);
    const float value = 1.0f * rawvalue;
    DEBUG_ECHOLNPGM("V2:", value);
    float newvalue = 0;

    switch (var.VP) {
      default: return;
        #if HAS_HOTEND
          case VP_E0_PID_P: newvalue = value; break;
          case VP_E0_PID_I: newvalue = scalePID_i(value); break;
          case VP_E0_PID_D: newvalue = scalePID_d(value); break;
        #endif
        #if HAS_MULTI_HOTEND
          case VP_E1_PID_P: newvalue = value; break;
          case VP_E1_PID_I: newvalue = scalePID_i(value); break;
          case VP_E1_PID_D: newvalue = scalePID_d(value); break;
        #endif
        #if HAS_HEATED_BED
          case VP_BED_PID_P: newvalue = value; break;
          case VP_BED_PID_I: newvalue = scalePID_i(value); break;
          case VP_BED_PID_D: newvalue = scalePID_d(value); break;
        #endif
    }

    DEBUG_ECHOLNPGM("V3:", newvalue);
    *(float *)var.memadr = newvalue;

    settings.save();
    skipVP = var.VP; // don't overwrite value the next update time as the display might autoincrement in parallel
  }
#endif // HAS_PID_HEATING

#if ENABLED(BABYSTEPPING)
  void DGUSScreenHandler::HandleLiveAdjustZ(DGUS_VP_Variable &var, void *val_ptr) {
    DEBUG_ECHOLNPGM("HandleLiveAdjustZ");
    // char babystep_buf[30];
    float step = ZOffset_distance;

    uint16_t flag = swap16(*(uint16_t*)val_ptr);
    switch (flag) {
      case 0:
        if (step == 1)
          queue.inject(F("M290 Z-0.01"));
        else if (step == 10)
          queue.inject(F("M290 Z-0.1"));
        else if (step == 50)
          queue.inject(F("M290 Z-0.5"));
        else if (step == 100)
          queue.inject(F("M290 Z-1"));
        else
          queue.inject(F("M290 Z-0.01"));

        z_offset_add = z_offset_add - ZOffset_distance;
        break;

      case 1:
        if (step == 1)
          queue.inject(F("M290 Z0.01"));
        else if (step == 10)
          queue.inject(F("M290 Z0.1"));
        else if (step == 50)
          queue.inject(F("M290 Z0.5"));
        else if (step == 100)
          queue.inject(F("M290 Z1"));
        else
          queue.inject(F("M290 Z-0.01"));

        z_offset_add = z_offset_add + ZOffset_distance;
        break;

      default:
        break;
    }
    
    ForceCompleteUpdate();
  }
#endif // BABYSTEPPING

void DGUSScreenHandler::GetManualFilament(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("GetManualFilament");

  uint16_t value_len = swap16(*(uint16_t*)val_ptr);

  float value = (float)value_len;

  DEBUG_ECHOLNPGM("Get Filament len value:", value);
  distanceFilament = value;
  
  if(distanceFilament>100)distanceFilament=100;//max length->100

  skipVP = var.VP; // don't overwrite value the next update time as the display might autoincrement in parallel
}

void DGUSScreenHandler::GetManualFilamentSpeed(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("GetManualFilamentSpeed");

  uint16_t value_len = swap16(*(uint16_t*)val_ptr);

  DEBUG_ECHOLNPGM("filamentSpeed_mm_s value:", value_len);

  filamentSpeed_mm_s = value_len;
  if(filamentSpeed_mm_s>10)filamentSpeed_mm_s=10;//max speed->10

  skipVP = var.VP; // don't overwrite value the next update time as the display might autoincrement in parallel
}

void DGUSScreenHandler::MKS_FilamentLoadUnload(DGUS_VP_Variable &var, void *val_ptr, const int filamentDir) {
  #if EITHER(HAS_MULTI_HOTEND, SINGLENOZZLE)
    uint8_t swap_tool = 0;
  #else
    constexpr uint8_t swap_tool = 1; // T0 (or none at all)
  #endif

  #if HAS_HOTEND
    uint8_t hotend_too_cold = 0;
  #endif

  if (!print_job_timer.isPaused() && !queue.ring_buffer.empty())
    return;

  const uint16_t val_t = swap16(*(uint16_t*)val_ptr);
  switch (val_t) {
    default: break;
    case 0:
      #if HAS_HOTEND
        if (thermalManager.tooColdToExtrude(0))
          hotend_too_cold = 1;
        else {
          #if EITHER(HAS_MULTI_HOTEND, SINGLENOZZLE)
            swap_tool = 1;
          #endif
        }
      #endif
      break;
    case 1:
      #if HAS_MULTI_HOTEND
        if (thermalManager.tooColdToExtrude(1)) hotend_too_cold = 2; else swap_tool = 2;
      #elif ENABLED(SINGLENOZZLE)
        if (thermalManager.tooColdToExtrude(0)) hotend_too_cold = 1; else swap_tool = 2;
      #endif
      break;
  }

  #if BOTH(HAS_HOTEND, PREVENT_COLD_EXTRUSION)
    if (hotend_too_cold) {
      if (thermalManager.targetTooColdToExtrude(hotend_too_cold - 1)) 
         thermalManager.setTargetHotend(thermalManager.extrude_min_temp, hotend_too_cold - 1);
      // sendinfoscreen(F("NOTICE"), nullptr, F("Please wait."), F("Nozzle heating!"), true, true, true, true);
      // SetupConfirmAction(nullptr);
      // GotoScreen(DGUSLCD_SCREEN_POPUP);
          GotoScreen(MKSLCD_Screen_EXTRUDE_HEATING_POPUP);
          filament_data.heating = true;
          if(filamentDir==1)filament_data.action = 1;
          else filament_data.action = 0;
    }
  #endif

  if (swap_tool) {
    char buf[30];
    snprintf_P(buf, 30
      #if EITHER(HAS_MULTI_HOTEND, SINGLENOZZLE)
        , PSTR("M1002T%cE%dF%d"), char('0' + swap_tool - 1)
      #else
        , PSTR("M1002E%dF%d")
      #endif
      , (int)distanceFilament * filamentDir, filamentSpeed_mm_s * 60
    );
    queue.inject(buf);
  }
}

/**
 * M1002: Do a tool-change and relative move for MKS_FilamentLoadUnload
 *        within the G-code execution window for best concurrency.
 */
void GcodeSuite::M1002() {
  #if EITHER(HAS_MULTI_HOTEND, SINGLENOZZLE)
  {
    char buf[3];
    sprintf_P(buf, PSTR("T%c"), char('0' + parser.intval('T')));
    process_subcommands_now(buf);
  }
  #endif

  const uint8_t old_axis_relative = axis_relative;
  set_e_relative(); // M83
  {
    char buf[20];
    snprintf_P(buf, 20, PSTR("G1E%dF%d"), parser.intval('E'), parser.intval('F'));
    process_subcommands_now(buf);
  }
  axis_relative = old_axis_relative;
}

void DGUSScreenHandler::MKS_PrintFilamentLoad_Confirm(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("MKS_PrintFilamentLoad_Confirm");
  uint16_t value = swap16(*(uint16_t*)val_ptr);
  switch(value)
  {
    case 0:
        GotoScreen(MKSLCD_Screen_PRINT_FLAMENT);
        if (!ExtUI::isPrintingFromMediaPaused()) {
          nozzle_park_mks.print_pause_start_flag = 1;
          nozzle_park_mks.blstatus = true;
          ExtUI::pausePrint();
          loadfilament_confirm_flg = true;

          print_set_from = 1;
        }     
    break;
    case 1:
      GotoScreen(MKSLCD_Screen_PRINT_FLAMENT);
    break;
  }
}
void DGUSScreenHandler::MKS_PrintFilamentUnLoad_Confirm(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("MKS_PrintFilamentLoad_Confirm");
  uint16_t value = swap16(*(uint16_t*)val_ptr);
  switch(value)
  {
    case 0:
        GotoScreen(MKSLCD_Screen_PRINT_FLAMENT);
        if (!ExtUI::isPrintingFromMediaPaused()) {
          nozzle_park_mks.print_pause_start_flag = 1;
          nozzle_park_mks.blstatus = true;
          ExtUI::pausePrint();
          unloadfilament_confirm_flg = true;

          print_set_from = 1;
        }     
    break;
    case 1:
      GotoScreen(MKSLCD_Screen_PRINT_FLAMENT);
    break;
  }
}
void DGUSScreenHandler::MKS_PrintFilamentLoad(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("MKS_PrintFilamentLoad");
  if (!print_job_timer.isPaused() && !queue.ring_buffer.empty())
  {
    GotoScreen(MKSLCD_Screen_PRINT_FLAMENT_LOAD);
    return;
  } 
  const float olde = current_position.e;
  current_position.e += PRINT_FILAMENT_LENGTH;
  line_to_current_position(MMM_TO_MMS(PRINT_FILAMENT_SPEED));
  current_position.e = olde;
  planner.set_e_position_mm(olde);
  planner.synchronize();
}

void DGUSScreenHandler::MKS_PrintFilamentUnLoad(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("MKS_PrintFilamentUnLoad");
  if (!print_job_timer.isPaused() && !queue.ring_buffer.empty())
  {
    GotoScreen(MKSLCD_Screen_PRINT_FLAMENT_UNLOAD);
    return;
  }
  const float olde = current_position.e;
  current_position.e -= PRINT_FILAMENT_LENGTH;
  line_to_current_position(MMM_TO_MMS(PRINT_FILAMENT_SPEED));
  current_position.e = olde;
  planner.set_e_position_mm(olde);
  planner.synchronize();
}

void DGUSScreenHandler::MKS_FilamentCancelHeating(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("MKS_FilamentCancelHeating");
  thermalManager.setTargetHotend(0, 0);
  filament_data.heating = false;
  GotoScreen(MKSLCD_SCREEN_EXTRUDE_P1);
}

void DGUSScreenHandler::MKS_Extrude_load_popup(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("MKS_Extrude_handle");
  char buf[30];
  uint16_t value = swap16(*(uint16_t*)val_ptr);
  switch(value){
    case 0:
      filament_data.heating = false;
      snprintf_P(buf,30,PSTR("M1002E%dF%d"),(int)distanceFilament, filamentSpeed_mm_s * 60);
      queue.inject(buf);
      GotoScreen(MKSLCD_SCREEN_EXTRUDE_P1);
    break;
    case 1:
      filament_data.heating = false;
      quickstop_stepper();
      GotoScreen(MKSLCD_SCREEN_EXTRUDE_P1);

    break;
  }
}
void DGUSScreenHandler::MKS_Extrude_unload_popup(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("MKS_Extrude_handle");
  char buf[30];
  uint16_t value = swap16(*(uint16_t*)val_ptr);
  switch(value){
    case 0:
      filament_data.heating = false;
      snprintf_P(buf,30,PSTR("M1002E-%dF%d"),(int)distanceFilament, filamentSpeed_mm_s * 60);
      queue.inject(buf);
      GotoScreen(MKSLCD_SCREEN_EXTRUDE_P1);
    break;
    case 1:
      filament_data.heating = false;
      quickstop_stepper();
      GotoScreen(MKSLCD_SCREEN_EXTRUDE_P1);
    break;
  }
}
void DGUSScreenHandler::MKS_FilamentLoad(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("MKS_FilamentLoad");
     MKS_FilamentLoadUnload(var, val_ptr, 1);
}

void DGUSScreenHandler::MKS_FilamentUnLoad(DGUS_VP_Variable &var, void *val_ptr) {
  DEBUG_ECHOLNPGM("MKS_FilamentUnLoad");
    MKS_FilamentLoadUnload(var, val_ptr, -1);
}

#if ENABLED(DGUS_FILAMENT_LOADUNLOAD)

  void DGUSScreenHandler::HandleFilamentOption(DGUS_VP_Variable &var, void *val_ptr) {
    DEBUG_ECHOLNPGM("HandleFilamentOption");

    uint8_t e_temp = 0;
    filament_data.heated = false;
    uint16_t preheat_option = swap16(*(uint16_t*)val_ptr);
    if (preheat_option >= 10) {     // Unload filament type
      preheat_option -= 10;
      filament_data.action = 2;
      filament_data.purge_length = DGUS_FILAMENT_PURGE_LENGTH;
    }
    else if (preheat_option <= 8)   // Load filament type
      filament_data.action = 1;
    else                            // Cancel filament operation
      filament_data.action = 0;

    switch (preheat_option) {
      case 0: // Load PLA
        #ifdef PREHEAT_1_TEMP_HOTEND
          e_temp = PREHEAT_1_TEMP_HOTEND;
        #endif
        break;
      case 1: // Load ABS
        TERN_(PREHEAT_2_TEMP_HOTEND, e_temp = PREHEAT_2_TEMP_HOTEND);
        break;
      case 2: // Load PET
        #ifdef PREHEAT_3_TEMP_HOTEND
          e_temp = PREHEAT_3_TEMP_HOTEND;
        #endif
        break;
      case 3: // Load FLEX
        #ifdef PREHEAT_4_TEMP_HOTEND
          e_temp = PREHEAT_4_TEMP_HOTEND;
        #endif
        break;
      case 9: // Cool down
      default:
        e_temp = 0;
        break;
    }

    if (filament_data.action == 0) { // Go back to utility screen
      #if HAS_HOTEND
        thermalManager.setTargetHotend(e_temp, ExtUI::extruder_t::E0);
      #endif
      #if HAS_MULTI_HOTEND
        thermalManager.setTargetHotend(e_temp, ExtUI::extruder_t::E1);
      #endif
      GotoScreen(DGUSLCD_SCREEN_UTILITY);
    }
    else { // Go to the preheat screen to show the heating progress
      switch (var.VP) {
        default: return;
          #if HAS_HOTEND
            case VP_E0_FILAMENT_LOAD_UNLOAD:
              filament_data.extruder = ExtUI::extruder_t::E0;
              thermalManager.setTargetHotend(e_temp, filament_data.extruder);
              break;
          #endif
          #if HAS_MULTI_HOTEND
            case VP_E1_FILAMENT_LOAD_UNLOAD:
              filament_data.extruder = ExtUI::extruder_t::E1;
              thermalManager.setTargetHotend(e_temp, filament_data.extruder);
              break;
          #endif
      }
    }
  }

  void DGUSScreenHandler::HandleFilamentLoadUnload(DGUS_VP_Variable &var) {
    DEBUG_ECHOLNPGM("HandleFilamentLoadUnload");
    if (filament_data.action <= 0) return;

    // If we close to the target temperature, we can start load or unload the filament
    if (thermalManager.hotEnoughToExtrude(filament_data.extruder) && \
        thermalManager.targetHotEnoughToExtrude(filament_data.extruder)) {
      float movevalue = DGUS_FILAMENT_LOAD_LENGTH_PER_TIME;

      if (filament_data.action == 1) { // load filament
        if (!filament_data.heated) {
          filament_data.heated = true;
        }
        movevalue = ExtUI::getAxisPosition_mm(filament_data.extruder) + movevalue;
      }
      else { // unload filament
        if (!filament_data.heated) {
          GotoScreen(DGUSLCD_SCREEN_FILAMENT_UNLOADING);
          filament_data.heated = true;
        }
        // Before unloading extrude to prevent jamming
        if (filament_data.purge_length >= 0) {
          movevalue = ExtUI::getAxisPosition_mm(filament_data.extruder) + movevalue;
          filament_data.purge_length -= movevalue;
        }
        else {
          movevalue = ExtUI::getAxisPosition_mm(filament_data.extruder) - movevalue;
        }
      }
      ExtUI::setAxisPosition_mm(movevalue, filament_data.extruder);
    }
  }

#endif // DGUS_FILAMENT_LOADUNLOAD

// #define ERR_UNDISPLAY_ICON          (uint16_t)0   
// #define ERR_DISPLAY_ICON            (uint16_t)1  
void dgus_sd_read_err_disp(uint16_t on_off) {
  dgusdisplay.WriteVariable(VP_SD_read_err, on_off);
}


/*****************************************************************
 *  For MKS test
 * **************************************************************/
#if ENABLED(MKS_TEST) 

void mks_test_handler(void) {



}


#endif // MKS_TEST



bool DGUSScreenHandler::loop() {
  dgusdisplay.loop();

  const millis_t ms = millis();
  static millis_t next_event_ms = 0;
  uint16_t default_fan_icon_val = 1;
  static uint8_t language_times = 2;
  static char Ebuf[50];

  if (!IsScreenComplete() || ELAPSED(ms, next_event_ms)) {
    next_event_ms = ms + DGUS_UPDATE_INTERVAL_MS;
    UpdateScreenVPData();
  }

  if (language_times != 0) {
    mks_language_index = MKS_English;
    LanguagePInit();
    DGUS_LanguageDisplay(mks_language_index);
    language_times--;
  }

  if(level_temp_check() == 1) {
    GotoScreen(MKSLCD_AUTO_LEVEL);
    #if 0
      gcode.process_subcommands_now(PSTR("G91"));
      gcode.process_subcommands_now(PSTR("G1 Z5 F1000"));
      ZERO(Ebuf);
      sprintf(Ebuf,"G1 E20 F%d", filamentSpeed_mm_s * 60);
      gcode.process_subcommands_now(PSTR(Ebuf));
      ZERO(Ebuf);
      sprintf(Ebuf,"G1 E-100 F%d", filamentSpeed_mm_s * 60);
      gcode.process_subcommands_now(PSTR(Ebuf));
    #endif

    //2-------G29中已开启G28 
    queue.inject_P(PSTR("G29"));
  }


  #if ENABLED(SHOW_BOOTSCREEN)
    static bool booted = false;
    if (!booted && ELAPSED(ms, TERN(USE_MKS_GREEN_UI, 1000, BOOTSCREEN_TIMEOUT))) {
      booted = true;

      TERN_(MKS_TEST, mks_test_get());

      if (mks_test_flag == 0x1E) 
      {
        GotoScreen(MKSLCD_SCREEN_TEST);
        init_test_gpio();
        mks_language_index = MKS_English;
      }
      else{
        
      #if USE_SENSORLESS
        TERN_(X_HAS_STEALTHCHOP, tmc_step.x = stepperX.homing_threshold());
        TERN_(Y_HAS_STEALTHCHOP, tmc_step.y = stepperY.homing_threshold());
        TERN_(Z_HAS_STEALTHCHOP, tmc_step.z = stepperZ.homing_threshold());
      #endif

      #if ENABLED(PREVENT_COLD_EXTRUSION)
        if (mks_min_extrusion_temp != 0)
          thermalManager.extrude_min_temp = mks_min_extrusion_temp;
      #endif

      DGUS_ExtrudeLoadInit();
      
      TERN_(DGUS_MKS_RUNOUT_SENSOR, DGUS_RunoutInit());
      DGUS_Runout_init(); // 初始化断料检测
      
      queue.inject_P(PSTR("M84"));

      dgusdisplay.WriteVariable(VP_FAN_ON_OFF_VAL, default_fan_icon_val);
      dgus_sd_read_err_disp(0);

      dgusdisplay.WriteVariable(VP_ZNP_LANGUAGE, mks_language);
      dgusdisplay.WriteVariable(VP_DEFAULT_PLA_T_TEMP, mks_PLA_default_e0_temp);
      dgusdisplay.WriteVariable(VP_DEFAULT_PLA_B_TEMP, mks_PLA_default_bed_temp);
      dgusdisplay.WriteVariable(VP_DEFAULT_ABS_T_TEMP, mks_ABS_default_e0_temp);
      dgusdisplay.WriteVariable(VP_DEFAULT_ABS_B_TEMP, mks_ABS_default_bed_temp);
      dgusdisplay.WriteVariable(VP_DEFAULT_AL_T_TEMP, mks_AL_default_e0_temp);
      dgusdisplay.WriteVariable(VP_DEFAULT_AL_B_TEMP, mks_AL_default_bed_temp);
      dgusdisplay.WriteVariable(VP_SETTING_FILAMENT_DET, mks_filament_det_enable);


      if (TERN0(POWER_LOSS_RECOVERY, recovery.valid()))
      {
      	recovery_flg = true;
		    recovery_ms = millis();
        GotoScreen(DGUSLCD_SCREEN_POWER_LOSS);
      }
      else
      {
        GotoScreen(DGUSLCD_SCREEN_MAIN);
      }
   
      }
    }

      if (mks_test_flag == 0x1E) {
        mks_gpio_test();
        mks_hardware_test();
      }

      #if ENABLED(DGUS_MKS_RUNOUT_SENSOR)
      if(recovery_flg == true)
      {
        //if((thermalManager.temp_hotend[0].target!=0)&&(thermalManager.temp_hotend[0].target <= thermalManager.temp_hotend[0].celsius))
        if((thermalManager.temp_hotend[0].target!=0)&&(millis()-recovery_ms)>=1000*360)
        {
          recovery_flg = false;
        }
      }

      if(booted && printingIsActive() && (recovery_flg!=true) && (current_position.z > 0.8) && (mks_filament_det_enable == 1) ) {

        if(runout_mks.runout_status == RUNOUT_UNCKECK) { runout_mks.runout_status = UNRUNOUT_STATUS; }

        DGUS_Runout_Idle();

      }else {
        DGUS_Runout_reset();
      }

          

      if(runout_mks.beeper_flg == 1)
      {
        if(millis()-runout_mks.beeper_tick1>=2000)
        {
          runout_mks.beeper_tick1 = millis();
          dgusdisplay.SetBeeper_time(0x64);
          runout_mks.beeper_cnt++;
          if(runout_mks.beeper_cnt>=10){
            runout_mks.beeper_flg = 0;
          }
        }
      }	  
      #endif
      if(IS_SD_PAUSED()&&(loadfilament_confirm_flg==true)&&(filament_change_flg == true))
      {
        loadfilament_confirm_flg = false;
        filament_change_flg = false;

        const float olde = current_position.e;
        current_position.e += PRINT_FILAMENT_LENGTH;
        line_to_current_position(MMM_TO_MMS(PRINT_FILAMENT_SPEED));
        current_position.e = olde;
        planner.set_e_position_mm(olde);
        planner.synchronize();
      }

      if(IS_SD_PAUSED()&&(unloadfilament_confirm_flg==true)&&(filament_change_flg == true))
      {
        unloadfilament_confirm_flg = false;

        const float olde = current_position.e;
        current_position.e -= PRINT_FILAMENT_LENGTH;
        line_to_current_position(MMM_TO_MMS(PRINT_FILAMENT_SPEED));
        current_position.e = olde;
        planner.set_e_position_mm(olde);
        planner.synchronize();
      }

      if(filament_data.heating == true){
        if((thermalManager.temp_hotend[0].celsius) >= thermalManager.extrude_min_temp){
          filament_data.heating = false;
          if(filament_data.action==1){
            GotoScreen(MKSLCD_Screen_EXTRUDE_LOAD_POPUP);
          }
          else{
            GotoScreen(MKSLCD_Screen_EXTRUDE_UNLOAD_POPUP);
          }
        }

    }
  #endif // SHOW_BOOTSCREEN

  return IsScreenComplete();
}

void DGUSScreenHandler::LanguagePInit() {
  switch (mks_language_index) {
    case MKS_SimpleChinese:
      dgusdisplay.MKS_WriteVariable(VP_LANGUAGE_CHANGE1, MKS_Language_Choose);
      dgusdisplay.MKS_WriteVariable(VP_LANGUAGE_CHANGE2, MKS_Language_NoChoose);
      break;
    case MKS_English:
      dgusdisplay.MKS_WriteVariable(VP_LANGUAGE_CHANGE1, MKS_Language_NoChoose);
      dgusdisplay.MKS_WriteVariable(VP_LANGUAGE_CHANGE2, MKS_Language_Choose);
      break;
    default:
      break;
  }
}

void DGUSScreenHandler::DGUS_ExtrudeLoadInit(void) {
  ex_filament.ex_length           = distanceFilament;
  ex_filament.ex_load_unload_flag = 0;
  ex_filament.ex_need_time        = filamentSpeed_mm_s;
  ex_filament.ex_speed            = 0;
  ex_filament.ex_status           = EX_NONE;
  ex_filament.ex_tick_end         = 0;
  ex_filament.ex_tick_start       = 0;
}

void DGUSScreenHandler::DGUS_RunoutInit(void) {
  #if PIN_EXISTS(MT_DET_1)
    SET_INPUT_PULLUP(MT_DET_1_PIN);
  #endif
  runout_mks.de_count      = 0;
  runout_mks.de_times      = 10;
  runout_mks.pin_status    = 1;
  runout_mks.runout_status = UNRUNOUT_STATUS;
}

void DGUSScreenHandler::DGUS_Runout_reset(void) {
  runout_mks.runout_status = RUNOUT_UNCKECK;
}

void DGUSScreenHandler::DGUS_Runout_init(void) {
  runout_mks.runout_status = UNRUNOUT_STATUS;
}

void DGUSScreenHandler::DGUS_Runout_Idle(void) {
  #if ENABLED(DGUS_MKS_RUNOUT_SENSOR)
    // scanf runout pin
    switch (runout_mks.runout_status) {

      case RUNOUT_UNCKECK:

      break;

      case RUNOUT_STATUS:
        runout_mks.runout_status = RUNOUT_WAITTING_STATUS;//RUNOUT_BEGIN_STATUS;
        queue.inject(F("M25"));
        //GotoScreen(MKSLCD_SCREEN_PAUSE);

        // sendinfoscreen(F("NOTICE"), nullptr, F("Please change filament!"), nullptr, true, true, true, true);
        //SetupConfirmAction(nullptr);
        // GotoScreen(DGUSLCD_SCREEN_POPUP);
        GotoScreen(MKSLCD_Screen_RUNOUT_POPUP);
		dgusdisplay.SetBeeper_time(0x64);
		runout_mks.beeper_tick1 = millis();
		runout_mks.beeper_flg = 1;
        break;

      case UNRUNOUT_STATUS:
        if (READ(MT_DET_1_PIN) == MT_DET_PIN_STATE)//���͵�ƽ
        {
          runout_mks.runout_status = RUNOUT_CHECK_LOW;
		  runout_mks.runout_tick_1 = millis();
        }
		else//���ߵ�ƽ
		{
		  runout_mks.runout_status = RUNOUT_CHECK_HIGH;
		  runout_mks.runout_tick_1 = millis();
		}
        break;
	  case RUNOUT_CHECK_LOW:
	  	if(millis()-runout_mks.runout_tick_1 <= 5000)
	  	{
		  	if (READ(MT_DET_1_PIN) == HIGH)//5s���е�ƽ���䣬�����¿�ʼ���
		  	{
					runout_mks.runout_status = UNRUNOUT_STATUS;
		  	}	
	  	}
		else
		{
			if (READ(MT_DET_1_PIN) == LOW)//5s�ڵ�ƽһֱ���䣬����н�һ�����
			{
				runout_mks.runout_status = RUNOUT_CHECK_20S_LOW;
				runout_mks.runout_tick_2 = millis();
				runout_mks.E_pos_breakout_before = planner.get_axis_position_mm(E_AXIS);
			}
			else//�������ʱ�̳�������
			{
				runout_mks.runout_status = UNRUNOUT_STATUS;
			}
		}
	  	break;
	  case RUNOUT_CHECK_HIGH:
	  	if(millis()-runout_mks.runout_tick_1 <= 5000)
	  	{
		  	if(READ(MT_DET_1_PIN) == LOW)
		  	{
					runout_mks.runout_status = UNRUNOUT_STATUS;
		  	}	
	  	}
		else
		{
			if(READ(MT_DET_1_PIN) == HIGH)
			{
				runout_mks.runout_status = RUNOUT_CHECK_20S_HIGH;
				runout_mks.runout_tick_2 = millis();
				runout_mks.E_pos_breakout_before = planner.get_axis_position_mm(E_AXIS);
			}
			else
			{
				runout_mks.runout_status = UNRUNOUT_STATUS;
			}
		}	  	
	  	break;
	  case RUNOUT_CHECK_20S_LOW:
	  	if(millis()-runout_mks.runout_tick_2<=20*1000)
	  	{
			if(READ(MT_DET_1_PIN) == HIGH)//��һ�����ʱ������е�ƽ�仯����������¼��
			{
				runout_mks.runout_status = UNRUNOUT_STATUS;
			}

	  	}
		else
		{
	  		if(READ(MT_DET_1_PIN) == LOW)//20s��ƽһֱ���䣬��E��һֱ��ת
	  		{
	  			runout_mks.E_pos_breakout_after = planner.get_axis_position_mm(E_AXIS);
				  runout_mks.E_pos_differ =  runout_mks.E_pos_breakout_after - runout_mks.E_pos_breakout_before;
				  if(runout_mks.E_pos_differ>=10)//�����������룬���ж�Ϊ���ϻ���ϡ�
					  runout_mks.runout_status = RUNOUT_STATUS;
          else{
            runout_mks.runout_status = UNRUNOUT_STATUS;
          }
	  		}
			else
			{
				runout_mks.runout_status = UNRUNOUT_STATUS;
			}

		}
	  	break;
	  case RUNOUT_CHECK_20S_HIGH:
	  	if(millis()-runout_mks.runout_tick_2<=20*1000)
	  	{
        if(READ(MT_DET_1_PIN) == LOW)//��һ�����ʱ������е�ƽ�仯����������¼��
        {
          runout_mks.runout_status = UNRUNOUT_STATUS;
        }

	  	}
		  else
		  {
	  		if(READ(MT_DET_1_PIN) == HIGH)//20s��ƽһֱ���䣬��E��һֱ��ת
	  		{
	  			runout_mks.E_pos_breakout_after = planner.get_axis_position_mm(E_AXIS);
          runout_mks.E_pos_differ =  runout_mks.E_pos_breakout_after - runout_mks.E_pos_breakout_before;
          if(runout_mks.E_pos_differ>=10)//�����������룬���ж�Ϊ���ϻ���ϡ�
            runout_mks.runout_status = RUNOUT_STATUS;
          else
          {
            runout_mks.runout_status = UNRUNOUT_STATUS;
          }
	  		}
        else
        {
          runout_mks.runout_status = UNRUNOUT_STATUS;
        }

		}
	  	break;		
      case RUNOUT_BEGIN_STATUS:
        //if (READ(MT_DET_1_PIN) != MT_DET_PIN_STATE)
        //  runout_mks.runout_status = RUNOUT_WAITTING_STATUS;
        break;

      case RUNOUT_WAITTING_STATUS:
        //if (READ(MT_DET_1_PIN) == MT_DET_PIN_STATE)
        //  runout_mks.runout_status = RUNOUT_BEGIN_STATUS;

        break;

      default: break;
    }
  #endif
}

void disp_language_char_set(uint16_t adr, const char *values, uint8_t num, uint8_t all_clean) {
  dgusdisplay.write_str_to_disp(adr, values, num, all_clean);
}

void disp_language_uint16_set(uint16_t adr, const uint16_t *values, uint8_t num, uint8_t all_clean) {
  dgusdisplay.write_str_to_disp(adr, values, num, true);
}

void DGUSScreenHandler::DGUS_LanguageDisplay(uint8_t var) {
  if (var == MKS_English) {
    const char COM_BACK_buf_en[] = "Back";
    disp_language_char_set(VP_COM_BACK_Dis, COM_BACK_buf_en, 5, true);

    const char Print_buf_en[] = "Print";
    disp_language_char_set(VP_PRINT_Dis, Print_buf_en, 6, false);

    const char Prepare_buf_en[] = "Prepare";
    disp_language_char_set(VP_PREPARE_Dis, Prepare_buf_en, 8, false);

    const char Settings_buf_en[] = "Settings";
    disp_language_char_set(VP_SETTING_Dis, Settings_buf_en, 9, false);

    const char Level_buf_en[] = "Level";
    disp_language_char_set(VP_LVEVL_Dis, Level_buf_en, 6, false);

    const char Preheat_buf_en[] = "Preheat";
    disp_language_char_set(VP_preheat_Dis, Preheat_buf_en, 8, true);

    const char Move_buf_en[] = "Move";
    disp_language_char_set(VP_move_Dis, Move_buf_en, 5, true);

    const char extrude_buf_en[] = "Extrude";

    disp_language_char_set(VP_extrude_Dis, extrude_buf_en, 8, true);

    const char Filchange_buf_en[] = "Filchange";
    disp_language_char_set(VP_Filchange_Dis, Filchange_buf_en, 10, true);

    const char fan_buf_en[] = "Fan";
    disp_language_char_set(VP_fan_Dis, fan_buf_en, 4, true);
  }
  else if (var == MKS_SimpleChinese) {

    const char COM_BACK_buf_en[] = "Back";
    disp_language_char_set(VP_COM_BACK_Dis, COM_BACK_buf_en, 5, true);

    const char Print_buf_en[] = "Printing";
    dgusdisplay.write_str_to_disp(VP_PRINT_Dis, Print_buf_en, 9, false);

    const char Prepare_buf_en[] = "Prepare";
    disp_language_char_set(VP_PREPARE_Dis, Prepare_buf_en, 8, false);

    const char Settings_buf_en[] = "Settings";
    disp_language_char_set(VP_SETTING_Dis, Settings_buf_en, 9, false);

    const char Level_buf_en[] = "Level";
    disp_language_char_set(VP_LVEVL_Dis, Level_buf_en, 6, false);

    const char Preheat_buf_en[] = "Preheat";
    disp_language_char_set(VP_preheat_Dis, Preheat_buf_en, 8, true);

    const char Move_buf_en[] = "Move";
    disp_language_char_set(VP_move_Dis, Move_buf_en, 5, true);

    const char extrude_buf_en[] = "Extrude";
    disp_language_char_set(VP_extrude_Dis, extrude_buf_en, 8, true);

    const char Filchange_buf_en[] = "Filchange";
    disp_language_char_set(VP_Filchange_Dis, Filchange_buf_en, 10, true);

    const char fan_buf_en[] = "Fan";
    disp_language_char_set(VP_fan_Dis, fan_buf_en, 4, true);
  }
}

#endif // DGUS_LCD_UI_MKS
