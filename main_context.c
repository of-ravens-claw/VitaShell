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
#include "io_process.h"
#include "context_menu.h"
#include "file.h"
#include "language.h"
#include "property_dialog.h"
#include "message_dialog.h"
#include "ime_dialog.h"
#include "utils.h"
#include "pfs.h"

enum MenuMainEntrys {
  MENU_MAIN_ENTRY_OPEN_DECRYPTED,
  MENU_MAIN_ENTRY_MARK_UNMARK_ALL,
  MENU_MAIN_ENTRY_MOVE,
  MENU_MAIN_ENTRY_COPY,
  MENU_MAIN_ENTRY_PASTE,
  MENU_MAIN_ENTRY_DELETE,
  MENU_MAIN_ENTRY_RENAME,
  MENU_MAIN_ENTRY_PROPERTIES,
  MENU_MAIN_ENTRY_NEW,
  MENU_MAIN_ENTRY_SORT_BY,
  MENU_MAIN_ENTRY_MORE,
};

MenuEntry menu_main_entries[] = {
  { OPEN_DECRYPTED, 0, 0, CTX_INVISIBLE },
  { MARK_ALL,       1, 0, CTX_INVISIBLE },
  { MOVE,           3, 0, CTX_INVISIBLE },
  { COPY,           4, 0, CTX_INVISIBLE },
  { PASTE,          5, 0, CTX_INVISIBLE },
  { DELETE,         7, 0, CTX_INVISIBLE },
  { RENAME,         8, 0, CTX_INVISIBLE },
  { PROPERTIES,     10, 0, CTX_INVISIBLE },
  { NEW,            12, CTX_FLAG_MORE, CTX_VISIBLE },
  { SORT_BY,        13, CTX_FLAG_MORE, CTX_VISIBLE },
  { MORE,           14, CTX_FLAG_MORE, CTX_INVISIBLE },
};

#define N_MENU_MAIN_ENTRIES (sizeof(menu_main_entries) / sizeof(MenuEntry))

enum MenuSortEntrys {
  MENU_SORT_ENTRY_BY_NAME,
  MENU_SORT_ENTRY_BY_SIZE,
  MENU_SORT_ENTRY_BY_DATE,
};

MenuEntry menu_sort_entries[] = {
  { BY_NAME, 13, 0, CTX_INVISIBLE },
  { BY_SIZE, 14, 0, CTX_INVISIBLE },
  { BY_DATE, 15, 0, CTX_INVISIBLE },
};

#define N_MENU_SORT_ENTRIES (sizeof(menu_sort_entries) / sizeof(MenuEntry))

enum MenuMoreEntrys {
  MENU_MORE_ENTRY_COMPRESS,
  MENU_MORE_ENTRY_INSTALL_ALL,
  MENU_MORE_ENTRY_INSTALL_FOLDER,
  MENU_MORE_ENTRY_CALCULATE_SHA1,
};

MenuEntry menu_more_entries[] = {
  { COMPRESS,       14, 0, CTX_INVISIBLE },
  { INSTALL_ALL,    15, 0, CTX_INVISIBLE },
  { INSTALL_FOLDER, 16, 0, CTX_INVISIBLE },
  { CALCULATE_SHA1, 17, 0, CTX_INVISIBLE },
};

#define N_MENU_MORE_ENTRIES (sizeof(menu_more_entries) / sizeof(MenuEntry))

enum MenuNewEntrys {
  MENU_NEW_FILE,
  MENU_NEW_FOLDER,
};

MenuEntry menu_new_entries[] = {
  { NEW_FILE,      12, 0, CTX_INVISIBLE },
  { NEW_FOLDER,    13, 0, CTX_INVISIBLE },
};

#define N_MENU_NEW_ENTRIES (sizeof(menu_new_entries) / sizeof(MenuEntry))

static int contextMenuMainEnterCallback(int sel, void *context);
static int contextMenuSortEnterCallback(int sel, void *context);
static int contextMenuMoreEnterCallback(int sel, void *context);
static int contextMenuNewEnterCallback(int sel, void *context);

ContextMenu context_menu_main = {
  .parent = NULL,
  .entries = menu_main_entries,
  .n_entries = N_MENU_MAIN_ENTRIES,
  .max_width = 0.0f,
  .callback = contextMenuMainEnterCallback,
  .sel = -1,
};

ContextMenu context_menu_sort = {
  .parent = &context_menu_main,
  .entries = menu_sort_entries,
  .n_entries = N_MENU_SORT_ENTRIES,
  .max_width = 0.0f,
  .callback = contextMenuSortEnterCallback,
  .sel = -1,
};

