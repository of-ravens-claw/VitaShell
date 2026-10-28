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
#include "main_context.h"
#include "browser.h"
#include "init.h"
#include "io_process.h"
#include "makezip.h"
#include "package_installer.h"
#include "context_menu.h"
#include "archive.h"
#include "photo.h"
#include "file.h"
#include "text.h"
#include "hex.h"
#include "settings.h"
#include "property_dialog.h"
#include "message_dialog.h"
#include "ime_dialog.h"
#include "theme.h"
#include "language.h"
#include "utils.h"
#include "sfo.h"
#include "coredump.h"
#include "pfs.h"

int _newlib_heap_size_user = 128 * 1024 * 1024;

static volatile int dialog_step = DIALOG_STEP_NONE;

static char install_path[MAX_PATH_LENGTH];
static char compress_name[MAX_NAME_LENGTH];

static SceKernelLwMutexWork dialog_mutex;

char vita_ip[16];
unsigned short int vita_port;

int SCE_CTRL_ENTER = SCE_CTRL_CROSS, SCE_CTRL_CANCEL = SCE_CTRL_CIRCLE;

int getDialogStep() {
  sceKernelLockLwMutex(&dialog_mutex, 1, NULL);
  volatile int step = dialog_step;
  sceKernelUnlockLwMutex(&dialog_mutex, 1);
  return step;
}

void setDialogStep(int step) {
  sceKernelLockLwMutex(&dialog_mutex, 1, NULL);
  dialog_step = step;
  sceKernelUnlockLwMutex(&dialog_mutex, 1);
}

void drawScrollBar(int pos, int n) {
  if (n > MAX_POSITION) {
    vita2d_draw_rectangle(SCROLL_BAR_X, START_Y, SCROLL_BAR_WIDTH, MAX_ENTRIES * FONT_Y_SPACE, SCROLL_BAR_BG_COLOR);

    float y = START_Y + ((pos * FONT_Y_SPACE) / (n * FONT_Y_SPACE)) * (MAX_ENTRIES * FONT_Y_SPACE);
    float height = ((MAX_POSITION * FONT_Y_SPACE) / (n * FONT_Y_SPACE)) * (MAX_ENTRIES * FONT_Y_SPACE);

    float scroll_bar_y = MIN(y, (START_Y + MAX_ENTRIES * FONT_Y_SPACE - height));
    vita2d_draw_rectangle(SCROLL_BAR_X, scroll_bar_y, SCROLL_BAR_WIDTH, MAX(height, SCROLL_BAR_MIN_HEIGHT), SCROLL_BAR_COLOR);
  }
}

