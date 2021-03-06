/** @file
  This function deal with the legacy boot option, it create, delete
  and manage the legacy boot option, all legacy boot option is getting from
  the legacy BBS table.

Copyright (c) 2004 - 2009, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BBSsupport.h"

/**

  Translate the first n characters of an Ascii string to
  Unicode characters. The count n is indicated by parameter
  Size. If Size is greater than the length of string, then
  the entire string is translated.


  @param AStr               Pointer to input Ascii string.
  @param Size               The number of characters to translate.
  @param UStr               Pointer to output Unicode string buffer.

**/
VOID
AsciiToUnicodeSize (
  IN UINT8              *AStr,
  IN UINTN              Size,
  OUT UINT16            *UStr
  )
{
  UINTN Idx;

  Idx = 0;
  while (AStr[Idx] != 0) {
    UStr[Idx] = (CHAR16) AStr[Idx];
    if (Idx == Size) {
      break;
    }

    Idx++;
  }
  UStr[Idx] = 0;
}

/**
  Build Legacy Device Name String according.

  @param CurBBSEntry     BBS Table.
  @param Index           Index.
  @param BufSize         The buffer size.
  @param BootString      The output string.

**/
VOID
BdsBuildLegacyDevNameString (
  IN  BBS_TABLE                 *CurBBSEntry,
  IN  UINTN                     Index,
  IN  UINTN                     BufSize,
  OUT CHAR16                    *BootString
  )
{
  CHAR16  *Fmt;
  CHAR16  *Type;
  UINT8   *StringDesc;
  CHAR16  Temp[80];

  switch (Index) {
  //
  // Primary Master
  //
  case 1:
    Fmt = L"Primary Master %s";
    break;

 //
 // Primary Slave
 //
  case 2:
    Fmt = L"Primary Slave %s";
    break;

  //
  // Secondary Master
  //
  case 3:
    Fmt = L"Secondary Master %s";
    break;

  //
  // Secondary Slave
  //
  case 4:
    Fmt = L"Secondary Slave %s";
    break;

  default:
    Fmt = L"%s";
    break;
  }

  switch (CurBBSEntry->DeviceType) {
  case BBS_FLOPPY:
    Type = L"Floppy";
    break;

  case BBS_HARDDISK:
    Type = L"Harddisk";
    break;

  case BBS_CDROM:
    Type = L"CDROM";
    break;

  case BBS_PCMCIA:
    Type = L"PCMCIAe";
    break;

  case BBS_USB:
    Type = L"USB";
    break;

  case BBS_EMBED_NETWORK:
    Type = L"Network";
    break;

  case BBS_BEV_DEVICE:
    Type = L"BEVe";
    break;

  case BBS_UNKNOWN:
  default:
    Type = L"Unknown";
    break;
  }
  //
  // If current BBS entry has its description then use it.
  //
  StringDesc = (UINT8 *) (UINTN) ((CurBBSEntry->DescStringSegment << 4) + CurBBSEntry->DescStringOffset);
  if (NULL != StringDesc) {
    //
    // Only get fisrt 32 characters, this is suggested by BBS spec
    //
    AsciiToUnicodeSize (StringDesc, 32, Temp);
    Fmt   = L"%s";
    Type  = Temp;
  }

  //
  // BbsTable 16 entries are for onboard IDE.
  // Set description string for SATA harddisks, Harddisk 0 ~ Harddisk 11
  //
  if (Index >= 5 && Index <= 16 && CurBBSEntry->DeviceType == BBS_HARDDISK) {
    Fmt = L"%s %d";
    UnicodeSPrint (BootString, BufSize, Fmt, Type, Index - 5);
  } else {
    UnicodeSPrint (BootString, BufSize, Fmt, Type);
  }
}