ContextMenu context_menu_more = {
  .parent = &context_menu_main,
  .entries = menu_more_entries,
  .n_entries = N_MENU_MORE_ENTRIES,
  .max_width = 0.0f,
  .callback = contextMenuMoreEnterCallback,
  .sel = -1,
};

ContextMenu context_menu_new = {
  .parent = &context_menu_main,
  .entries = menu_new_entries,
  .n_entries = N_MENU_NEW_ENTRIES,
  .max_width = 0.0f,
  .callback = contextMenuNewEnterCallback,
  .sel = -1,
};

void initContextMenuWidth() {
  int i;

  // Main
  for (i = 0; i < N_MENU_MAIN_ENTRIES; i++) {
    context_menu_main.max_width = MAX(context_menu_main.max_width,
        pgf_text_width(language_container[menu_main_entries[i].name]));

    if (menu_main_entries[i].name == MARK_ALL) {
      menu_main_entries[i].name = UNMARK_ALL;
      i--;
    }
  }

  context_menu_main.max_width += 2.0f * CONTEXT_MENU_MARGIN;
  context_menu_main.max_width = MAX(context_menu_main.max_width, CONTEXT_MENU_MIN_WIDTH);

  // Sort
  for (i = 0; i < N_MENU_SORT_ENTRIES; i++) {
    context_menu_sort.max_width = MAX(context_menu_sort.max_width, pgf_text_width(language_container[menu_sort_entries[i].name]));
  }

  context_menu_sort.max_width += 2.0f * CONTEXT_MENU_MARGIN;
  context_menu_sort.max_width = MAX(context_menu_sort.max_width, CONTEXT_MENU_MIN_WIDTH);

  // More
  for (i = 0; i < N_MENU_MORE_ENTRIES; i++) {
    context_menu_more.max_width = MAX(context_menu_more.max_width, pgf_text_width(language_container[menu_more_entries[i].name]));
  }

  context_menu_more.max_width += 2.0f * CONTEXT_MENU_MARGIN;
  context_menu_more.max_width = MAX(context_menu_more.max_width, CONTEXT_MENU_MIN_WIDTH);

  // New
  for (i = 0; i < N_MENU_NEW_ENTRIES; i++) {
    context_menu_new.max_width = MAX(context_menu_new.max_width,
        pgf_text_width(language_container[menu_new_entries[i].name]));
  }
  context_menu_new.max_width += 2.0f * CONTEXT_MENU_MARGIN;
  context_menu_new.max_width = MAX(context_menu_new.max_width, CONTEXT_MENU_MIN_WIDTH);
}

void setContextMenuMainVisibilities() {
  int i;

  // All visible
  for (i = 0; i < N_MENU_MAIN_ENTRIES; i++) {
    if (menu_main_entries[i].visibility == CTX_INVISIBLE)
      menu_main_entries[i].visibility = CTX_VISIBLE;
  }

  FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
  if (!file_entry)
    return;

  // menu_main_entries[MENU_MAIN_ENTRY_SEND].flags = CTX_FLAG_BARRIER;
  // menu_main_entries[MENU_MAIN_ENTRY_RECEIVE].flags = 0;

  // Invisble entries when on '..'
  if (strcmp(file_entry->name, DIR_UP) == 0) {
    menu_main_entries[MENU_MAIN_ENTRY_OPEN_DECRYPTED].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_MARK_UNMARK_ALL].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_MOVE].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_COPY].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_DELETE].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_RENAME].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_PROPERTIES].visibility = CTX_INVISIBLE;
  }

  // Invisible 'Paste' if nothing is copied yet
  if (copy_list.length == 0) {
    menu_main_entries[MENU_MAIN_ENTRY_PASTE].visibility = CTX_INVISIBLE;
  }

  // Invisible write operations in archives
  // TODO: read-only mount points
  if (isInArchive() || (pfs_mounted_path[0] && strstr(file_list.path, pfs_mounted_path) && read_only)) {
    menu_main_entries[MENU_MAIN_ENTRY_OPEN_DECRYPTED].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_MOVE].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_PASTE].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_DELETE].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_RENAME].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_NEW].visibility = CTX_INVISIBLE;
  }

  // Mark/Unmark all text
  if (mark_list.length == (file_list.length - 1)) { // All marked
    menu_main_entries[MENU_MAIN_ENTRY_MARK_UNMARK_ALL].name = UNMARK_ALL;
  } else { // Not all marked yet
    // On marked entry
    if (fileListFindEntry(&mark_list, file_entry->name)) {
      menu_main_entries[MENU_MAIN_ENTRY_MARK_UNMARK_ALL].name = UNMARK_ALL;
    } else {
      menu_main_entries[MENU_MAIN_ENTRY_MARK_UNMARK_ALL].name = MARK_ALL;
    }
  }

  // Invisible if it's not folder or sce_pfs does not exist
  if (!file_entry->is_folder) {
    menu_main_entries[MENU_MAIN_ENTRY_OPEN_DECRYPTED].visibility = CTX_INVISIBLE;
  } else {
    char path[MAX_PATH_LENGTH];
    snprintf(path, MAX_PATH_LENGTH, "%s%ssce_pfs", file_list.path, file_entry->name);

    if (!checkFolderExist(path))
      menu_main_entries[MENU_MAIN_ENTRY_OPEN_DECRYPTED].visibility = CTX_INVISIBLE;
  }

  // Go to first entry
  for (i = 0; i < N_MENU_MAIN_ENTRIES; i++) {
    if (menu_main_entries[i].visibility == CTX_VISIBLE) {
      context_menu_main.sel = i;
      break;
    }
  }

  if (i == N_MENU_MAIN_ENTRIES)
    context_menu_main.sel = -1;
}

