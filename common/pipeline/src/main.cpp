#include "Logger.hpp"
#include "dmvc/dmvc.h"

#include <cstdint>

#define DEVICE_LIST_UPDATE_TIMEOUT  1000  /* millisecond */

// --- Protótipos ---
DcError PrintSystemInfo(DcSystem system, Falcor::GeneralLogger& logger);
DcError PrintInterfaceInfo(DcInterface interface, Falcor::GeneralLogger& logger);
DcError PrintDeviceInfo2(DcDevice device, int64_t device_index, Falcor::GeneralLogger& logger);

int main()
{
    // Inicia o Quill
    auto* quill_logger = quill::create_logger("GENERAL");
    Falcor::GeneralLogger logger(quill_logger);

    DcSystem system;
    DcInterface interface;
    DcDevice device;
    size_t interface_count = 0;
    size_t device_count = 0;

    // Criação do Sistema
    if (DcSystemCreate(&system) != DC_ERROR_SUCCESS) {
        logger.log_error("Falha ao criar DcSystem");
        return -1;
    }

    PrintSystemInfo(system, logger);

    // Atualiza lista de interfaces
    DcSystemUpdateInterfaceList(system, NULL, DC_INFINITE);
    DcSystemGetInterfaceCount(system, &interface_count);

    logger.log_info("Found {} interfaces", (uint32_t)interface_count);

    for (uint32_t i = 0; i < interface_count; i++)
    {
        logger.log_info("Interface {}", i);

        if (DcSystemGetInterface(system, i, &interface) == DC_ERROR_SUCCESS) {
            PrintInterfaceInfo(interface, logger);

            if (DcInterfaceOpen(interface) == DC_ERROR_SUCCESS) {
                DcInterfaceUpdateDeviceList(interface, NULL, DEVICE_LIST_UPDATE_TIMEOUT);
                DcInterfaceGetDeviceCount(interface, &device_count);
                
                logger.log_info("  Found {} device(s) on interface", (uint32_t)device_count);

                for (uint32_t j = 0; j < device_count; j++)
                {
                    logger.log_info("    Device {}", j);
                    if (DcInterfaceGetDevice(interface, j, &device) == DC_ERROR_SUCCESS) {
                        PrintDeviceInfo2(device, j, logger);
                    }
                }
                DcInterfaceClose(interface);
            }
        }
    }

    DcSystemDestroy(system);
    logger.log_info("Finished device enumeration");

    return 0;
}

// --- Implementações (O que estava faltando para o Linker) ---

DcError PrintSystemInfo(DcSystem system, Falcor::GeneralLogger& logger)
{
    char buffer[256] = {0};
    size_t size = sizeof(buffer);

    if (DcSystemGetInfo(system, DC_SYSTEM_INFO_VENDOR, buffer, &size) == DC_ERROR_SUCCESS)
        logger.log_info("System Vendor: {}", buffer);

    size = sizeof(buffer);
    if (DcSystemGetInfo(system, DC_SYSTEM_INFO_MODEL, buffer, &size) == DC_ERROR_SUCCESS)
        logger.log_info("System Model: {}", buffer);

    return DC_ERROR_SUCCESS;
}

DcError PrintInterfaceInfo(DcInterface interface, Falcor::GeneralLogger& logger)
{
    char buffer[256] = {0};
    size_t size = sizeof(buffer);

    if (DcInterfaceGetInfo(interface, DC_INTERFACE_INFO_ID, buffer, &size) == DC_ERROR_SUCCESS)
        logger.log_info("  Interface ID: {}", buffer);

    size = sizeof(buffer);
    if (DcInterfaceGetInfo(interface, DC_INTERFACE_INFO_DISPLAY_NAME, buffer, &size) == DC_ERROR_SUCCESS)
        logger.log_info("  Interface Display Name: {}", buffer);

    return DC_ERROR_SUCCESS;
}

DcError PrintDeviceInfo2(DcDevice device, int64_t device_index, Falcor::GeneralLogger& logger)
{
    char buffer[256] = {0};
    size_t size = sizeof(buffer);

    if (DcDeviceGetInfo(device, DC_DEVICE_INFO_VENDOR, buffer, &size) == DC_ERROR_SUCCESS)
        logger.log_info("    Device Vendor: {}", buffer);

    size = sizeof(buffer);
    if (DcDeviceGetInfo(device, DC_DEVICE_INFO_MODEL, buffer, &size) == DC_ERROR_SUCCESS)
        logger.log_info("    Device Model: {}", buffer);

    size = sizeof(buffer);
    if (DcDeviceGetInfo(device, DC_DEVICE_INFO_SERIAL_NUMBER, buffer, &size) == DC_ERROR_SUCCESS)
        logger.log_info("    Serial Number: {}", buffer);

    return DC_ERROR_SUCCESS;
}