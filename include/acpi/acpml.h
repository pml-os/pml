#ifndef __ACPML_H
#define __ACPML_H

#define ACPI_USE_SYSTEM_CLIBRARY

#define ACPI_MACHINE_WIDTH                      64
#define COMPILER_DEPENDENT_INT64                long
#define COMPILER_DEPENDENT_UINT64               unsigned long

#define ACPI_MSG_ERROR                          "ACPI: error: "
#define ACPI_MSG_EXCEPTION                      ACPI_MSG_ERROR
#define ACPI_MSG_WARNING                        "ACPI: warning: "
#define ACPI_MSG_INFO                           "ACPI: "

#define ACPI_CACHE_T                            ACPI_MEMORY_LIST
#define ACPI_USE_LOCAL_CACHE                    1

#endif
