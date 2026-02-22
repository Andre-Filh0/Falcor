#include "Logger.hpp"
#include "dmvc/dmvc.h"

#include <cstdint>

#define DEVICE_LIST_UPDATE_TIMEOUT 1000 /* millisecond */

// Forward declarations
DcError PrintSystemInfo(DcSystem system, Falcor::GeneralLogger& logger);
DcError PrintInterfaceInfo(DcInterface interface, Falcor::GeneralLogger& logger);
DcError PrintDeviceInfo2(DcDevice device, int64_t device_index, Falcor::GeneralLogger& logger);

int main()
{
    // Create Quill logger
    auto* quill_logger = quill::create_logger("general");
    Falcor::GeneralLogger logger(quill_logger);

    DcSystem system;
    DcInterface interface;
    DcDevice device;
    size_t interface_count = 0;
    size_t device_count = 0;

    RETURN_IF_ERROR(DcSystemCreate(&system));
    PrintSystemInfo(system, logger);

    // Update interface list
    RETURN_IF_ERROR(DcSystemUpdateInterfaceList(system, nullptr, DC_INFINITE));

    RETURN_IF_ERROR(DcSystemGetInterfaceCount(system, &interface_count));
    logger.log_info("Found {} interfaces", static_cast<uint32_t>(interface_count));

    for (uint32_t i = 0; i < interface_count; i++)
    {
        logger.log_info("Interface {}", i);

        RETURN_IF_ERROR(DcSystemGetInterface(system, i, &interface));
        PrintInterfaceInfo(interface, logger);

        // Open interface and update device list
        RETURN_IF_ERROR(DcInterfaceOpen(interface));
        RETURN_IF_ERROR(DcInterfaceUpdateDeviceList(interface, nullptr, DEVICE_LIST_UPDATE_TIMEOUT));

        DcInterfaceGetDeviceCount(interface, &device_count);
        logger.log_info("  Found {} device(s) on the interface", static_cast<uint32_t>(device_count));

        for (uint32_t j = 0; j < device_count; j++)
        {
            logger.log_info("    Device {}", j);

            RETURN_IF_ERROR(DcInterfaceGetDevice(interface, j, &device));
            RETURN_IF_ERROR(PrintDeviceInfo2(device, j, logger));
        }
    }

    RETURN_IF_ERROR(DcSystemDestroy(system));

    logger.log_info("Finished device enumeration");

    return 0;
}

// ============================================================
// Print system info
// ============================================================
DcError PrintSystemInfo(DcSystem system, Falcor::GeneralLogger& logger)
{
    char buffer[256] = {0};
    size_t size;

    size = sizeof(buffer);
    RETURN_IF_ERROR(DcSystemGetInfo(system, DC_SYSTEM_INFO_VENDOR, buffer, &size));
    logger.log_info("System Vendor: {}", buffer);

    size = sizeof(buffer);
    RETURN_IF_ERROR(DcSystemGetInfo(system, DC_SYSTEM_INFO_MODEL, buffer, &size));
    logger.log_info("System Model: {}", buffer);

    size = sizeof(buffer);
    RETURN_IF_ERROR(DcSystemGetInfo(system, DC_SYSTEM_INFO_VERSION, buffer, &size));
    logger.log_info("System Version: {}", buffer);

    return DC_ERROR_SUCCESS;
}

// ============================================================
// Print interface info
// ============================================================
DcError PrintInterfaceInfo(DcInterface interface, Falcor::GeneralLogger& logger)
{
    char buffer[256] = {0};
    size_t size;

    size = sizeof(buffer);
    RETURN_IF_ERROR(DcInterfaceGetInfo(interface, DC_INTERFACE_INFO_ID, buffer, &size));
    logger.log_info("  Interface ID: {}", buffer);

    size = sizeof(buffer);
    RETURN_IF_ERROR(DcInterfaceGetInfo(interface, DC_INTERFACE_INFO_DISPLAY_NAME, buffer, &size));
    logger.log_info("  Interface Display Name: {}", buffer);

    return DC_ERROR_SUCCESS;
}

// ============================================================
// Print device info
// ============================================================
DcError PrintDeviceInfo2(DcDevice device, int64_t device_index, Falcor::GeneralLogger& logger)
{
    char buffer[256] = {0};
    size_t size;
    DcInterface parent;
    DcNodeList nodelist;
    DcNode node;
    DcIntegerNode device_selector;

    size = sizeof(buffer);
    RETURN_IF_ERROR(DcDeviceGetInfo(device, DC_DEVICE_INFO_VENDOR, buffer, &size));
    logger.log_info("    Device Vendor: {}", buffer);

    size = sizeof(buffer);
    RETURN_IF_ERROR(DcDeviceGetInfo(device, DC_DEVICE_INFO_MODEL, buffer, &size));
    logger.log_info("    Device Model: {}", buffer);

    size = sizeof(buffer);
    RETURN_IF_ERROR(DcDeviceGetInfo(device, DC_DEVICE_INFO_SERIAL_NUMBER, buffer, &size));
    logger.log_info("    Serial Number: {}", buffer);

    size = sizeof(buffer);
    RETURN_IF_ERROR(DcDeviceGetInfo(device, DC_DEVICE_INFO_VERSION, buffer, &size));
    logger.log_info("    Version: {}", buffer);

    RETURN_IF_ERROR(DcDeviceGetParent(device, &parent));
    RETURN_IF_ERROR(DcInterfaceGetNodeList(parent, &nodelist));
    RETURN_IF_ERROR(DcNodeListGetNode(nodelist, "DeviceSelector", &node));
    device_selector = DcNodeCastToIntegerNode(node);

    RETURN_IF_ERROR(DcIntegerNodeSetValue(device_selector, device_index));

    size = sizeof(buffer);
    DcNodeListGetValue(nodelist, "GevDeviceMACAddress", buffer, &size);
    logger.log_info("    Device MAC: {}", buffer);

    size = sizeof(buffer);
    DcNodeListGetValue(nodelist, "GevDeviceIPAddress", buffer, &size);
    logger.log_info("    Device IP: {}", buffer);

    return DC_ERROR_SUCCESS;
}