/**

  Create a legacy boot option for the specified entry of
  BBS table, save it as variable, and append it to the boot
  order list.


  @param CurrentBbsEntry    Pointer to current BBS table.
  @param CurrentBbsDevPath  Pointer to the Device Path Protocol instance of BBS
  @param Index              Index of the specified entry in BBS table.
  @param BootOrderList      On input, the original boot order list.
                            On output, the new boot order list attached with the
                            created node.
  @param BootOrderListSize  On input, the original size of boot order list.
                            On output, the size of new boot order list.

  @retval  EFI_SUCCESS             Boot Option successfully created.
  @retval  EFI_OUT_OF_RESOURCES    Fail to allocate necessary memory.
  @retval  Other                   Error occurs while setting variable.

**/
EFI_STATUS
BdsCreateLegacyBootOption (
  IN BBS_TABLE                        *CurrentBbsEntry,
  IN EFI_DEVICE_PATH_PROTOCOL         *CurrentBbsDevPath,
  IN UINTN                            Index,
  IN OUT UINT16                       **BootOrderList,
  IN OUT UINTN                        *BootOrderListSize
  )
{
  EFI_STATUS           Status;
  UINT16               CurrentBootOptionNo;
  UINT16               BootString[10];
  UINT16               BootDesc[100];
  CHAR8                HelpString[100];
  UINT16               *NewBootOrderList;
  UINTN                BufferSize;
  UINTN                StringLen;
  VOID                 *Buffer;
  UINT8                *Ptr;
  UINT16               CurrentBbsDevPathSize;
  UINTN                BootOrderIndex;
  UINTN                BootOrderLastIndex;
  UINTN                ArrayIndex;
  BOOLEAN              IndexNotFound;
  BBS_BBS_DEVICE_PATH  *NewBbsDevPathNode;

  if ((*BootOrderList) == NULL) {
    CurrentBootOptionNo = 0;
  } else {
    for (ArrayIndex = 0; ArrayIndex < (UINTN) (*BootOrderListSize / sizeof (UINT16)); ArrayIndex++) {
      IndexNotFound = TRUE;
      for (BootOrderIndex = 0; BootOrderIndex < (UINTN) (*BootOrderListSize / sizeof (UINT16)); BootOrderIndex++) {
        if ((*BootOrderList)[BootOrderIndex] == ArrayIndex) {
          IndexNotFound = FALSE;
          break;
        }
      }

      if (!IndexNotFound) {
        continue;
      } else {
        break;
      }
    }

    CurrentBootOptionNo = (UINT16) ArrayIndex;
  }

  UnicodeSPrint (
    BootString,
    sizeof (BootString),
    L"Boot%04x",
    CurrentBootOptionNo
    );

  BdsBuildLegacyDevNameString (CurrentBbsEntry, Index, sizeof (BootDesc), BootDesc);

  //
  // Create new BBS device path node with description string
  //
  UnicodeStrToAsciiStr ((CONST CHAR16*)&BootDesc, (CHAR8*)&HelpString);

  StringLen = AsciiStrLen (HelpString);
  NewBbsDevPathNode = AllocateZeroPool (sizeof (BBS_BBS_DEVICE_PATH) + StringLen);
  if (NewBbsDevPathNode == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (NewBbsDevPathNode, CurrentBbsDevPath, sizeof (BBS_BBS_DEVICE_PATH));
  CopyMem (NewBbsDevPathNode->String, HelpString, StringLen + 1);
  SetDevicePathNodeLength (&(NewBbsDevPathNode->Header), sizeof (BBS_BBS_DEVICE_PATH) + StringLen);

  //
  // Create entire new CurrentBbsDevPath with end node
  //
  CurrentBbsDevPath = AppendDevicePathNode (
                        EndDevicePath,
                        (EFI_DEVICE_PATH_PROTOCOL *) NewBbsDevPathNode
                        );
   if (CurrentBbsDevPath == NULL) {
    FreePool (NewBbsDevPathNode);
    return EFI_OUT_OF_RESOURCES;
  }

  CurrentBbsDevPathSize = (UINT16) (GetDevicePathSize (CurrentBbsDevPath));

  BufferSize = sizeof (UINT32) +
    sizeof (UINT16) +
    StrSize (BootDesc) +
    CurrentBbsDevPathSize +
    sizeof (BBS_TABLE) +
    sizeof (UINT16);

  Buffer = AllocateZeroPool (BufferSize);
  if (Buffer == NULL) {
    FreePool (NewBbsDevPathNode);
    FreePool (CurrentBbsDevPath);
    return EFI_OUT_OF_RESOURCES;
  }

  Ptr               = (UINT8 *) Buffer;

  *((UINT32 *) Ptr) = LOAD_OPTION_ACTIVE;
  Ptr += sizeof (UINT32);

  *((UINT16 *) Ptr) = CurrentBbsDevPathSize;
  Ptr += sizeof (UINT16);

  CopyMem (
    Ptr,
    BootDesc,
    StrSize (BootDesc)
    );
  Ptr += StrSize (BootDesc);

  CopyMem (
    Ptr,
    CurrentBbsDevPath,
    CurrentBbsDevPathSize
    );
  Ptr += CurrentBbsDevPathSize;

  CopyMem (
    Ptr,
    CurrentBbsEntry,
    sizeof (BBS_TABLE)
    );

  Ptr += sizeof (BBS_TABLE);
  *((UINT16 *) Ptr) = (UINT16) Index;

  Status = gRT->SetVariable (
                  BootString,
                  &gEfiGlobalVariableGuid,
                  VAR_FLAG,
                  BufferSize,
                  Buffer
                  );

  FreePool (Buffer);
  
  Buffer = NULL;

  NewBootOrderList = AllocateZeroPool (*BootOrderListSize + sizeof (UINT16));
  if (NULL == NewBootOrderList) {
    FreePool (NewBbsDevPathNode);
    FreePool (CurrentBbsDevPath);
    return EFI_OUT_OF_RESOURCES;
  }

  if (*BootOrderList != NULL) {
    CopyMem (NewBootOrderList, *BootOrderList, *BootOrderListSize);
    FreePool (*BootOrderList);
  }

  BootOrderLastIndex                    = (UINTN) (*BootOrderListSize / sizeof (UINT16));
  NewBootOrderList[BootOrderLastIndex]  = CurrentBootOptionNo;
  *BootOrderListSize += sizeof (UINT16);
  *BootOrderList = NewBootOrderList;

  FreePool (NewBbsDevPathNode);
  FreePool (CurrentBbsDevPath);
  return Status;
}

/**
  Check if the boot option is a legacy one.

  @param BootOptionVar   The boot option data payload.
  @param BbsEntry        The BBS Table.
  @param BbsIndex        The table index.

  @retval TRUE           It is a legacy boot option.
  @retval FALSE          It is not a legacy boot option.

**/
BOOLEAN
BdsIsLegacyBootOption (
  IN UINT8                 *BootOptionVar,
  OUT BBS_TABLE            **BbsEntry,
  OUT UINT16               *BbsIndex
  )
{
  UINT8                     *Ptr;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  BOOLEAN                   Ret;
  UINT16                    DevPathLen;

  Ptr = BootOptionVar;
  Ptr += sizeof (UINT32);
  DevPathLen = *(UINT16 *) Ptr;
  Ptr += sizeof (UINT16);
  Ptr += StrSize ((UINT16 *) Ptr);
  DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) Ptr;
  if ((BBS_DEVICE_PATH == DevicePath->Type) && (BBS_BBS_DP == DevicePath->SubType)) {
    Ptr += DevPathLen;
    *BbsEntry = (BBS_TABLE *) Ptr;
    Ptr += sizeof (BBS_TABLE);
    *BbsIndex = *(UINT16 *) Ptr;
    Ret       = TRUE;
  } else {
    *BbsEntry = NULL;
    Ret       = FALSE;
  }

  return Ret;
}