void setContextMenuSortVisibilities() {
  int i;

  // All visible
  for (i = 0; i < N_MENU_SORT_ENTRIES; i++) {
    if (menu_sort_entries[i].visibility == CTX_INVISIBLE)
      menu_sort_entries[i].visibility = CTX_VISIBLE;
  }

  // Invisible when it's the current mode
  if (sort_mode == SORT_BY_NAME)
    menu_sort_entries[MENU_SORT_ENTRY_BY_NAME].visibility = CTX_INVISIBLE;
  else if (sort_mode == SORT_BY_SIZE)
    menu_sort_entries[MENU_SORT_ENTRY_BY_SIZE].visibility = CTX_INVISIBLE;
  else if (sort_mode == SORT_BY_DATE)
    menu_sort_entries[MENU_SORT_ENTRY_BY_DATE].visibility = CTX_INVISIBLE;

  // Go to first entry
  for (i = 0; i < N_MENU_SORT_ENTRIES; i++) {
    if (menu_sort_entries[i].visibility == CTX_VISIBLE) {
      context_menu_sort.sel = i;
      break;
    }
  }

  if (i == N_MENU_SORT_ENTRIES)
    context_menu_sort.sel = -1;
}

void setContextMenuMoreVisibilities() {
  int i;

  // All visible
  for (i = 0; i < N_MENU_MORE_ENTRIES; i++) {
    if (menu_more_entries[i].visibility == CTX_INVISIBLE)
      menu_more_entries[i].visibility = CTX_VISIBLE;
  }

  FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
  if (!file_entry)
    return;

  // Invisble entries when on '..'
  if (strcmp(file_entry->name, DIR_UP) == 0) {
    menu_more_entries[MENU_MORE_ENTRY_COMPRESS].visibility = CTX_INVISIBLE;
    menu_more_entries[MENU_MORE_ENTRY_INSTALL_ALL].visibility = CTX_INVISIBLE;
    menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_INVISIBLE;
    menu_more_entries[MENU_MORE_ENTRY_CALCULATE_SHA1].visibility = CTX_INVISIBLE;
  }

  // Invisble operations in archives
  if (isInArchive() || (pfs_mounted_path[0] && strstr(file_list.path, pfs_mounted_path) && read_only)) {
    menu_more_entries[MENU_MORE_ENTRY_COMPRESS].visibility = CTX_INVISIBLE;
    menu_more_entries[MENU_MORE_ENTRY_INSTALL_ALL].visibility = CTX_INVISIBLE;
    menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_INVISIBLE;
    menu_more_entries[MENU_MORE_ENTRY_CALCULATE_SHA1].visibility = CTX_INVISIBLE;
  }

  if (file_entry->is_folder) {
    menu_more_entries[MENU_MORE_ENTRY_CALCULATE_SHA1].visibility = CTX_INVISIBLE;

    char check_path[MAX_PATH_LENGTH];

    do {
      if (strcasecmp(file_list.path, "ux0:app/") == 0 ||
          strcasecmp(file_list.path, "ux0:patch/") == 0) {
        menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_INVISIBLE;
        break;
      }

      snprintf(check_path, MAX_PATH_LENGTH, "%s%s/eboot.bin", file_list.path, file_entry->name);
      if (!checkFileExist(check_path)) {
        menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_INVISIBLE;
        break;
      }

      snprintf(check_path, MAX_PATH_LENGTH, "%s%s/sce_sys/param.sfo", file_list.path, file_entry->name);
      if (!checkFileExist(check_path)) {
        menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_INVISIBLE;
        break;
      }
    } while (0);
  } else {
    menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_INVISIBLE;
  }

  if(file_entry->type != FILE_TYPE_VPK) {
    menu_more_entries[MENU_MORE_ENTRY_INSTALL_ALL].visibility = CTX_INVISIBLE;
  }

  // Go to first entry
  for (i = 0; i < N_MENU_MORE_ENTRIES; i++) {
    if (menu_more_entries[i].visibility == CTX_VISIBLE) {
      context_menu_more.sel = i;
      break;
    }
  }

  if (i == N_MENU_MORE_ENTRIES)
    context_menu_more.sel = -1;
}

