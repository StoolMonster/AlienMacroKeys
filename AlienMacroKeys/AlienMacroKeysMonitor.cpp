#include <wtypes.h>
#include <strsafe.h>
#include "hid.h"
#include "AlienMacroKeys.h"

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")

DWORD StartMonitor(WORD targetVID, WORD targetPID)
{
    static HID_DEVICE               targetDevice;
    HANDLE                          completionEvent;
    bool                            readResult;
    DWORD                           waitStatus;
    OVERLAPPED                      overlap;
    DWORD                           bytesTransferred;
    PCHAR                           targetDevicePath = nullptr;
    PHID_DEVICE                     pDevice = nullptr;
    ULONG                           numberDevices;

    if (!FindKnownHidDevices(&pDevice, &numberDevices))
    {
        std::cerr << "No HID devices found." << std::endl;
        return -1;
    }

    for (ULONG iIndex = 0; iIndex < numberDevices; iIndex++, pDevice++)
    {
        // If currently pointed to device is our target device, copy the DevicePath into a buffer to use later
        if (pDevice->Attributes.VendorID == targetVID &&
            pDevice->Attributes.ProductID == targetPID &&
            pDevice->Caps.UsagePage == AW_USAGEPAGE &&
            pDevice->Caps.Usage == AW_USAGE)
        {
            int iDevicePathSize = static_cast<int>(strnlen(pDevice->DevicePath, MAX_PATH) + 1);
            // Try to allocate memory for storing the Device Path
            try
            {
                targetDevicePath = new char[iDevicePathSize];
                std::memset(targetDevicePath, 0, iDevicePathSize);
            }
            catch (const std::bad_alloc&)
            {
                std::cerr << "Unable to allocate memory for device path." << std::endl;
                return -1;
            }
            StringCbCopyA(targetDevicePath, iDevicePathSize, pDevice->DevicePath);
            pDevice -= iIndex;          // Move pDevice pointer back to the beginning of the list again in preparation for the free statement
            break;
        }
    }
    delete[] pDevice;
    pDevice = nullptr;

    if (targetDevicePath == nullptr)
    {
        std::cerr << "Target device could not be located!" << std::endl;
        return -1;
    }

#ifdef _DEBUG
    std::cout << "Target Device located: " << targetDevicePath << std::endl;
#endif

    // Open target device for asynchronous reading
    bool openForAsync = OpenHidDevice(targetDevicePath, true, false, true, false, &targetDevice);

    delete[] targetDevicePath;

    if (!openForAsync)
    {
        std::cerr << "Unable to open target HID device for async read" << std::endl;
        return -1;
    }

    std::cout << "Starting monitor" << std::endl;

    completionEvent = CreateEvent(nullptr, false, false, nullptr);

    if (completionEvent == nullptr)
    {
        return -1;
    }

    readResult = true;

    // Begin monitoring loop. This likely could be made more efficient. Is is mostly a copy of Microsoft's hclient sample
    do
    {
        readResult = ReadOverlapped(&targetDevice, completionEvent, &overlap);

        if (!readResult)
        {
            break;
        }
        while (true)
        {
            waitStatus = WaitForSingleObject(completionEvent, READ_THREAD_TIMEOUT);

            if (waitStatus == WAIT_OBJECT_0)
            {
                readResult = GetOverlappedResult(targetDevice.HidDevice, &overlap, &bytesTransferred, true);
                break;
            }
        }
        UnpackReport(targetDevice.InputReportBuffer,
            targetDevice.Caps.InputReportByteLength,
            HidP_Input,
            targetDevice.InputData,
            targetDevice.InputDataLength,
            targetDevice.Ppd);

        USAGE usage = *targetDevice.InputData->ButtonData.Usages;

        //std::cout << "usage: " << std::hex << usage << std::endl;

        if (usage >= MACROA && usage <= MACROE)
        {
            HandleMacroKey(usage);
        }
    } while (readResult);

    return 0;
}

void HandleMacroKey(USAGE macroKey)
{
    std::cout << "Read key: 0x" << std::hex << macroKey << " Macro " << (char)(macroKey - 0x10) << std::endl;

    INPUT inputs[2] = {};
    ZeroMemory(inputs, sizeof(inputs));

    // Map macroKey (MACROA..MACROE) to F13..F17 (VK_F13 = 0x7C)
    // Assuming MACROA = 0x51, MACROB = 0x52, ..., MACROE = 0x55
    // F13 = 0x7C, so offset is 0x7C - 0x51 = 0x2B
    // 0x7C is the virtual-key code (VK_F13) for the F13 key in the windows API, not the hardware scancode.
    // 00_64 is the hardware scancode for F13, which is what the HID report uses.

    WORD vkCode = 0x7C + (macroKey - MACROA);

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vkCode;

    std::cout << "Map to: " << std::hex << inputs[0].ki.wVk << std::endl;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = vkCode;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}