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

#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/io/fcntl.h>

#include <stdio.h>
#include <string.h>

#include <taihen.h>

#include "vitashell_kernel.h"

#define MOUNT_POINT_ID 0x800

int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);
int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);

int ksceNpDrmGetRifVitaKey(void *license_buf, uint8_t *klicensee, uint32_t *flags,
                           uint32_t *sku_flag, uint64_t *start_time, uint64_t *expiration_time);

typedef struct {
  const char *dev;
  const char *dev2;
  const char *blkdev;
  const char *blkdev2;
  int id;
} SceIoDevice;

typedef struct {
  int id;
  const char *dev_unix;
  int unk;
  int dev_major;
  int dev_minor;
  const char *dev_filesystem;
  int unk2;
  SceIoDevice *dev;
  int unk3;
  SceIoDevice *dev2;
  int unk4;
  int unk5;
  int unk6;
  int unk7;
} SceIoMountPoint;

static char ux0_blkdev[64], ux0_blkdev2[64];

static SceIoDevice ux0_dev = { "ux0:", "exfatux0", ux0_blkdev, ux0_blkdev2, MOUNT_POINT_ID };

static SceIoMountPoint *(* sceIoFindMountPoint)(int id) = NULL;

static SceUID hookid = -1;

static tai_hook_ref_t ksceSysrootIsSafeModeRef;

static tai_hook_ref_t ksceSblAimgrIsDolceRef;

static int ksceSysrootIsSafeModePatched(void) {
  return 1;
}

static int ksceSblAimgrIsDolcePatched(void) {
  return 1;
}

int shellKernelIsUx0Redirected(const char *blkdev, const char *blkdev2) {
  char k_blkdev[64], k_blkdev2[64];

  uint32_t state;
  ENTER_SYSCALL(state);

  SceIoMountPoint *mount = sceIoFindMountPoint(MOUNT_POINT_ID);
  if (!mount) {
    EXIT_SYSCALL(state);
    return 0;
  }

  ksceKernelStrncpyUserToKernel(k_blkdev, blkdev, sizeof(k_blkdev) - 1);
  ksceKernelStrncpyUserToKernel(k_blkdev2, blkdev2, sizeof(k_blkdev2) - 1);

  if (mount && mount->dev && mount->dev->blkdev && strcmp(mount->dev->blkdev, k_blkdev) == 0) {
    EXIT_SYSCALL(state);
    return 1;
  }

  EXIT_SYSCALL(state);
  return 0;
}

int shellKernelRedirectUx0(const char *blkdev, const char *blkdev2) {
  uint32_t state;
  ENTER_SYSCALL(state);

  SceIoMountPoint *mount = sceIoFindMountPoint(MOUNT_POINT_ID);
  if (!mount) {
    EXIT_SYSCALL(state);
    return -1;
  }

  ksceKernelStrncpyUserToKernel(ux0_blkdev, blkdev, sizeof(ux0_blkdev) - 1);
  ksceKernelStrncpyUserToKernel(ux0_blkdev2, blkdev2, sizeof(ux0_blkdev2) - 1);

  mount->dev = &ux0_dev;
  mount->dev2 = &ux0_dev;

  EXIT_SYSCALL(state);
  return 0;
}