void drawShellInfo(const char *path) {
  // Title
  char version[8];
  sprintf(version, "%X.%02X", VITASHELL_VERSION_MAJOR, VITASHELL_VERSION_MINOR);
  if (version[3] == '0')
    version[3] = '\0';

  pgf_draw_textf(SHELL_MARGIN_X, SHELL_MARGIN_Y, TITLE_COLOR, "VitaShell %s (stripped)", version);

  // Status bar
  float x = SCREEN_WIDTH - SHELL_MARGIN_X;

  // Battery
  if (sceKernelGetModel() == SCE_KERNEL_MODEL_VITA) {
    float battery_x = ALIGN_RIGHT(x, vita2d_texture_get_width(battery_image));
    vita2d_draw_texture(battery_image, battery_x, SHELL_MARGIN_Y + 3.0f);

    vita2d_texture *battery_bar_image = battery_bar_green_image;

    if (scePowerIsLowBattery() && !scePowerIsBatteryCharging()) {
      battery_bar_image = battery_bar_red_image;
    } 

    float percent = scePowerGetBatteryLifePercent() / 100.0f;

    float width = vita2d_texture_get_width(battery_bar_image);
    vita2d_draw_texture_part(battery_bar_image, battery_x + 3.0f + (1.0f - percent) * width,
                             SHELL_MARGIN_Y + 5.0f, (1.0f - percent) * width, 0.0f, percent * width,
                             vita2d_texture_get_height(battery_bar_image));

    if (scePowerIsBatteryCharging()) {
      vita2d_draw_texture(battery_bar_charge_image, battery_x + 3.0f, SHELL_MARGIN_Y + 5.0f);
    }

    x = battery_x - STATUS_BAR_SPACE_X;
  }

  // Date & time
  SceDateTime time;
  sceRtcGetCurrentClock(&time, 0);

  char date_string[24];
  getDateString(date_string, date_format, &time);

  char time_string[16];
  getTimeString(time_string, time_format, &time);

  char string[64];
  snprintf(string, sizeof(string), "%s  %s", date_string, time_string);
  float date_time_x = ALIGN_RIGHT(x, pgf_text_width(string));
  pgf_draw_text(date_time_x, SHELL_MARGIN_Y, DATE_TIME_COLOR, string);

  x = date_time_x - STATUS_BAR_SPACE_X;

  // FTP
  if (ftpvita_is_initialized()) {
    float ftp_x = ALIGN_RIGHT(x, vita2d_texture_get_width(ftp_image));
    vita2d_draw_texture(ftp_image, ftp_x, SHELL_MARGIN_Y + 3.0f);
    
    x = ftp_x - STATUS_BAR_SPACE_X;
  }

  // TODO: make this more elegant
  // Path
  int line_width = 0;

  int i;
  for (i = 0; i < strlen(path); i++) {
    char ch_width = font_size_cache[(int)path[i]];

    // Too long
    if ((line_width + ch_width) >= MAX_WIDTH)
      break;

    // Increase line width
    line_width += ch_width;
  }

  char path_first_line[256], path_second_line[256];

  strncpy(path_first_line, path, i);
  path_first_line[i] = '\0';

  strcpy(path_second_line, path+i);

  // home (SAFE/UNSAFE MODE)
  if (strcmp(path_first_line, HOME_PATH) == 0) {    
    pgf_draw_textf(SHELL_MARGIN_X, PATH_Y, PATH_COLOR, "%s (%s)", HOME_PATH,
                   isSafeMode() ? language_container[SAFE_MODE] : language_container[UNSAFE_MODE]);
  } else {
    // Path
    pgf_draw_text(SHELL_MARGIN_X, PATH_Y, PATH_COLOR, path_first_line);
    pgf_draw_text(SHELL_MARGIN_X, PATH_Y + FONT_Y_SPACE, PATH_COLOR, path_second_line);
  }
}

void initFtp() {
  // Add all the current mountpoints to ftpvita
  int i;
  for (i = 0; i < getNumberOfDevices(); i++) {
    char **devices = getDevices();
    if (devices[i]) {
      if (isSafeMode() && strcmp(devices[i], "ux0:") != 0)
        continue;

      ftpvita_add_device(devices[i]);
    }
  }

  ftpvita_ext_add_custom_command("PROM", ftpvita_PROM);
}

