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

#include "../../inc/MarlinConfig.h"

#if ENABLED(GRID_SKEW_COMPENSATION)

#include "../gcode.h"
#include "../../module/planner.h"

/**
 * M853: Get or set the machine skew factors. Reports current values with no arguments.
 *
 *  L[zx_factor] - New ZX skew factor
 *  M[zy_factor] - New ZY skew factor
 */
void GcodeSuite::M853() {
  if (!parser.seen("LM")) return M853_report();

  uint8_t badval = 0, setval = 0;

  if (parser.seenval('L')) {
    const float value = parser.value_linear_units();
    if (WITHIN(value, SKEW_FACTOR_MIN, SKEW_FACTOR_MAX)) {
      if (planner.skew_factor.zx != value) {
        planner.skew_factor.zx = value;
        ++setval;
      }
    }
    else
      ++badval;
  }

  if (parser.seenval('M')) {
    const float value = parser.value_linear_units();
    if (WITHIN(value, SKEW_FACTOR_MIN, SKEW_FACTOR_MAX)) {
      if (planner.skew_factor.zy != value) {
        planner.skew_factor.zy = value;
        ++setval;
      }
    }
    else
      ++badval;
  }


  if (badval)
    SERIAL_ECHOLNPGM(STR_SKEW_MIN " " STRINGIFY(SKEW_FACTOR_MIN) " " STR_SKEW_MAX " " STRINGIFY(SKEW_FACTOR_MAX));

  // When skew is changed the current position changes
  if (setval) {
    set_current_from_steppers_for_axis(ALL_AXES_ENUM);
    sync_plan_position();
    report_current_position();
  }
}

void GcodeSuite::M853_report(const bool forReplay/*=true*/) {
  report_heading_etc(forReplay, F(STR_SKEW_FACTOR));
  SERIAL_ECHOPAIR_F("  M853 L", planner.skew_factor.zx, 6);
    SERIAL_ECHOPAIR_F(" M", planner.skew_factor.zy, 6);
    SERIAL_ECHOLNPGM(" ; ZX ,ZY");
}

#endif // GRID_SKEW_COMPENSATION