int _shellKernelMountById(ShellMountIdArgs *args) {
  int res;

  void *(* sceAppMgrFindProcessInfoByPid)(void *data, SceUID pid);
  int (* sceAppMgrMountById)(SceUID pid, void *info, int id, const char *titleid, const char *path,
                             const char *desired_mount_point, const void *klicensee, char *mount_point);
  int (* _ksceKernelGetModuleInfo)(SceUID pid, SceUID modid, SceKernelModuleInfo *info);

  // Get tai module info
  tai_module_info_t tai_info;
  tai_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceAppMgr", &tai_info) < 0)
    return SCE_KERNEL_START_SUCCESS;

  int info_offset = 0;
  switch (tai_info.module_nid) {
    case 0xF1102066: // 3.30
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x3E01, (uintptr_t *)&sceAppMgrFindProcessInfoByPid);
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x1CEFD, (uintptr_t *)&sceAppMgrMountById);
      info_offset = 0x578;
			break;

    case 0xA1BE0ED6: // 3.35
    case 0x6ABD8ABA: // 3.36
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x3E01, (uintptr_t *)&sceAppMgrFindProcessInfoByPid);
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x1CF29, (uintptr_t *)&sceAppMgrMountById);
      info_offset = 0x578;
			break;

    case 0xE2EDFCC3: // 3.50
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x2DE1, (uintptr_t *)&sceAppMgrFindProcessInfoByPid);
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x198E9, (uintptr_t *)&sceAppMgrMountById);
      info_offset = 0x580;
			break;

    case 0x8C76997D: // 3.51
    case 0x1D32C3FF: // 3.52
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x2DE1, (uintptr_t *)&sceAppMgrFindProcessInfoByPid);
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x19D5D, (uintptr_t *)&sceAppMgrMountById);
      info_offset = 0x580;
			break;

    case 0x94CEFE4B: // 3.55
    case 0xDFBC288C: // 3.57
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x2DE1, (uintptr_t *)&sceAppMgrFindProcessInfoByPid);
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x19E15, (uintptr_t *)&sceAppMgrMountById);
      info_offset = 0x580;
      break;

    case 0xDBB29DB7: // 3.60
    case 0xB5F8EA7C: // 3.61
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x2DE1, (uintptr_t *)&sceAppMgrFindProcessInfoByPid);
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x19B51, (uintptr_t *)&sceAppMgrMountById);
      info_offset = 0x580;
      break;

    case 0x23B967C5: // 3.63
    case 0x1C9879D6: // 3.65
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x2DE1, (uintptr_t *)&sceAppMgrFindProcessInfoByPid);
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x19E61, (uintptr_t *)&sceAppMgrMountById);
      info_offset = 0x580;
      break;

    case 0x54E2E984: // 3.67
    case 0xC3C538DE: // 3.68
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x2DE1, (uintptr_t *)&sceAppMgrFindProcessInfoByPid);
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x19E6D, (uintptr_t *)&sceAppMgrMountById);
      info_offset = 0x580;
      break;

    case 0x321E4852: // 3.69
    case 0x700DA0CD: // 3.70
    case 0xF7846B4E: // 3.71
    case 0xA8E80BA8: // 3.72
    case 0xB299D195: // 3.73
    case 0x30007BD3: // 3.74
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x2DE9, (uintptr_t *)&sceAppMgrFindProcessInfoByPid);
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x19E95, (uintptr_t *)&sceAppMgrMountById);
      info_offset = 0x580;
      break;
  }

  res = module_get_export_func(KERNEL_PID, "SceKernelModulemgr",
                               0xC445FA63, 0xD269F915, (uintptr_t *)&_ksceKernelGetModuleInfo);
  if (res < 0)
    res = module_get_export_func(KERNEL_PID, "SceKernelModulemgr",
                                 0x92C9FFC2, 0xDAA90093, (uintptr_t *)&_ksceKernelGetModuleInfo);
  if (res < 0)
    return res;

  // Module info
  SceKernelModuleInfo mod_info;
  mod_info.size = sizeof(SceKernelModuleInfo);
  res = _ksceKernelGetModuleInfo(KERNEL_PID, tai_info.modid, &mod_info);
  if (res < 0)
    return res;

  uint32_t appmgr_data_addr = (uint32_t)mod_info.segments[1].vaddr;

  SceUID process_id = ksceKernelGetProcessId();

  void *info = sceAppMgrFindProcessInfoByPid((void *)(appmgr_data_addr + 0x500), process_id);
  if (!info)
    return -1;

  char process_titleid[12];
  char path[256];
  char desired_mount_point[16];
  char mount_point[16];
  char klicensee[16];

  memset(mount_point, 0, sizeof(mount_point));

  if (args->process_titleid)
    ksceKernelStrncpyUserToKernel(process_titleid, args->process_titleid, 11);
  if (args->path)
    ksceKernelStrncpyUserToKernel(path, args->path, 255);
  if (args->desired_mount_point)
    ksceKernelStrncpyUserToKernel(desired_mount_point, args->desired_mount_point, 15);
  if (args->klicensee)
    ksceKernelMemcpyUserToKernel(klicensee, args->klicensee, 0x10);

  res = sceAppMgrMountById(process_id,
                           info + info_offset,
                           args->id,
                           args->process_titleid ? process_titleid : NULL,
                           args->path ? path : NULL,
                           args->desired_mount_point ? desired_mount_point : NULL,
                           args->klicensee ? klicensee : NULL,
                           mount_point);

  if (args->mount_point)
    ksceKernelStrncpyKernelToUser(args->mount_point, mount_point, 15);

  return res;
}

int shellKernelMountById(ShellMountIdArgs *args) {
  uint32_t state;
  ENTER_SYSCALL(state);

  ShellMountIdArgs k_args;
  ksceKernelMemcpyUserToKernel(&k_args, args, sizeof(ShellMountIdArgs));

  int res = ksceKernelRunWithStack(0x2000, (void *)_shellKernelMountById, &k_args);

  EXIT_SYSCALL(state);
  return res;
}