/**
  Delete all the invalid legacy boot options.

  @retval EFI_SUCCESS             All invalide legacy boot options are deleted.
  @retval EFI_OUT_OF_RESOURCES    Fail to allocate necessary memory.
  @retval EFI_NOT_FOUND           Fail to retrive variable of boot order.
**/
EFI_STATUS
EFIAPI
BdsDeleteAllInvalidLegacyBootOptions (
  VOID
  )
{
  UINT16                    *BootOrder;
  UINT8                     *BootOptionVar;
  UINTN                     BootOrderSize;
  UINTN                     BootOptionSize;
  EFI_STATUS                Status;
  UINT16                    HddCount;
  UINT16                    BbsCount;
  HDD_INFO                  *LocalHddInfo;
  BBS_TABLE                 *LocalBbsTable;
  BBS_TABLE                 *BbsEntry;
  UINT16                    BbsIndex;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  UINTN                     Index;
  UINT16                    BootOption[10];
  UINT16                    BootDesc[100];
  BOOLEAN                   DescStringMatch;

  Status        = EFI_SUCCESS;
  BootOrder     = NULL;
  BootOrderSize = 0;
  HddCount      = 0;
  BbsCount      = 0;
  LocalHddInfo  = NULL;
  LocalBbsTable = NULL;
  BbsEntry      = NULL;

  Status        = EfiLibLocateProtocol (&gEfiLegacyBiosProtocolGuid, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  LegacyBios->GetBbsInfo (
                LegacyBios,
                &HddCount,
                &LocalHddInfo,
                &BbsCount,
                &LocalBbsTable
                );

  BootOrder = BdsLibGetVariableAndSize (
                L"BootOrder",
                &gEfiGlobalVariableGuid,
                &BootOrderSize
                );
  if (NULL == BootOrder) {
    return EFI_NOT_FOUND;
  }

  Index = 0;
  while (Index < BootOrderSize / sizeof (UINT16)) {
    UnicodeSPrint (BootOption, sizeof (BootOption), L"Boot%04x", BootOrder[Index]);
    BootOptionVar = BdsLibGetVariableAndSize (
                      BootOption,
                      &gEfiGlobalVariableGuid,
                      &BootOptionSize
                      );
    if (NULL == BootOptionVar) {
      if (BootOrder != NULL) {
        FreePool (BootOrder);
      }
      return EFI_OUT_OF_RESOURCES;
    }
  
    //
    // Skip Non-Legacy boot options
    // 
    if (!BdsIsLegacyBootOption (BootOptionVar, &BbsEntry, &BbsIndex)) {
      if (BootOptionVar!= NULL) {
        FreePool (BootOptionVar);
      }
      Index++;
      continue;
    }

    //
    // Check if BBS Description String is changed
    //
    DescStringMatch = FALSE;

    BdsBuildLegacyDevNameString (
      &LocalBbsTable[BbsIndex],
      BbsIndex,
      sizeof(BootDesc),
      BootDesc
      );

    if (StrCmp (BootDesc, (UINT16*)(BootOptionVar + sizeof (UINT32) + sizeof (UINT16))) == 0) {
      DescStringMatch = TRUE;
    }

    if (!((LocalBbsTable[BbsIndex].BootPriority == BBS_IGNORE_ENTRY) ||
          (LocalBbsTable[BbsIndex].BootPriority == BBS_DO_NOT_BOOT_FROM)) &&
        (LocalBbsTable[BbsIndex].DeviceType == BbsEntry->DeviceType) &&
        DescStringMatch) {
      Index++;
      continue;
    }

    if (BootOptionVar != NULL) {
      FreePool (BootOptionVar);
    }
    //
    // should delete
    //
    BdsDeleteBootOption (
      BootOrder[Index],
      BootOrder,
      &BootOrderSize
      );
  }

  //
  // Adjust the number of boot options.
  //
  if (BootOrderSize != 0) {
    Status = gRT->SetVariable (
                    L"BootOrder",
                    &gEfiGlobalVariableGuid,
                    VAR_FLAG,
                    BootOrderSize,
                    BootOrder
                    );
  } else {
    EfiLibDeleteVariable (L"BootOrder", &gEfiGlobalVariableGuid);
  }

  if (BootOrder != NULL) {
    FreePool (BootOrder);
  }

  return Status;
}

/**
  Find all legacy boot option by device type.

  @param BootOrder       The boot order array.
  @param BootOptionNum   The number of boot option.
  @param DevType         Device type.
  @param Attribute       The boot option attribute.
  @param BbsIndex        The BBS table index.
  @param OptionNumber    The boot option index.

  @retval TRUE           The Legacy boot option is found.
  @retval FALSE          The legacy boot option is not found.

**/
BOOLEAN
BdsFindLegacyBootOptionByDevType (
  IN UINT16                 *BootOrder,
  IN UINTN                  BootOptionNum,
  IN UINT16                 DevType,
  OUT UINT32                *Attribute,
  OUT UINT16                *BbsIndex,
  OUT UINTN                 *OptionNumber
  )
{
  UINTN     Index;
  UINTN     BootOrderIndex;
  UINT16    BootOption[100];
  UINTN     BootOptionSize;
  UINT8     *BootOptionVar;
  BBS_TABLE *BbsEntry;
  BOOLEAN   Found;

  BbsEntry  = NULL;
  Found     = FALSE;

  if (NULL == BootOrder) {
    return Found;
  }

  //
  // Loop all boot option from variable
  //
  for (BootOrderIndex = 0; BootOrderIndex < BootOptionNum; BootOrderIndex++) {
    Index = (UINTN) BootOrder[BootOrderIndex];
    UnicodeSPrint (BootOption, sizeof (BootOption), L"Boot%04x", Index);
    BootOptionVar = BdsLibGetVariableAndSize (
                      BootOption,
                      &gEfiGlobalVariableGuid,
                      &BootOptionSize
                      );
    if (NULL == BootOptionVar) {
      continue;
    }

    //
    // Skip Non-legacy boot option
    //
    if (!BdsIsLegacyBootOption (BootOptionVar, &BbsEntry, BbsIndex)) {
      FreePool (BootOptionVar);
      continue;
    }

    if (BbsEntry->DeviceType != DevType) {
      FreePool (BootOptionVar);
      continue;
    }

    *Attribute    = *(UINT32 *) BootOptionVar;
    *OptionNumber = Index;
    Found         = TRUE;
    FreePool (BootOptionVar);
    break;
  }

  return Found;
}

/**
  Create a legacy boot option.

  @param BbsItem         The BBS Table entry.
  @param Index           Index of the specified entry in BBS table.
  @param BootOrderList   The boot order list.
  @param BootOrderListSize The size of boot order list.

  @retval EFI_OUT_OF_RESOURCE  No enough memory.
  @retval EFI_SUCCESS          The function complete successfully.
  @return Other value if the legacy boot option is not created.

**/
EFI_STATUS
BdsCreateOneLegacyBootOption (
  IN BBS_TABLE              *BbsItem,
  IN UINTN                  Index,
  IN OUT UINT16             **BootOrderList,
  IN OUT UINTN              *BootOrderListSize
  )
{
  BBS_BBS_DEVICE_PATH       BbsDevPathNode;
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;

  DevPath                       = NULL;

  //
  // Create device path node.
  //
  BbsDevPathNode.Header.Type    = BBS_DEVICE_PATH;
  BbsDevPathNode.Header.SubType = BBS_BBS_DP;
  SetDevicePathNodeLength (&BbsDevPathNode.Header, sizeof (BBS_BBS_DEVICE_PATH));
  BbsDevPathNode.DeviceType = BbsItem->DeviceType;
  CopyMem (&BbsDevPathNode.StatusFlag, &BbsItem->StatusFlags, sizeof (UINT16));

  DevPath = AppendDevicePathNode (
              EndDevicePath,
              (EFI_DEVICE_PATH_PROTOCOL *) &BbsDevPathNode
              );
  if (NULL == DevPath) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = BdsCreateLegacyBootOption (
            BbsItem,
            DevPath,
            Index,
            BootOrderList,
            BootOrderListSize
            );
  BbsItem->BootPriority = 0x00;

  FreePool (DevPath);

  return Status;
}

/**
  Add the legacy boot options from BBS table if they do not exist.

  @retval EFI_SUCCESS          The boot options are added successfully 
                               or they are already in boot options.
  @retval EFI_NOT_FOUND        No legacy boot options is found.
  @retval EFI_OUT_OF_RESOURCE  No enough memory.
  @return Other value          LegacyBoot options are not added.
**/
EFI_STATUS
EFIAPI
BdsAddNonExistingLegacyBootOptions (
  VOID
  )
{
  UINT16                    *BootOrder;
  UINTN                     BootOrderSize;
  EFI_STATUS                Status;
  UINT16                    HddCount;
  UINT16                    BbsCount;
  HDD_INFO                  *LocalHddInfo;
  BBS_TABLE                 *LocalBbsTable;
  UINT16                    BbsIndex;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  UINTN                     Index;
  UINT32                    Attribute;
  UINTN                     OptionNumber;
  BOOLEAN                   Ret;

  BootOrder     = NULL;
  HddCount      = 0;
  BbsCount      = 0;
  LocalHddInfo  = NULL;
  LocalBbsTable = NULL;

  Status        = EfiLibLocateProtocol (&gEfiLegacyBiosProtocolGuid, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  LegacyBios->GetBbsInfo (
                LegacyBios,
                &HddCount,
                &LocalHddInfo,
                &BbsCount,
                &LocalBbsTable
                );

  BootOrder = BdsLibGetVariableAndSize (
                L"BootOrder",
                &gEfiGlobalVariableGuid,
                &BootOrderSize
                );
  if (NULL == BootOrder) {
    BootOrderSize = 0;
  }

  for (Index = 0; Index < BbsCount; Index++) {
    if ((LocalBbsTable[Index].BootPriority == BBS_IGNORE_ENTRY) ||
        (LocalBbsTable[Index].BootPriority == BBS_DO_NOT_BOOT_FROM)
        ) {
      continue;
    }

    Ret = BdsFindLegacyBootOptionByDevType (
            BootOrder,
            BootOrderSize / sizeof (UINT16),
            LocalBbsTable[Index].DeviceType,
            &Attribute,
            &BbsIndex,
            &OptionNumber
            );
    if (Ret) {
      continue;
    }

    //
    // Not found such type of legacy device in boot options or we found but it's disabled
    // so we have to create one and put it to the tail of boot order list
    //
    Status = BdsCreateOneLegacyBootOption (
              &LocalBbsTable[Index],
              Index,
              &BootOrder,
              &BootOrderSize
              );
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  if (BootOrderSize > 0) {
    Status = gRT->SetVariable (
                    L"BootOrder",
                    &gEfiGlobalVariableGuid,
                    VAR_FLAG,
                    BootOrderSize,
                    BootOrder
                    );
  } else {
    EfiLibDeleteVariable (L"BootOrder", &gEfiGlobalVariableGuid);
  }

  if (BootOrder != NULL) {
    FreePool (BootOrder);
  }

  return Status;
}

/**
  Fill the device order buffer.

  @param BbsTable        The BBS table.
  @param BbsType         The BBS Type.
  @param BbsCount        The BBS Count.
  @param Buf             device order buffer.

  @return The device order buffer.

**/
UINT16 *
BdsFillDevOrderBuf (
  IN BBS_TABLE                    *BbsTable,
  IN BBS_TYPE                     BbsType,
  IN UINTN                        BbsCount,
  OUT UINT16                       *Buf
  )
{
  UINTN Index;

  for (Index = 0; Index < BbsCount; Index++) {
    if (BbsTable[Index].BootPriority == BBS_IGNORE_ENTRY) {
      continue;
    }

    if (BbsTable[Index].DeviceType != BbsType) {
      continue;
    }

    *Buf = (UINT16) (Index & 0xFF);
    Buf++;
  }

  return Buf;
}

/**
  Create the device order buffer.

  @param BbsTable        The BBS table.
  @param BbsCount        The BBS Count.

  @retval EFI_SUCCES             The buffer is created and the EFI variable named 
                                 VAR_LEGACY_DEV_ORDER and EfiLegacyDevOrderGuid is
                                 set correctly.
  @retval EFI_OUT_OF_RESOURCES   Memmory or storage is not enough.
  @retval EFI_DEVICE_ERROR       Fail to add the device order into EFI variable fail
                                 because of hardware error.
**/
EFI_STATUS
BdsCreateDevOrder (
  IN BBS_TABLE                  *BbsTable,
  IN UINT16                     BbsCount
  )
{
  UINTN       Index;
  UINTN       FDCount;
  UINTN       HDCount;
  UINTN       CDCount;
  UINTN       NETCount;
  UINTN       BEVCount;
  UINTN       TotalSize;
  UINTN       HeaderSize;
  UINT8       *DevOrder;
  UINT8       *Ptr;
  EFI_STATUS  Status;

  FDCount     = 0;
  HDCount     = 0;
  CDCount     = 0;
  NETCount    = 0;
  BEVCount    = 0;
  TotalSize   = 0;
  HeaderSize  = sizeof (BBS_TYPE) + sizeof (UINT16);
  DevOrder    = NULL;
  Ptr         = NULL;
  Status      = EFI_SUCCESS;

  //
  // Count all boot devices
  //
  for (Index = 0; Index < BbsCount; Index++) {
    if (BbsTable[Index].BootPriority == BBS_IGNORE_ENTRY) {
      continue;
    }

    switch (BbsTable[Index].DeviceType) {
    case BBS_FLOPPY:
      FDCount++;
      break;

    case BBS_HARDDISK:
      HDCount++;
      break;

    case BBS_CDROM:
      CDCount++;
      break;

    case BBS_EMBED_NETWORK:
      NETCount++;
      break;

    case BBS_BEV_DEVICE:
      BEVCount++;
      break;

    default:
      break;
    }
  }

  TotalSize += (HeaderSize + sizeof (UINT16) * FDCount);
  TotalSize += (HeaderSize + sizeof (UINT16) * HDCount);
  TotalSize += (HeaderSize + sizeof (UINT16) * CDCount);
  TotalSize += (HeaderSize + sizeof (UINT16) * NETCount);
  TotalSize += (HeaderSize + sizeof (UINT16) * BEVCount);

  //
  // Create buffer to hold all boot device order
  //
  DevOrder = AllocateZeroPool (TotalSize);
  if (NULL == DevOrder) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ptr                 = DevOrder;

  *((BBS_TYPE *) Ptr) = BBS_FLOPPY;
  Ptr += sizeof (BBS_TYPE);
  *((UINT16 *) Ptr) = (UINT16) (sizeof (UINT16) + FDCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);
  if (FDCount != 0) {
    Ptr = (UINT8 *) BdsFillDevOrderBuf (BbsTable, BBS_FLOPPY, BbsCount, (UINT16 *) Ptr);
  }

  *((BBS_TYPE *) Ptr) = BBS_HARDDISK;
  Ptr += sizeof (BBS_TYPE);
  *((UINT16 *) Ptr) = (UINT16) (sizeof (UINT16) + HDCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);
  if (HDCount != 0) {
    Ptr = (UINT8 *) BdsFillDevOrderBuf (BbsTable, BBS_HARDDISK, BbsCount, (UINT16 *) Ptr);
  }

  *((BBS_TYPE *) Ptr) = BBS_CDROM;
  Ptr += sizeof (BBS_TYPE);
  *((UINT16 *) Ptr) = (UINT16) (sizeof (UINT16) + CDCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);
  if (CDCount != 0) {
    Ptr = (UINT8 *) BdsFillDevOrderBuf (BbsTable, BBS_CDROM, BbsCount, (UINT16 *) Ptr);
  }

  *((BBS_TYPE *) Ptr) = BBS_EMBED_NETWORK;
  Ptr += sizeof (BBS_TYPE);
  *((UINT16 *) Ptr) = (UINT16) (sizeof (UINT16) + NETCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);
  if (NETCount != 0) {
    Ptr = (UINT8 *) BdsFillDevOrderBuf (BbsTable, BBS_EMBED_NETWORK, BbsCount, (UINT16 *) Ptr);
  }

  *((BBS_TYPE *) Ptr) = BBS_BEV_DEVICE;
  Ptr += sizeof (BBS_TYPE);
  *((UINT16 *) Ptr) = (UINT16) (sizeof (UINT16) + BEVCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);
  if (BEVCount != 0) {
    Ptr = (UINT8 *) BdsFillDevOrderBuf (BbsTable, BBS_BEV_DEVICE, BbsCount, (UINT16 *) Ptr);
  }

  //
  // Save device order for legacy boot device to variable.
  //
  Status = gRT->SetVariable (
                  VAR_LEGACY_DEV_ORDER,
                  &EfiLegacyDevOrderGuid,
                  VAR_FLAG,
                  TotalSize,
                  DevOrder
                  );
  FreePool (DevOrder);

  return Status;
}

/**
  Add the legacy boot devices from BBS table into 
  the legacy device boot order.

  @retval EFI_SUCCESS           The boot devices are added successfully.
  @retval EFI_NOT_FOUND         The legacy boot devices are not found.
  @retval EFI_OUT_OF_RESOURCES  Memmory or storage is not enough.
  @retval EFI_DEVICE_ERROR      Fail to add the legacy device boot order into EFI variable
                                because of hardware error.
**/
EFI_STATUS
EFIAPI
BdsUpdateLegacyDevOrder (
  VOID
  )
{
  UINT8                     *DevOrder;
  UINT8                     *NewDevOrder;
  UINTN                     DevOrderSize;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  EFI_STATUS                Status;
  UINT16                    HddCount;
  UINT16                    BbsCount;
  HDD_INFO                  *LocalHddInfo;
  BBS_TABLE                 *LocalBbsTable;
  UINTN                     Index;
  UINTN                     Index2;
  UINTN                     *Idx;
  UINTN                     FDCount;
  UINTN                     HDCount;
  UINTN                     CDCount;
  UINTN                     NETCount;
  UINTN                     BEVCount;
  UINTN                     TotalSize;
  UINTN                     HeaderSize;
  UINT8                     *Ptr;
  UINT8                     *NewPtr;
  UINT16                    *NewFDPtr;
  UINT16                    *NewHDPtr;
  UINT16                    *NewCDPtr;
  UINT16                    *NewNETPtr;
  UINT16                    *NewBEVPtr;
  UINT16                    *NewDevPtr;
  UINT16                    Length;
  UINT16                    Tmp;
  UINTN                     FDIndex;
  UINTN                     HDIndex;
  UINTN                     CDIndex;
  UINTN                     NETIndex;
  UINTN                     BEVIndex;

  LocalHddInfo  = NULL;
  LocalBbsTable = NULL;
  Idx           = NULL;
  FDCount       = 0;
  HDCount       = 0;
  CDCount       = 0;
  NETCount      = 0;
  BEVCount      = 0;
  TotalSize     = 0;
  HeaderSize    = sizeof (BBS_TYPE) + sizeof (UINT16);
  FDIndex       = 0;
  HDIndex       = 0;
  CDIndex       = 0;
  NETIndex      = 0;
  BEVIndex      = 0;
  NewDevPtr     = NULL;

  Status        = EfiLibLocateProtocol (&gEfiLegacyBiosProtocolGuid, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  LegacyBios->GetBbsInfo (
                LegacyBios,
                &HddCount,
                &LocalHddInfo,
                &BbsCount,
                &LocalBbsTable
                );

  DevOrder = (UINT8 *) BdsLibGetVariableAndSize (
                        VAR_LEGACY_DEV_ORDER,
                        &EfiLegacyDevOrderGuid,
                        &DevOrderSize
                        );
  if (NULL == DevOrder) {
    return BdsCreateDevOrder (LocalBbsTable, BbsCount);
  }
  //
  // First we figure out how many boot devices with same device type respectively
  //
  for (Index = 0; Index < BbsCount; Index++) {
    if ((LocalBbsTable[Index].BootPriority == BBS_IGNORE_ENTRY) ||
        (LocalBbsTable[Index].BootPriority == BBS_DO_NOT_BOOT_FROM)
        ) {
      continue;
    }

    switch (LocalBbsTable[Index].DeviceType) {
    case BBS_FLOPPY:
      FDCount++;
      break;

    case BBS_HARDDISK:
      HDCount++;
      break;

    case BBS_CDROM:
      CDCount++;
      break;

    case BBS_EMBED_NETWORK:
      NETCount++;
      break;

    case BBS_BEV_DEVICE:
      BEVCount++;
      break;

    default:
      break;
    }
  }

  TotalSize += (HeaderSize + FDCount * sizeof (UINT16));
  TotalSize += (HeaderSize + HDCount * sizeof (UINT16));
  TotalSize += (HeaderSize + CDCount * sizeof (UINT16));
  TotalSize += (HeaderSize + NETCount * sizeof (UINT16));
  TotalSize += (HeaderSize + BEVCount * sizeof (UINT16));

  NewDevOrder = AllocateZeroPool (TotalSize);
  if (NULL == NewDevOrder) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewFDPtr  = (UINT16 *) (NewDevOrder + HeaderSize);
  NewHDPtr  = (UINT16 *) ((UINT8 *) NewFDPtr + FDCount * sizeof (UINT16) + HeaderSize);
  NewCDPtr  = (UINT16 *) ((UINT8 *) NewHDPtr + HDCount * sizeof (UINT16) + HeaderSize);
  NewNETPtr = (UINT16 *) ((UINT8 *) NewCDPtr + CDCount * sizeof (UINT16) + HeaderSize);
  NewBEVPtr = (UINT16 *) ((UINT8 *) NewNETPtr + NETCount * sizeof (UINT16) + HeaderSize);

  //
  // copy FD
  //
  Ptr                     = DevOrder;
  NewPtr                  = NewDevOrder;
  *((BBS_TYPE *) NewPtr)  = *((BBS_TYPE *) Ptr);
  Ptr += sizeof (BBS_TYPE);
  NewPtr += sizeof (BBS_TYPE);
  Length                = *((UINT16 *) Ptr);
  *((UINT16 *) NewPtr)  = (UINT16) (sizeof (UINT16) + FDCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);

  for (Index = 0; Index < Length / sizeof (UINT16) - 1; Index++) {
    if (LocalBbsTable[*Ptr].BootPriority == BBS_IGNORE_ENTRY ||
        LocalBbsTable[*Ptr].BootPriority == BBS_DO_NOT_BOOT_FROM ||
        LocalBbsTable[*Ptr].DeviceType != BBS_FLOPPY
        ) {
      Ptr += sizeof (UINT16);
      continue;
    }

    NewFDPtr[FDIndex] = *(UINT16 *) Ptr;
    FDIndex++;
    Ptr += sizeof (UINT16);
  }
  //
  // copy HD
  //
  NewPtr                  = (UINT8 *) NewHDPtr - HeaderSize;
  *((BBS_TYPE *) NewPtr)  = *((BBS_TYPE *) Ptr);
  Ptr += sizeof (BBS_TYPE);
  NewPtr += sizeof (BBS_TYPE);
  Length                = *((UINT16 *) Ptr);
  *((UINT16 *) NewPtr)  = (UINT16) (sizeof (UINT16) + HDCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);

  for (Index = 0; Index < Length / sizeof (UINT16) - 1; Index++) {
    if (LocalBbsTable[*Ptr].BootPriority == BBS_IGNORE_ENTRY ||
        LocalBbsTable[*Ptr].BootPriority == BBS_DO_NOT_BOOT_FROM ||
        LocalBbsTable[*Ptr].BootPriority == BBS_LOWEST_PRIORITY ||
        LocalBbsTable[*Ptr].DeviceType != BBS_HARDDISK
        ) {
      Ptr += sizeof (UINT16);
      continue;
    }

    NewHDPtr[HDIndex] = *(UINT16 *) Ptr;
    HDIndex++;
    Ptr += sizeof (UINT16);
  }
  //
  // copy CD
  //
  NewPtr                  = (UINT8 *) NewCDPtr - HeaderSize;
  *((BBS_TYPE *) NewPtr)  = *((BBS_TYPE *) Ptr);
  Ptr += sizeof (BBS_TYPE);
  NewPtr += sizeof (BBS_TYPE);
  Length                = *((UINT16 *) Ptr);
  *((UINT16 *) NewPtr)  = (UINT16) (sizeof (UINT16) + CDCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);

  for (Index = 0; Index < Length / sizeof (UINT16) - 1; Index++) {
    if (LocalBbsTable[*Ptr].BootPriority == BBS_IGNORE_ENTRY ||
        LocalBbsTable[*Ptr].BootPriority == BBS_DO_NOT_BOOT_FROM ||
        LocalBbsTable[*Ptr].BootPriority == BBS_LOWEST_PRIORITY ||
        LocalBbsTable[*Ptr].DeviceType != BBS_CDROM
        ) {
      Ptr += sizeof (UINT16);
      continue;
    }

    NewCDPtr[CDIndex] = *(UINT16 *) Ptr;
    CDIndex++;
    Ptr += sizeof (UINT16);
  }
  //
  // copy NET
  //
  NewPtr                  = (UINT8 *) NewNETPtr - HeaderSize;
  *((BBS_TYPE *) NewPtr)  = *((BBS_TYPE *) Ptr);
  Ptr += sizeof (BBS_TYPE);
  NewPtr += sizeof (BBS_TYPE);
  Length                = *((UINT16 *) Ptr);
  *((UINT16 *) NewPtr)  = (UINT16) (sizeof (UINT16) + NETCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);

  for (Index = 0; Index < Length / sizeof (UINT16) - 1; Index++) {
    if (LocalBbsTable[*Ptr].BootPriority == BBS_IGNORE_ENTRY ||
        LocalBbsTable[*Ptr].BootPriority == BBS_DO_NOT_BOOT_FROM ||
        LocalBbsTable[*Ptr].BootPriority == BBS_LOWEST_PRIORITY ||
        LocalBbsTable[*Ptr].DeviceType != BBS_EMBED_NETWORK
        ) {
      Ptr += sizeof (UINT16);
      continue;
    }

    NewNETPtr[NETIndex] = *(UINT16 *) Ptr;
    NETIndex++;
    Ptr += sizeof (UINT16);
  }
  //
  // copy BEV
  //
  NewPtr                  = (UINT8 *) NewBEVPtr - HeaderSize;
  *((BBS_TYPE *) NewPtr)  = *((BBS_TYPE *) Ptr);
  Ptr += sizeof (BBS_TYPE);
  NewPtr += sizeof (BBS_TYPE);
  Length                = *((UINT16 *) Ptr);
  *((UINT16 *) NewPtr)  = (UINT16) (sizeof (UINT16) + BEVCount * sizeof (UINT16));
  Ptr += sizeof (UINT16);

  for (Index = 0; Index < Length / sizeof (UINT16) - 1; Index++) {
    if (LocalBbsTable[*Ptr].BootPriority == BBS_IGNORE_ENTRY ||
        LocalBbsTable[*Ptr].BootPriority == BBS_DO_NOT_BOOT_FROM ||
        LocalBbsTable[*Ptr].BootPriority == BBS_LOWEST_PRIORITY ||
        LocalBbsTable[*Ptr].DeviceType != BBS_BEV_DEVICE
        ) {
      Ptr += sizeof (UINT16);
      continue;
    }

    NewBEVPtr[BEVIndex] = *(UINT16 *) Ptr;
    BEVIndex++;
    Ptr += sizeof (UINT16);
  }

  for (Index = 0; Index < BbsCount; Index++) {
    if ((LocalBbsTable[Index].BootPriority == BBS_IGNORE_ENTRY) ||
        (LocalBbsTable[Index].BootPriority == BBS_DO_NOT_BOOT_FROM)
        ) {
      continue;
    }

    switch (LocalBbsTable[Index].DeviceType) {
    case BBS_FLOPPY:
      Idx       = &FDIndex;
      NewDevPtr = NewFDPtr;
      break;

    case BBS_HARDDISK:
      Idx       = &HDIndex;
      NewDevPtr = NewHDPtr;
      break;

    case BBS_CDROM:
      Idx       = &CDIndex;
      NewDevPtr = NewCDPtr;
      break;

    case BBS_EMBED_NETWORK:
      Idx       = &NETIndex;
      NewDevPtr = NewNETPtr;
      break;

    case BBS_BEV_DEVICE:
      Idx       = &BEVIndex;
      NewDevPtr = NewBEVPtr;
      break;

    default:
      Idx = NULL;
      break;
    }
    //
    // at this point we have copied those valid indexes to new buffer
    // and we should check if there is any new appeared boot device
    //
    if (Idx != 0) {
      for (Index2 = 0; Index2 < *Idx; Index2++) {
        if ((NewDevPtr[Index2] & 0xFF) == (UINT16) Index) {
          break;
        }
      }

      if (Index2 == *Idx) {
        //
        // Index2 == *Idx means we didn't find Index
        // so Index is a new appeared device's index in BBS table
        // save it.
        //
        NewDevPtr[*Idx] = (UINT16) (Index & 0xFF);
        (*Idx)++;
      }
    }
  }

  if (FDCount != 0) {
    //
    // Just to make sure that disabled indexes are all at the end of the array
    //
    for (Index = 0; Index < FDIndex - 1; Index++) {
      if (0xFF00 != (NewFDPtr[Index] & 0xFF00)) {
        continue;
      }

      for (Index2 = Index + 1; Index2 < FDIndex; Index2++) {
        if (0 == (NewFDPtr[Index2] & 0xFF00)) {
          Tmp               = NewFDPtr[Index];
          NewFDPtr[Index]   = NewFDPtr[Index2];
          NewFDPtr[Index2]  = Tmp;
          break;
        }
      }
    }
  }

  if (HDCount != 0) {
    //
    // Just to make sure that disabled indexes are all at the end of the array
    //
    for (Index = 0; Index < HDIndex - 1; Index++) {
      if (0xFF00 != (NewHDPtr[Index] & 0xFF00)) {
        continue;
      }

      for (Index2 = Index + 1; Index2 < HDIndex; Index2++) {
        if (0 == (NewHDPtr[Index2] & 0xFF00)) {
          Tmp               = NewHDPtr[Index];
          NewHDPtr[Index]   = NewHDPtr[Index2];
          NewHDPtr[Index2]  = Tmp;
          break;
        }
      }
    }
  }

  if (CDCount != 0) {
    //
    // Just to make sure that disabled indexes are all at the end of the array
    //
    for (Index = 0; Index < CDIndex - 1; Index++) {
      if (0xFF00 != (NewCDPtr[Index] & 0xFF00)) {
        continue;
      }

      for (Index2 = Index + 1; Index2 < CDIndex; Index2++) {
        if (0 == (NewCDPtr[Index2] & 0xFF00)) {
          Tmp               = NewCDPtr[Index];
          NewCDPtr[Index]   = NewCDPtr[Index2];
          NewCDPtr[Index2]  = Tmp;
          break;
        }
      }
    }
  }

  if (NETCount != 0) {
    //
    // Just to make sure that disabled indexes are all at the end of the array
    //
    for (Index = 0; Index < NETIndex - 1; Index++) {
      if (0xFF00 != (NewNETPtr[Index] & 0xFF00)) {
        continue;
      }

      for (Index2 = Index + 1; Index2 < NETIndex; Index2++) {
        if (0 == (NewNETPtr[Index2] & 0xFF00)) {
          Tmp               = NewNETPtr[Index];
          NewNETPtr[Index]  = NewNETPtr[Index2];
          NewNETPtr[Index2] = Tmp;
          break;
        }
      }
    }
  }

  if (BEVCount!= 0) {
    //
    // Just to make sure that disabled indexes are all at the end of the array
    //
    for (Index = 0; Index < BEVIndex - 1; Index++) {
      if (0xFF00 != (NewBEVPtr[Index] & 0xFF00)) {
        continue;
      }

      for (Index2 = Index + 1; Index2 < BEVIndex; Index2++) {
        if (0 == (NewBEVPtr[Index2] & 0xFF00)) {
          Tmp               = NewBEVPtr[Index];
          NewBEVPtr[Index]  = NewBEVPtr[Index2];
          NewBEVPtr[Index2] = Tmp;
          break;
        }
      }
    }
  }

  FreePool (DevOrder);

  Status = gRT->SetVariable (
                  VAR_LEGACY_DEV_ORDER,
                  &EfiLegacyDevOrderGuid,
                  VAR_FLAG,
                  TotalSize,
                  NewDevOrder
                  );
  FreePool (NewDevOrder);

  return Status;
}

/**
  Set Boot Priority for specified device type.

  @param DeviceType      The device type.
  @param LocalBbsTable   The BBS table.
  @param Priority        The prority table.

  @retval EFI_SUCCESS           The function completes successfully.
  @retval EFI_NOT_FOUND         Failed to find device.
  @retval EFI_OUT_OF_RESOURCES  Failed to get the efi variable of device order.

**/
EFI_STATUS
BdsSetBootPriority4SameTypeDev (
  IN UINT16                                              DeviceType,
  IN OUT BBS_TABLE                                       *LocalBbsTable,
  IN OUT UINT16                                          *Priority
  )
{
  UINT8   *DevOrder;

  UINT8   *OrigBuffer;
  UINT16  *DevIndex;
  UINTN   DevOrderSize;
  UINTN   DevCount;
  UINTN   Index;

  DevOrder = BdsLibGetVariableAndSize (
              VAR_LEGACY_DEV_ORDER,
              &EfiLegacyDevOrderGuid,
              &DevOrderSize
              );
  if (NULL == DevOrder) {
    return EFI_OUT_OF_RESOURCES;
  }

  OrigBuffer = DevOrder;
  while (DevOrder < OrigBuffer + DevOrderSize) {
    if (DeviceType == * (BBS_TYPE *) DevOrder) {
      break;
    }

    DevOrder += sizeof (BBS_TYPE);
    DevOrder += *(UINT16 *) DevOrder;
  }

  if (DevOrder >= OrigBuffer + DevOrderSize) {
    FreePool (OrigBuffer);
    return EFI_NOT_FOUND;
  }

  DevOrder += sizeof (BBS_TYPE);
  DevCount  = (*((UINT16 *) DevOrder) - sizeof (UINT16)) / sizeof (UINT16);
  DevIndex  = (UINT16 *) (DevOrder + sizeof (UINT16));
  //
  // If the high byte of the DevIndex is 0xFF, it indicates that this device has been disabled.
  //
  for (Index = 0; Index < DevCount; Index++) {
    if ((DevIndex[Index] & 0xFF00) == 0xFF00) {
      //
      // LocalBbsTable[DevIndex[Index] & 0xFF].BootPriority = BBS_DISABLED_ENTRY;
      //
    } else {
      LocalBbsTable[DevIndex[Index] & 0xFF].BootPriority = *Priority;
      (*Priority)++;
    }
  }

  FreePool (OrigBuffer);
  return EFI_SUCCESS;
}

/**
  Print the BBS Table.

  @param LocalBbsTable   The BBS table.
  @param BbsCount        The count of entry in BBS table.
**/
VOID
PrintBbsTable (
  IN BBS_TABLE  *LocalBbsTable,
  IN UINT16     BbsCount
  )
{
  UINT16  Idx;

  DEBUG ((DEBUG_ERROR, "\n"));
  DEBUG ((DEBUG_ERROR, " NO  Prio bb/dd/ff cl/sc Type Stat segm:offs\n"));
  DEBUG ((DEBUG_ERROR, "=============================================\n"));
  for (Idx = 0; Idx < BbsCount; Idx++) {
    if ((LocalBbsTable[Idx].BootPriority == BBS_IGNORE_ENTRY) ||
        (LocalBbsTable[Idx].BootPriority == BBS_DO_NOT_BOOT_FROM) ||
        (LocalBbsTable[Idx].BootPriority == BBS_LOWEST_PRIORITY)
        ) {
      continue;
    }

    DEBUG (
      (DEBUG_ERROR,
      " %02x: %04x %02x/%02x/%02x %02x/%02x %04x %04x %04x:%04x\n",
      (UINTN) Idx,
      (UINTN) LocalBbsTable[Idx].BootPriority,
      (UINTN) LocalBbsTable[Idx].Bus,
      (UINTN) LocalBbsTable[Idx].Device,
      (UINTN) LocalBbsTable[Idx].Function,
      (UINTN) LocalBbsTable[Idx].Class,
      (UINTN) LocalBbsTable[Idx].SubClass,
      (UINTN) LocalBbsTable[Idx].DeviceType,
      (UINTN) * (UINT16 *) &LocalBbsTable[Idx].StatusFlags,
      (UINTN) LocalBbsTable[Idx].BootHandlerSegment,
      (UINTN) LocalBbsTable[Idx].BootHandlerOffset,
      (UINTN) ((LocalBbsTable[Idx].MfgStringSegment << 4) + LocalBbsTable[Idx].MfgStringOffset),
      (UINTN) ((LocalBbsTable[Idx].DescStringSegment << 4) + LocalBbsTable[Idx].DescStringOffset))
      );
  }

  DEBUG ((DEBUG_ERROR, "\n"));
}

/**
  Set the boot priority for BBS entries based on boot option entry and boot order.

  @param  Entry             The boot option is to be checked for refresh BBS table.
  
  @retval EFI_SUCCESS           The boot priority for BBS entries is refreshed successfully.
  @retval EFI_NOT_FOUND         BBS entries can't be found.
  @retval EFI_OUT_OF_RESOURCES  Failed to get the legacy device boot order.
**/
EFI_STATUS
EFIAPI
BdsRefreshBbsTableForBoot (
  IN BDS_COMMON_OPTION        *Entry
  )
{
  EFI_STATUS                Status;
  UINT16                    HddCount;
  UINT16                    BbsCount;
  HDD_INFO                  *LocalHddInfo;
  BBS_TABLE                 *LocalBbsTable;
  UINT16                    DevType;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  UINTN                     Index;
  UINT16                    Priority;
  UINT16                    *BootOrder;
  UINTN                     BootOrderSize;
  UINT8                     *BootOptionVar;
  UINTN                     BootOptionSize;
  UINT16                    BootOption[100];
  UINT8                     *Ptr;
  UINT16                    DevPathLen;
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;

  HddCount      = 0;
  BbsCount      = 0;
  LocalHddInfo  = NULL;
  LocalBbsTable = NULL;
  DevType       = BBS_UNKNOWN;

  Status        = EfiLibLocateProtocol (&gEfiLegacyBiosProtocolGuid, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  LegacyBios->GetBbsInfo (
                LegacyBios,
                &HddCount,
                &LocalHddInfo,
                &BbsCount,
                &LocalBbsTable
                );
  //
  // First, set all the present devices' boot priority to BBS_UNPRIORITIZED_ENTRY
  // We will set them according to the settings setup by user
  //
  for (Index = 0; Index < BbsCount; Index++) {
    if (!((BBS_IGNORE_ENTRY == LocalBbsTable[Index].BootPriority) ||
        (BBS_DO_NOT_BOOT_FROM == LocalBbsTable[Index].BootPriority) ||
         (BBS_LOWEST_PRIORITY == LocalBbsTable[Index].BootPriority))) {
      LocalBbsTable[Index].BootPriority = BBS_UNPRIORITIZED_ENTRY;
    }
  }
  //
  // boot priority always starts at 0
  //
  Priority = 0;
  if (Entry->LoadOptionsSize == sizeof (BBS_TABLE) + sizeof (UINT16)) {
    //
    // If Entry stands for a legacy boot option, we prioritize the devices with the same type first.
    //
    DevType = ((BBS_TABLE *) Entry->LoadOptions)->DeviceType;
    Status = BdsSetBootPriority4SameTypeDev (
              DevType,
              LocalBbsTable,
              &Priority
              );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  //
  // we have to set the boot priority for other BBS entries with different device types
  //
  BootOrder = (UINT16 *) BdsLibGetVariableAndSize (
                          L"BootOrder",
                          &gEfiGlobalVariableGuid,
                          &BootOrderSize
                          );
  for (Index = 0; ((BootOrder != NULL) && (Index < BootOrderSize / sizeof (UINT16))); Index++) {
    UnicodeSPrint (BootOption, sizeof (BootOption), L"Boot%04x", BootOrder[Index]);
    BootOptionVar = BdsLibGetVariableAndSize (
                      BootOption,
                      &gEfiGlobalVariableGuid,
                      &BootOptionSize
                      );
    if (NULL == BootOptionVar) {
      continue;
    }

    Ptr = BootOptionVar;

    Ptr += sizeof (UINT32);
    DevPathLen = *(UINT16 *) Ptr;
    Ptr += sizeof (UINT16);
    Ptr += StrSize ((UINT16 *) Ptr);
    DevPath = (EFI_DEVICE_PATH_PROTOCOL *) Ptr;
    if (BBS_DEVICE_PATH != DevPath->Type || BBS_BBS_DP != DevPath->SubType) {
      FreePool (BootOptionVar);
      continue;
    }

    Ptr += DevPathLen;
    if (DevType == ((BBS_TABLE *) Ptr)->DeviceType) {
      //
      // We don't want to process twice for a device type
      //
      FreePool (BootOptionVar);
      continue;
    }

    Status = BdsSetBootPriority4SameTypeDev (
              ((BBS_TABLE *) Ptr)->DeviceType,
              LocalBbsTable,
              &Priority
              );
    FreePool (BootOptionVar);
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  if (BootOrder != NULL) {
    FreePool (BootOrder);
  }

  DEBUG_CODE_BEGIN();
    PrintBbsTable (LocalBbsTable, BbsCount);
  DEBUG_CODE_END();
  
  return Status;
}
