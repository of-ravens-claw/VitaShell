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

#ifndef __LANGUAGE_H__
#define __LANGUAGE_H__

enum LanguageContainer {
  // General strings
  ERROR,
  OK,
  YES,
  NO,
  CANCEL,
  ON,
  OFF,
  FILE_,
  FOLDER,

  // Progress strings
  MOVING,
  COPYING,
  DELETING,
  EXPORTING,
  INSTALLING,
  DOWNLOADING,
  EXTRACTING,
  COMPRESSING,
  HASHING,
  REFRESHING,
  SENDING,
  RECEIVING,

  // Audio player strings
  TITLE,
  ALBUM,
  ARTIST,
  GENRE,
  YEAR,

  // Hex editor strings
  OFFSET,
  OPEN_HEX_EDITOR,

  // Text editor strings
  EDIT_LINE,
  ENTER_SEARCH_TERM,

  // Context menu strings
  SORT_BY,
  BY_NAME,
  BY_SIZE,
  BY_DATE,
  MARK_ALL,
  UNMARK_ALL,
  MOVE,
  COPY,
  PASTE,
  DELETE,
  RENAME,
  NEW,
  NEW_FILE,
  NEW_FOLDER,
  PROPERTIES,
  SEND,
  RECEIVE,
  OPEN_DECRYPTED,
  EXPORT_MEDIA,
  CUT,
  INSERT_EMPTY_LINE,
  SEARCH,

  // File browser properties strings
  PROPERTY_NAME,
  PROPERTY_TYPE,
  PROPERTY_FSELF_MODE,
  PROPERTY_SIZE,
  PROPERTY_COMPRESSED_SIZE,
  PROPERTY_CONTAINS,
  PROPERTY_CONTAINS_FILES_FOLDERS,
  PROPERTY_CREATION_DATE,
  PROPERTY_MODFICATION_DATE,
  PROPERTY_TYPE_ARCHIVE,
  PROPERTY_TYPE_INI,
  PROPERTY_TYPE_SFO,
  PROPERTY_TYPE_TXT,
  PROPERTY_TYPE_VPK,
  PROPERTY_TYPE_XML,
  PROPERTY_TYPE_FSELF,
  PROPERTY_FSELF_MODE_SAFE,
  PROPERTY_FSELF_MODE_UNSAFE,
  PROPERTY_FSELF_MODE_SCE,

  // File browser strings
  COPIED_FILE,
  COPIED_FOLDER,
  COPIED_FILES_FOLDERS,

  // Dialog questions
  DELETE_FILE_QUESTION,
  DELETE_FOLDER_QUESTION,
  DELETE_FILES_FOLDERS_QUESTION,
  INSTALL_ALL_QUESTION,
  INSTALL_FOLDER_QUESTION,
  INSTALL_QUESTION,
  INSTALL_WARNING,
  INSTALL_BRICK_WARNING,
  HASH_FILE_QUESTION,
  SAVE_MODIFICATIONS,

  // VitaShell settings
  VITASHELL_SETTINGS_MAIN,
  VITASHELL_SETTINGS_RESTART_SHELL,
  VITASHELL_SETTINGS_POWER,
  VITASHELL_SETTINGS_REBOOT,
  VITASHELL_SETTINGS_POWEROFF,
  VITASHELL_SETTINGS_STANDBY,

  // Others
  SAFE_MODE,
  UNSAFE_MODE,
  PLEASE_WAIT,
  MEMORY_CARD_NOT_FOUND,
  GAME_CARD_NOT_FOUND,
  MICROSD_NOT_FOUND,
  NO_SPACE_ERROR,
  EXTENDED_PERMISSIONS_REQUIRED,
  WIFI_ERROR,
  FTP_SERVER,
  UPDATE_QUESTION,
  ARCHIVE_NAME,
  COMPRESSION_LEVEL,
  ENTER_PASSWORD,
  
  LANGUAGE_CONTAINER_SIZE,
};

extern char *language_container[LANGUAGE_CONTAINER_SIZE];

void freeLanguageContainer();
void loadLanguage(int id);

#endif