int shellKernelGetRifVitaKey(const void *license_buf, void *klicensee) {
  char k_license_buf[0x200];
  char k_klicensee[0x10];

  memset(k_klicensee, 0, sizeof(k_klicensee));

  if (license_buf)
    ksceKernelMemcpyUserToKernel(k_license_buf, license_buf, sizeof(k_license_buf));

  int res = ksceNpDrmGetRifVitaKey(k_license_buf, (uint8_t *)k_klicensee, NULL, NULL, NULL, NULL);

  if (klicensee)
    ksceKernelMemcpyKernelToUser(klicensee, k_klicensee, sizeof(k_klicensee));

  return res;
}

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
  SceUID tmp1, tmp2;
  // Get tai module info
  tai_module_info_t info;
  info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceIofilemgr", &info) < 0)
    return SCE_KERNEL_START_SUCCESS;

  // Get important function
  switch (info.module_nid) {
    case 0x2BDA8288: // 3.30
    case 0xA79798A7: // 3.35
    case 0x2167E848: // 3.36
      module_get_offset(KERNEL_PID, info.modid, 0, 0x13BB5, (uintptr_t *)&sceIoFindMountPoint);
      break;
    
    case 0xE33310C2: // 3.50
    case 0xC29134F9: // 3.51
    case 0x5AD3B43C: // 3.52
      module_get_offset(KERNEL_PID, info.modid, 0, 0x13855, (uintptr_t *)&sceIoFindMountPoint);
      break;

    case 0x7A1DBDE6: // 3.55
    case 0xEF58597E: // 3.57
    case 0x9642948C: // 3.60
      module_get_offset(KERNEL_PID, info.modid, 0, 0x138C1, (uintptr_t *)&sceIoFindMountPoint);
      break;

    case 0xE923F19C: // 3.61
      module_get_offset(KERNEL_PID, info.modid, 0, 0x138D5, (uintptr_t *)&sceIoFindMountPoint);
      break;

    case 0x5FC2B87D: // 3.63
    case 0xA96ACE9D: // 3.65
    case 0x3347A95F: // 3.67
    case 0x90DA33DE: // 3.68
      module_get_offset(KERNEL_PID, info.modid, 0, 0x182F5, (uintptr_t *)&sceIoFindMountPoint);
      break;

    case 0xF16E72C7: // 3.69
    case 0x81A49C2B: // 3.70
    case 0xF2D59083: // 3.71
    case 0x9C16D40A: // 3.72
    case 0xF7794A6C: // 3.73
    case 0x796DAFAF: // 3.74
      module_get_offset(KERNEL_PID, info.modid, 0, 0x18735, (uintptr_t *)&sceIoFindMountPoint);
      break;

    default:
      return SCE_KERNEL_START_SUCCESS;
  }

  // Fake safe mode so that SceUsbMass can be loaded
  tmp1 = taiHookFunctionExportForKernel(KERNEL_PID, &ksceSysrootIsSafeModeRef, "SceSysmem",
                                        0x2ED7F97A, 0x834439A7, ksceSysrootIsSafeModePatched);
  if (tmp1 < 0)
    return SCE_KERNEL_START_SUCCESS;
  // this patch is only needed on handheld units
  tmp2 = taiHookFunctionExportForKernel(KERNEL_PID, &ksceSblAimgrIsDolceRef, "SceSysmem",
                                        0xFD00C69A, 0x71608CA3, ksceSblAimgrIsDolcePatched);
  if (tmp2 < 0)
    return SCE_KERNEL_START_SUCCESS;

  // Load SceUsbMass
  SceUID modid = ksceKernelLoadStartModule("ux0:VitaShell/module/umass.skprx", 0, NULL, 0, NULL, NULL);

  // Release patch
  taiHookReleaseForKernel(tmp1, ksceSysrootIsSafeModeRef);
  taiHookReleaseForKernel(tmp2, ksceSblAimgrIsDolceRef);

  // Check result
  if (modid < 0)
    return SCE_KERNEL_START_SUCCESS;

  // Fake safe mode in SceUsbServ
  hookid = taiHookFunctionImportForKernel(KERNEL_PID, &ksceSysrootIsSafeModeRef, "SceUsbServ",
                                          0x2ED7F97A, 0x834439A7, ksceSysrootIsSafeModePatched);

  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
  if (hookid >= 0)
    taiHookReleaseForKernel(hookid, ksceSysrootIsSafeModeRef);

  return SCE_KERNEL_STOP_SUCCESS;
}
