/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    IsaAcpi.c
    
Abstract:

    EFI ISA ACPI Protocol

Revision History

--*/

#include "Tiano.h"

#include EFI_PROTOCOL_DEFINITION (IsaAcpi)

EFI_GUID  gEfiIsaAcpiProtocolGuid = EFI_ISA_ACPI_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiIsaAcpiProtocolGuid, "ISA Acpi Protocol", "ISA Acpi Protocol");