void setContextMenuNewVisibilities() {
  int i;

  // All visible
  for (i = 0; i < N_MENU_NEW_ENTRIES; i++) {
    if (menu_new_entries[i].visibility == CTX_INVISIBLE)
      menu_new_entries[i].visibility = CTX_VISIBLE;
  }

  // Go to first entry
  for (i = 0; i < N_MENU_NEW_ENTRIES; i++) {
    if (menu_new_entries[i].visibility == CTX_VISIBLE) {
      context_menu_new.sel = i;
      break;
    }
  }

  FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
  if (file_entry)
    snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);

  if (i == N_MENU_NEW_ENTRIES)
    context_menu_new.sel = -1;
}

static int contextMenuMainEnterCallback(int sel, void *context) {
  switch (sel) {

    case MENU_MAIN_ENTRY_OPEN_DECRYPTED:
    {
      FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
      if (file_entry) {
        char path[MAX_PATH_LENGTH];
        int res;

        pfsUmount();

        snprintf(path, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);
        res = pfsMount(path);

        // In case we're at ux0:patch or grw0:patch we need to apply the mounting at ux0:app or gro0:app
        if (res < 0) {
          if (strncasecmp(file_list.path, "ux0:patch", 9) == 0 ||
              strncasecmp(file_list.path, "grw0:patch", 10) == 0) {
            snprintf(path, MAX_PATH_LENGTH, "ux0:app/%s", file_entry->name);
            res = pfsMount(path);

            if (res < 0) {
              snprintf(path, MAX_PATH_LENGTH, "gro0:app/%s", file_entry->name);
              res = pfsMount(path);
            }
          }
        }

        if (res < 0)
          errorDialog(res);

        if (res >= 0) {
          addEndSlash(file_list.path);
          strcat(file_list.path, file_entry->name);
          strcpy(pfs_mounted_path, file_list.path);
          dirLevelUp();

          // Open folder
          int res = refreshFileList();
          if (res < 0)
            errorDialog(res);
        } else {
          errorDialog(res);
        }
      }

      break;
    }

    case MENU_MAIN_ENTRY_MARK_UNMARK_ALL:
    {
      FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
      if (file_entry) {
        int on_marked_entry = 0;
        int length = mark_list.length;

        if (fileListFindEntry(&mark_list, file_entry->name))
          on_marked_entry = 1;

        // Empty mark list
        fileListEmpty(&mark_list);

        // Mark all if not all entries are marked yet and we are not focusing on a marked entry
        if (length != (file_list.length - 1) && !on_marked_entry) {
          FileListEntry *file_entry = file_list.head->next; // Ignore '..'

          int i;
          for (i = 0; i < file_list.length - 1; i++) {
            fileListAddEntry(&mark_list, fileListCopyEntry(file_entry), SORT_NONE);

            // Next
            file_entry = file_entry->next;
          }
        }
      }

      break;
    }

    case MENU_MAIN_ENTRY_MOVE:
    case MENU_MAIN_ENTRY_COPY:
    {
      FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
      if (file_entry) {
        // Umount if last path copied from is the pfs mounted path
        if (pfs_mounted_path[0] &&
            !strstr(file_list.path, pfs_mounted_path) &&
             strstr(copy_list.path, pfs_mounted_path)) {
          pfsUmount();
        }

        // Mode
        if (sel == MENU_MAIN_ENTRY_MOVE) {
          copy_mode = COPY_MODE_MOVE;
        } else {
          copy_mode = isInArchive() ? COPY_MODE_EXTRACT : COPY_MODE_NORMAL;
        }

        strcpy(archive_copy_path, archive_path);

        // Empty copy list at first
        fileListEmpty(&copy_list);

        // Paths
        if (fileListFindEntry(&mark_list, file_entry->name)) { // On marked entry
          // Copy mark list to copy list
          FileListEntry *mark_entry = mark_list.head;

          int i;
          for (i = 0; i < mark_list.length; i++) {
            fileListAddEntry(&copy_list, fileListCopyEntry(mark_entry), SORT_NONE);

            // Next
            mark_entry = mark_entry->next;
          }
        } else {
          fileListAddEntry(&copy_list, fileListCopyEntry(file_entry), SORT_NONE);
        }

        strcpy(copy_list.path, file_list.path);
        copy_list.is_in_archive = isInArchive();

        char *message;

        // On marked entry
        if (copy_list.length > 1 && fileListFindEntry(&copy_list, file_entry->name)) {
          message = language_container[COPIED_FILES_FOLDERS];
        } else {
          message = language_container[file_entry->is_folder ? COPIED_FOLDER : COPIED_FILE];
        }

        // Copy message
        infoDialog(message, copy_list.length);
      }

      break;
    }

    case MENU_MAIN_ENTRY_PASTE:
    {
      int copy_text = 0;

      switch (copy_mode) {
        case COPY_MODE_NORMAL:
          copy_text = COPYING;
          break;

        case COPY_MODE_MOVE:
          copy_text = MOVING;
          break;

        case COPY_MODE_EXTRACT:
          copy_text = EXTRACTING;
          break;
      }

      initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[copy_text]);
      setDialogStep(DIALOG_STEP_PASTE);
      break;
    }

    case MENU_MAIN_ENTRY_DELETE:
    {
      FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
      if (file_entry) {
        char *message;

        // On marked entry
        if (mark_list.length > 1 && fileListFindEntry(&mark_list, file_entry->name)) {
          message = language_container[DELETE_FILES_FOLDERS_QUESTION];
        } else {
          message = language_container[file_entry->is_folder ? DELETE_FOLDER_QUESTION : DELETE_FILE_QUESTION];
        }

        initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, message);
        setDialogStep(DIALOG_STEP_DELETE_QUESTION);
      }

      break;
    }

    case MENU_MAIN_ENTRY_RENAME:
    {
      FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
      if (file_entry) {
        char name[MAX_NAME_LENGTH];
        strcpy(name, file_entry->name);
        removeEndSlash(name);

        initImeDialog(language_container[RENAME], name, MAX_NAME_LENGTH, SCE_IME_TYPE_DEFAULT, 0, 0);

        setDialogStep(DIALOG_STEP_RENAME);
      }

      break;
    }

    case MENU_MAIN_ENTRY_PROPERTIES:
    {
      FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
      if (file_entry) {
        snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);
        initPropertyDialog(cur_file, file_entry);
      }

      break;
    }

    case MENU_MAIN_ENTRY_NEW:
    {
      setContextMenu(&context_menu_new);
      setContextMenuNewVisibilities();
      return CONTEXT_MENU_MORE_OPENING;
    }

    case MENU_MAIN_ENTRY_MORE:
    {
      setContextMenu(&context_menu_more);
      setContextMenuMoreVisibilities();
      return CONTEXT_MENU_MORE_OPENING;
    }

    case MENU_MAIN_ENTRY_SORT_BY:
    {
      setContextMenu(&context_menu_sort);
      setContextMenuSortVisibilities();
      return CONTEXT_MENU_MORE_OPENING;
    }
  }

  return CONTEXT_MENU_CLOSING;
}

