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

#include "main.h"
#include "browser.h"
#include "init.h"
#include "file.h"
#include "package_installer.h"
#include "utils.h"
#include "rif.h"

char vitashell_titleid[12];

int is_safe_mode = 0;

// System params
int language = 0, enter_button = 0, date_format = 0, time_format = 0;

int isSafeMode() {
  return is_safe_mode;
}

static void initSceAppUtil() {
  // Init SceAppUtil
  SceAppUtilInitParam init_param;
  SceAppUtilBootParam boot_param;
  memset(&init_param, 0, sizeof(SceAppUtilInitParam));
  memset(&boot_param, 0, sizeof(SceAppUtilBootParam));
  sceAppUtilInit(&init_param, &boot_param);

  // Mount
  sceAppUtilMusicMount();
  sceAppUtilPhotoMount();

  // System params
  sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &language);
  sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &enter_button);
  sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_DATE_FORMAT, &date_format);
  sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_TIME_FORMAT, &time_format);

  // Set common dialog config
  SceCommonDialogConfigParam config;
  sceCommonDialogConfigParamInit(&config);
  sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, (int *)&config.language);
  sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, (int *)&config.enterButtonAssign);
  sceCommonDialogSetConfigParam(&config);
}

static void finishSceAppUtil() {
  // Unmount
  sceAppUtilPhotoUmount();
  sceAppUtilMusicUmount();

  // Shutdown AppUtil
  sceAppUtilShutdown();
}

static int isKoreanChar(unsigned int c) {
  unsigned short ch = c;

  // Hangul compatibility jamo block
  if (0x3130 <= ch && ch <= 0x318F) {
    return 1;
  }

  // Hangul syllables block
  if (0xAC00 <= ch && ch <= 0xD7AF) {
    return 1;
  }

  // Korean won sign
  if (ch == 0xFFE6) {
    return 1;
  }

  return 0;
}

static int isLatinChar(unsigned int c) {
  unsigned short ch = c;

  // Basic latin block + latin-1 supplement block
  if (ch <= 0x00FF) {
    return 1;
  }

  // Cyrillic block
  if (0x0400 <= ch && ch <= 0x04FF) {
    return 1;
  }

  return 0;
}

vita2d_pgf *loadSystemFonts() {
  vita2d_system_pgf_config configs[] = {
    { SCE_FONT_LANGUAGE_KOREAN, isKoreanChar },
    { SCE_FONT_LANGUAGE_LATIN, isLatinChar },
    { SCE_FONT_LANGUAGE_DEFAULT, NULL },
  };

  return vita2d_load_system_pgf(3, configs);
}

static void initVita2dLib() {
  vita2d_init();
}

static void finishVita2dLib() {
  vita2d_free_pgf(font);
  vita2d_fini();

  font = NULL;
}

static int initSQLite() {
  return sqlite_init();
}

static int finishSQLite() {
  return sqlite_exit();
}

#define NET_MEMORY_SIZE (4 * 1024 * 1024)

static char *net_memory = NULL;

static void initNet() {
  net_memory = malloc(NET_MEMORY_SIZE);

  SceNetInitParam param;
  param.memory = net_memory;
  param.size = NET_MEMORY_SIZE;
  param.flags = 0;

  sceNetInit(&param);
  sceNetCtlInit();

  sceSslInit(300 * 1024);
  sceHttpInit(40 * 1024);

  sceHttpsDisableOption(SCE_HTTPS_FLAG_SERVER_VERIFY);
}

static void finishNet() {
  sceSslTerm();
  sceHttpTerm();
  sceNetCtlTerm();
  sceNetTerm();
  free(net_memory);
}

void initVitaShell() {
  // Set CPU to 444mhz
  scePowerSetArmClockFrequency(444);

  // Init SceShellUtil events
  sceShellUtilInitEvents(0);

  // Get titleid
  memset(vitashell_titleid, 0, sizeof(vitashell_titleid));
  sceAppMgrAppParamGetString(sceKernelGetProcessId(), 12, vitashell_titleid, sizeof(vitashell_titleid));

  // Allow writing to ux0:app/VITASHELL
  sceAppMgrUmount("app0:");
  sceAppMgrUmount("savedata0:");

  // Is safe mode
  if (sceIoDevctl("ux0:", 0x3001, NULL, 0, NULL, 0) == 0x80010030)
    is_safe_mode = 1;

  // Set sampling mode
  sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

  // Load modules
  sceSysmoduleLoadModule(SCE_SYSMODULE_VIDEO_EXPORT);
  sceSysmoduleLoadModule(SCE_SYSMODULE_PGF);
  sceSysmoduleLoadModule(SCE_SYSMODULE_MUSIC_EXPORT);
  sceSysmoduleLoadModule(SCE_SYSMODULE_PHOTO_EXPORT);
  sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
  sceSysmoduleLoadModule(SCE_SYSMODULE_HTTPS);
  sceSysmoduleLoadModule(SCE_SYSMODULE_SQLITE);

  // Init
  initVita2dLib();
  initSceAppUtil();
  initNet();
  initSQLite();

  // Init power tick thread
  initPowerTickThread();
}

void finishVitaShell() {
  // Finish
  finishSQLite();
  finishNet();
  finishSceAppUtil();
  finishVita2dLib();
  
  // Unload modules
  sceSysmoduleUnloadModule(SCE_SYSMODULE_SQLITE);
  sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTPS);
  sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
  sceSysmoduleUnloadModule(SCE_SYSMODULE_PHOTO_EXPORT);
  sceSysmoduleUnloadModule(SCE_SYSMODULE_MUSIC_EXPORT);
  sceSysmoduleUnloadModule(SCE_SYSMODULE_PGF);
  sceSysmoduleUnloadModule(SCE_SYSMODULE_VIDEO_EXPORT);
}