int dialogSteps() {
  int refresh = REFRESH_MODE_NONE;

  int msg_result = updateMessageDialog();
  int ime_result = updateImeDialog();

  switch (getDialogStep()) {
    case DIALOG_STEP_ERROR:
    case DIALOG_STEP_INFO:
    case DIALOG_STEP_SYSTEM:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_NONE ||
          msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        refresh = REFRESH_MODE_NORMAL;
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_CANCELED:
      refresh = REFRESH_MODE_NORMAL;
      setDialogStep(DIALOG_STEP_NONE);
      break;
      
    case DIALOG_STEP_DELETED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_NONE ||
          msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
        if (file_entry) {
          // Empty mark list if on marked entry
          if (fileListFindEntry(&mark_list, file_entry->name)) {
            fileListEmpty(&mark_list);
          }

          refresh = REFRESH_MODE_NORMAL;
        }
        
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_COMPRESSED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_NONE ||
          msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
        if (file_entry) {
          // Empty mark list if on marked entry
          if (fileListFindEntry(&mark_list, file_entry->name)) {
            fileListEmpty(&mark_list);
          }
          
          // Focus
          setFocusName(compress_name);
          
          refresh = REFRESH_MODE_SETFOCUS;
        }
        
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_COPIED:
    case DIALOG_STEP_MOVED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_NONE ||
          msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        // Empty mark list
        fileListEmpty(&mark_list);
        
        // Copy copy list to mark list
        FileListEntry *copy_entry = copy_list.head;
        
        int i;
        for (i = 0; i < copy_list.length; i++) {
          fileListAddEntry(&mark_list, fileListCopyEntry(copy_entry), SORT_NONE);

          // Next
          copy_entry = copy_entry->next;
        }
        
        // Focus
        setFocusName(copy_list.head->name);

        // Empty copy list when moved
        if (getDialogStep() == DIALOG_STEP_MOVED)
          fileListEmpty(&copy_list);
        
        // Umount and remove from clipboard after pasting
        if (pfs_mounted_path[0] &&
            !strstr(file_list.path, pfs_mounted_path) &&
             strstr(copy_list.path, pfs_mounted_path)) {
          pfsUmount();
          fileListEmpty(&copy_list);
        }

        refresh = REFRESH_MODE_SETFOCUS;
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_FTP_WAIT:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
        int state = 0;
        sceNetCtlInetGetState(&state);
        if (state == 3) {
          int res = ftpvita_init(vita_ip, &vita_port);
          if (res >= 0) {
            initFtp();
            sceMsgDialogClose();
          }
        }
      } else {
        if (msg_result == MESSAGE_DIALOG_RESULT_NONE ||
            msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
          setDialogStep(DIALOG_STEP_NONE);

          // Dialog
          if (ftpvita_is_initialized()) {
            initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL, language_container[FTP_SERVER], vita_ip, vita_port);
            setDialogStep(DIALOG_STEP_FTP);
          } else {
            powerUnlock();
          }
        }
      }
      
      break;
    }
    
    case DIALOG_STEP_FTP:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
        refresh = REFRESH_MODE_NORMAL;
        setDialogStep(DIALOG_STEP_NONE);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
        powerUnlock();
        ftpvita_fini();
        refresh = REFRESH_MODE_NORMAL;
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_PASTE:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
        CopyArguments args;
        args.file_list = &file_list;
        args.copy_list = &copy_list;
        args.archive_path = archive_copy_path;
        args.copy_mode = copy_mode;

        setDialogStep(DIALOG_STEP_COPYING);

        SceUID thid = sceKernelCreateThread("copy_thread", (SceKernelThreadEntry)copy_thread, 0x40, 0x100000, 0, 0, NULL);
        if (thid >= 0)
          sceKernelStartThread(thid, sizeof(CopyArguments), &args);
      }

      break;
    }
    
    case DIALOG_STEP_DELETE_QUESTION:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
        initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[DELETING]);
        setDialogStep(DIALOG_STEP_DELETE_CONFIRMED);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_DELETE_CONFIRMED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
        DeleteArguments args;
        args.file_list = &file_list;
        args.mark_list = &mark_list;
        args.index = base_pos + rel_pos;

        setDialogStep(DIALOG_STEP_DELETING);

        SceUID thid = sceKernelCreateThread("delete_thread", (SceKernelThreadEntry)delete_thread, 0x40, 0x100000, 0, 0, NULL);
        if (thid >= 0)
          sceKernelStartThread(thid, sizeof(DeleteArguments), &args);
      }

      break;
    }
    
    case DIALOG_STEP_RENAME:
    {
      if (ime_result == IME_DIALOG_RESULT_FINISHED) {
        char *name = (char *)getImeDialogInputTextUTF8();
        if (name[0] == '\0') {
          setDialogStep(DIALOG_STEP_NONE);
        } else {
          FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
          if (!file_entry) {
            setDialogStep(DIALOG_STEP_NONE);
            break;
          }
          
          char old_name[MAX_NAME_LENGTH];
          strcpy(old_name, file_entry->name);
          removeEndSlash(old_name);

          if (strcasecmp(old_name, name) == 0) { // No change
            setDialogStep(DIALOG_STEP_NONE);
          } else {
            char old_path[MAX_PATH_LENGTH];
            char new_path[MAX_PATH_LENGTH];

            snprintf(old_path, MAX_PATH_LENGTH, "%s%s", file_list.path, old_name);
            snprintf(new_path, MAX_PATH_LENGTH, "%s%s", file_list.path, name);

            int res = sceIoRename(old_path, new_path);
            if (res < 0) {
              errorDialog(res);
            } else {
              refresh = REFRESH_MODE_NORMAL;
              setDialogStep(DIALOG_STEP_NONE);
            }
          }
        }
      } else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }

    case DIALOG_STEP_NEW_FOLDER:
    {
      if (ime_result == IME_DIALOG_RESULT_FINISHED) {
        char *name = (char *)getImeDialogInputTextUTF8();
        if (name[0] == '\0') {
          setDialogStep(DIALOG_STEP_NONE);
        } else {
          char path[MAX_PATH_LENGTH];
          snprintf(path, MAX_PATH_LENGTH, "%s%s", file_list.path, name);

          int res = sceIoMkdir(path, 0777);
          if (res < 0) {
            errorDialog(res);
          } else {
            // Focus
            char focus_name[MAX_NAME_LENGTH];
            strcpy(focus_name, name);
            addEndSlash(focus_name);
            setFocusName(focus_name);
            
            refresh = REFRESH_MODE_SETFOCUS;
            setDialogStep(DIALOG_STEP_NONE);
          }
        }
      } else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }

    case DIALOG_STEP_NEW_FILE:
    {
      if (ime_result == IME_DIALOG_RESULT_FINISHED) {
        char *name = (char *)getImeDialogInputTextUTF8();
        if (name[0] == '\0') {
          setDialogStep(DIALOG_STEP_NONE);
        } else {
          char path[MAX_PATH_LENGTH];
          snprintf(path, MAX_PATH_LENGTH, "%s%s", file_list.path, name);

          SceUID fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT, 0777);
          if (fd < 0) {
            errorDialog(fd);
          } else {
            sceIoClose(fd);

            // Focus
            char focus_name[MAX_NAME_LENGTH];
            strcpy(focus_name, name);
            addEndSlash(focus_name);
            setFocusName(focus_name);

            refresh = REFRESH_MODE_SETFOCUS;
            setDialogStep(DIALOG_STEP_NONE);
            refreshFileList();
          }
        }
      } else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
        setDialogStep(DIALOG_STEP_NONE);
      }
      break;
    }

    case DIALOG_STEP_COMPRESS_NAME:
    {
      if (ime_result == IME_DIALOG_RESULT_FINISHED) {
        char *name = (char *)getImeDialogInputTextUTF8();
        if (name[0] == '\0') {
          setDialogStep(DIALOG_STEP_NONE);
        } else {
          strcpy(compress_name, name);

          initImeDialog(language_container[COMPRESSION_LEVEL], "6", 1, SCE_IME_TYPE_NUMBER, 0, 0);
          setDialogStep(DIALOG_STEP_COMPRESS_LEVEL);
        }
      } else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
        setDialogStep(DIALOG_STEP_NONE);
      }
      
      break;
    }

    case DIALOG_STEP_COMPRESS_LEVEL:
    {
      if (ime_result == IME_DIALOG_RESULT_FINISHED) {
        char *level = (char *)getImeDialogInputTextUTF8();
        if (level[0] == '\0') {
          setDialogStep(DIALOG_STEP_NONE);
        } else {
          snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, compress_name);

          CompressArguments args;
          args.file_list = &file_list;
          args.mark_list = &mark_list;
          args.index = base_pos + rel_pos;
          args.level = atoi(level);
          args.path = cur_file;

          initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[COMPRESSING]);
          setDialogStep(DIALOG_STEP_COMPRESSING);

          SceUID thid = sceKernelCreateThread("compress_thread", (SceKernelThreadEntry)compress_thread, 0x40, 0x100000, 0, 0, NULL);
          if (thid >= 0)
            sceKernelStartThread(thid, sizeof(CompressArguments), &args);
        }
      } else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
        setDialogStep(DIALOG_STEP_NONE);
      }
      
      break;
    }
    
    case DIALOG_STEP_HASH_QUESTION:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
        // Throw up the progress bar, enter hashing state
        initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[HASHING]);
        setDialogStep(DIALOG_STEP_HASH_CONFIRMED);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
        // Quit
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_HASH_CONFIRMED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
        // User has confirmed desire to hash, get requested file entry
        FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
        if (!file_entry) {
          setDialogStep(DIALOG_STEP_NONE);
          break;
        }
        
        // Place the full file path in cur_file
        snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);

        HashArguments args;
        args.file_path = cur_file;

        setDialogStep(DIALOG_STEP_HASHING);

        // Create a thread to run out actual sum
        SceUID thid = sceKernelCreateThread("hash_thread", (SceKernelThreadEntry)hash_thread, 0x40, 0x100000, 0, 0, NULL);
        if (thid >= 0)
          sceKernelStartThread(thid, sizeof(HashArguments), &args);
      }

      break;
    }
    
    case DIALOG_STEP_INSTALL_QUESTION:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
        initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[INSTALLING]);
        setDialogStep(DIALOG_STEP_INSTALL_CONFIRMED);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_INSTALL_CONFIRMED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
        InstallArguments args;

        if (install_list.length > 0) {
          FileListEntry *entry = install_list.head;
          snprintf(install_path, MAX_PATH_LENGTH, "%s%s", install_list.path, entry->name);
          args.file = install_path;

          // Focus
          setFocusOnFilename(entry->name);

          // Remove entry
          fileListRemoveEntry(&install_list, entry);
        } else {
          args.file = cur_file;
        }

        setDialogStep(DIALOG_STEP_INSTALLING);

        SceUID thid = sceKernelCreateThread("install_thread", (SceKernelThreadEntry)install_thread, 0x40, 0x100000, 0, 0, NULL);
        if (thid >= 0)
          sceKernelStartThread(thid, sizeof(InstallArguments), &args);
      }

      break;
    }
 
    case DIALOG_STEP_INSTALL_WARNING:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
        setDialogStep(DIALOG_STEP_INSTALL_WARNING_AGREED);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
        setDialogStep(DIALOG_STEP_CANCELED);
      }

      break;
    }
    
    case DIALOG_STEP_INSTALLED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_NONE ||
          msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        if (install_list.length > 0) {
          initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[INSTALLING]);
          setDialogStep(DIALOG_STEP_INSTALL_CONFIRMED);
          break;
        }

        refresh = REFRESH_MODE_NORMAL;
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_SETTINGS_AGREEMENT:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
        settingsAgree();
        setDialogStep(DIALOG_STEP_NONE);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
        settingsDisagree();
        setDialogStep(DIALOG_STEP_NONE);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        settingsAgree();
        setDialogStep(DIALOG_STEP_NONE);
      }
      
      break;
    }
    
    case DIALOG_STEP_SETTINGS_STRING:
    {
      if (ime_result == IME_DIALOG_RESULT_FINISHED) {
        char *string = (char *)getImeDialogInputTextUTF8();
        if (string[0] != '\0') {
          strcpy((char *)getImeDialogInitialText(), string);
        }

        setDialogStep(DIALOG_STEP_NONE);
      } else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
        setDialogStep(DIALOG_STEP_NONE);
      }
      
      break;
    }
    
    case DIALOG_STEP_ENTER_PASSWORD:
    {
      if (ime_result == IME_DIALOG_RESULT_FINISHED) {
        char *password = (char *)getImeDialogInputTextUTF8();
        if (password[0] == '\0') {
          setDialogStep(DIALOG_STEP_NONE);
        } else {
          // TODO: verify password
          archiveSetPassword(password);
          
          FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
          if (!file_entry) {
            setDialogStep(DIALOG_STEP_NONE);
            break;
          }
          
          setInArchive();
          setDirArchiveLevel();

          snprintf(archive_path, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);

          strcat(file_list.path, file_entry->name);
          addEndSlash(file_list.path);

          dirLevelUp();
          
          refresh = REFRESH_MODE_NORMAL;
          setDialogStep(DIALOG_STEP_NONE);
        }
      } else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
  }

  return refresh;
}

void ftpvita_PROM(ftpvita_client_info_t *client) {
  char cmd[64];
  char path[MAX_PATH_LENGTH];
  sscanf(client->recv_buffer, "%s %s", cmd, path);

  if (installPackage(path) == 0) {
    ftpvita_ext_client_send_ctrl_msg(client, "200 OK PROMOTING\r\n");
  } else {
    ftpvita_ext_client_send_ctrl_msg(client, "500 ERROR PROMOTING\r\n");
  }
}

int main(int argc, const char *argv[]) {  
  // Create mutex
  sceKernelCreateLwMutex(&dialog_mutex, "dialog_mutex", 2, 0, NULL);

  // Init VitaShell
  initVitaShell();
  
  // Load stuff
  loadTheme();
  loadLanguage(language);

  // Init context menu width
  initContextMenuWidth();
  initTextContextMenuWidth();

  // File browser
  browserMain();

  // Finish VitaShell
  finishVitaShell();
  
  return 0;
}
