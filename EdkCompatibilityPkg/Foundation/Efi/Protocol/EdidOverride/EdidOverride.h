/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EdidOverride.h

Abstract:

  EDID Override Protocol from the UEFI 2.0 specification.

  Allow platform to provide EDID information to producer of the Graphics Output
  protocol.

--*/

#ifndef __EDID_OVERRIDE_H__
#define __EDID_OVERRIDE_H__

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (EdidDiscovered)

#define EFI_EDID_OVERRIDE_PROTOCOL_GUID \
  { \
    0x48ecb431, 0xfb72, 0x45c0, {0xa9, 0x22, 0xf4, 0x58, 0xfe, 0x4, 0xb, 0xd5} \
  }

typedef struct _EFI_EDID_OVERRIDE_PROTOCOL EFI_EDID_OVERRIDE_PROTOCOL;

#define EFI_EDID_OVERRIDE_DONT_OVERRIDE   0x01
#define EFI_EDID_OVERRIDE_ENABLE_HOT_PLUG 0x02

typedef
EFI_STATUS
(EFIAPI *EFI_EDID_OVERRIDE_PROTOCOL_GET_EDID) (
  IN  EFI_EDID_OVERRIDE_PROTOCOL                  *This,
  IN  EFI_HANDLE                                  *ChildHandle,
  OUT UINT32                                      *Attributes,
  IN OUT UINTN                                    *EdidSize,
  IN OUT UINT8                                    **Edid
  )
/*++

  Routine Description:
    Return the current video mode information.

  Arguments:
    This                  - Protocol instance pointer.
    ChildHandle           - A child handle produced by the Graphics Output EFI
                            driver that represents a video output device. 
    Attributes            - The attributes associated with ChildHandle video output device.
    EdidSize              - A pointer to the size, in bytes, of the Edid buffer.
    Edid                  - A pointer to callee allocated buffer that contains the EDID that
                            should be used for ChildHandle. A value of NULL 
                            represents no EDID override for ChildHandle.

  Returns:
    EFI_SUCCESS           - Valid overrides returned for ChildHandle.
    EFI_UNSUPPORTED       - ChildHandle has no overrides.

--*/
;

struct _EFI_EDID_OVERRIDE_PROTOCOL {
  EFI_EDID_OVERRIDE_PROTOCOL_GET_EDID   GetEdid;
};

extern EFI_GUID gEfiEdidOverrideProtocolGuid;

#endif