static int contextMenuSortEnterCallback(int sel, void *context) {
  switch (sel) {
    case MENU_SORT_ENTRY_BY_NAME:
      sort_mode = SORT_BY_NAME;
      break;

    case MENU_SORT_ENTRY_BY_SIZE:
      sort_mode = SORT_BY_SIZE;
      break;

    case MENU_SORT_ENTRY_BY_DATE:
      sort_mode = SORT_BY_DATE;
      break;
  }

  last_set_sort_mode = sort_mode;

  // Refresh list
  refreshFileList();

  return CONTEXT_MENU_CLOSING;
}

static int contextMenuMoreEnterCallback(int sel, void *context) {
  switch (sel) {
    case MENU_MORE_ENTRY_COMPRESS:
    {
      FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
      if (file_entry) {
        char path[MAX_NAME_LENGTH];

        // On marked entry
        if (mark_list.length > 1 && fileListFindEntry(&mark_list, file_entry->name)) {
          int end_slash = removeEndSlash(file_list.path);

          char *p = strrchr(file_list.path, '/');
          if (!p)
            p = strrchr(file_list.path, ':');

          if (strlen(p + 1) > 0) {
            strcpy(path, p + 1);
          } else {
            strncpy(path, file_list.path, p - file_list.path);
            path[p - file_list.path] = '\0';
          }

          if (end_slash)
            addEndSlash(file_list.path);
        } else {
          char *p = strrchr(file_entry->name, '.');
          if (!p)
            p = strrchr(file_entry->name, '/');
          if (!p)
            p = file_entry->name + strlen(file_entry->name);

          strncpy(path, file_entry->name, p-file_entry->name);
          path[p - file_entry->name] = '\0';
        }

        // Append .zip extension
        strcat(path, ".zip");

        initImeDialog(language_container[ARCHIVE_NAME], path, MAX_NAME_LENGTH, SCE_IME_TYPE_BASIC_LATIN, 0, 0);
        setDialogStep(DIALOG_STEP_COMPRESS_NAME);
      }

      break;
    }

    case MENU_MORE_ENTRY_INSTALL_ALL:
    {
      // Empty install list
      fileListEmpty(&install_list);

      FileListEntry *file_entry = file_list.head->next; // Ignore '..'

      int i;
      for (i = 0; i < file_list.length - 1; i++) {
        char path[MAX_PATH_LENGTH];
        snprintf(path, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);

        int type = getFileType(path);
        if (type == FILE_TYPE_VPK) {
          fileListAddEntry(&install_list, fileListCopyEntry(file_entry), SORT_NONE);
        }

        // Next
        file_entry = file_entry->next;
      }

      strcpy(install_list.path, file_list.path);

      initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[INSTALL_ALL_QUESTION]);
      setDialogStep(DIALOG_STEP_INSTALL_QUESTION);

      break;
    }

    case MENU_MORE_ENTRY_INSTALL_FOLDER:
    {
      FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
      if (file_entry) {
        snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);
        initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[INSTALL_FOLDER_QUESTION]);
        setDialogStep(DIALOG_STEP_INSTALL_QUESTION);
      }

      break;
    }

    case MENU_MORE_ENTRY_CALCULATE_SHA1:
    {
      // Ensure user wants to actually take the hash
      initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[HASH_FILE_QUESTION]);
      setDialogStep(DIALOG_STEP_HASH_QUESTION);
      break;
    }
  }

  return CONTEXT_MENU_CLOSING;
}
static int contextMenuNewEnterCallback(int sel, void *context) {
  switch (sel) {
    case MENU_NEW_FILE: {
      char path[MAX_PATH_LENGTH];
      int count = 1;
      while (1) {
        if (count == 1) {
          snprintf(path, MAX_PATH_LENGTH, "%s%s", file_list.path,
                   language_container[NEW_FILE]);
        } else {
          snprintf(path, MAX_PATH_LENGTH, "%s%s (%d)", file_list.path,
                   language_container[NEW_FILE], count);
        }
        if (!checkFileExist(path))
          break;

        count++;
      }
      initImeDialog(language_container[NEW_FILE], path + strlen(file_list.path),
                    MAX_NAME_LENGTH, SCE_IME_TYPE_DEFAULT, 0, 0);
      setDialogStep(DIALOG_STEP_NEW_FILE);
      break;
    };
    case MENU_NEW_FOLDER: {
      // Find a new folder name
      char path[MAX_PATH_LENGTH];

      int count = 1;
      while (1) {
        if (count == 1) {
          snprintf(path, MAX_PATH_LENGTH, "%s%s", file_list.path,
                   language_container[NEW_FOLDER]);
        } else {
          snprintf(path, MAX_PATH_LENGTH, "%s%s (%d)", file_list.path,
                   language_container[NEW_FOLDER], count);
        }

        if (!checkFolderExist(path))
          break;

        count++;
      }

      initImeDialog(language_container[NEW_FOLDER], path + strlen(file_list.path),
                    MAX_NAME_LENGTH, SCE_IME_TYPE_DEFAULT, 0, 0);
      setDialogStep(DIALOG_STEP_NEW_FOLDER);
      break;
    };
  }
  // Refresh list
  refreshFileList();

  return CONTEXT_MENU_CLOSING;
}
