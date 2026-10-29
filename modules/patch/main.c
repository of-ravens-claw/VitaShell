/*
  VitaShell
  Copyright (C) 2015-2018, TheFloW

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <psp2kern/kernel/modulemgr.h>
#include <taihen.h>

static SceUID hooks[2];

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
  // Get tai module info
  tai_module_info_t info;
  info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceAppMgr", &info) < 0)
    return SCE_KERNEL_START_SUCCESS;

  // Patch to allow Memory Card remount
  uint32_t nop_nop_opcode = 0xBF00BF00;
  switch (info.module_nid) {
    case 0x97A7FDCC: // 3.18
      hooks[0] = taiInjectDataForKernel(KERNEL_PID, info.modid, 0, 0xE4AE, &nop_nop_opcode, 4);
      hooks[1] = taiInjectDataForKernel(KERNEL_PID, info.modid, 0, 0xE4DE, &nop_nop_opcode, 2);
      break;

    case 0xF1102066: // 3.30
    case 0xA1BE0ED6: // 3.35
    case 0x6ABD8ABA: // 3.36
      hooks[0] = taiInjectDataForKernel(KERNEL_PID, info.modid, 0, 0xE4E6, &nop_nop_opcode, 4);
      hooks[1] = taiInjectDataForKernel(KERNEL_PID, info.modid, 0, 0xE516, &nop_nop_opcode, 2);
      break;

    case 0xE2EDFCC3: // 3.50
    case 0x8C76997D: // 3.51
    case 0x1D32C3FF: // 3.52
    case 0x94CEFE4B: // 3.55
    case 0xDFBC288C: // 3.57
    case 0xDBB29DB7: // 3.60
    case 0xB5F8EA7C: // 3.61
    case 0x23B967C5: // 3.63
    case 0x1C9879D6: // 3.65
      hooks[0] = taiInjectDataForKernel(KERNEL_PID, info.modid, 0, 0xB338, &nop_nop_opcode, 4);
      hooks[1] = taiInjectDataForKernel(KERNEL_PID, info.modid, 0, 0xB368, &nop_nop_opcode, 2);
      break;
      
    case 0x54E2E984: // 3.67
    case 0xC3C538DE: // 3.68
      hooks[0] = taiInjectDataForKernel(KERNEL_PID, info.modid, 0, 0xB344, &nop_nop_opcode, 4);
      hooks[1] = taiInjectDataForKernel(KERNEL_PID, info.modid, 0, 0xB374, &nop_nop_opcode, 2);
      break;
      
    case 0x321E4852: // 3.69
    case 0x700DA0CD: // 3.70
    case 0xF7846B4E: // 3.71
    case 0xA8E80BA8: // 3.72
    case 0xB299D195: // 3.73
    case 0x30007BD3: // 3.74
      hooks[0] = taiInjectDataForKernel(KERNEL_PID, info.modid, 0, 0xB34C, &nop_nop_opcode, 4);
      hooks[1] = taiInjectDataForKernel(KERNEL_PID, info.modid, 0, 0xB37C, &nop_nop_opcode, 2);
      break;
  }

  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
  if (hooks[1] >= 0)
    taiInjectReleaseForKernel(hooks[1]);

  if (hooks[0] >= 0)
    taiInjectReleaseForKernel(hooks[0]);

  return SCE_KERNEL_STOP_SUCCESS;
}
