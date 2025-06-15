#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h> // Added for uint8_t
#include <stdarg.h> // Added for va_list
#include <stdbool.h> // Added for bool type
#include <unistd.h> // Added for usleep
#include <wafel/dynamic.h>
#include <wafel/utils.h>
#include <wafel/ios/svc.h> // Added for MLC dumper
#include <wafel/services/fsa.h> // Added for FSA
#include <wafel/ios/memory.h> // Added for iosAlloc, iosFree
#include <wafel/trampoline.h> // Added for trampoline_t_blreplace

// Constants for MLC dump
#define MLC_DUMP_PATH "/vol/sdcard/mlc_dump/"
#define MLC_DUMP_PART_PREFIX "mlc.part"
#define MLC_DUMP_PART_SIZE_BYTES (1 * 1024 * 1024 * 1024ULL) // 1GB
#define MLC_DUMP_CHUNK_SIZE_BYTES (1 * 1024 * 1024) // 1MB

// Global FSA client handle
static int fsaHandle = -1;

// Copied from led.h for SetNotificationLED
typedef enum NotifLedType {
    NOTIF_LED_OFF = 0,
    NOTIF_LED_BLUE_SOLID = 1,
    NOTIF_LED_RED_SOLID = 2,
    NOTIF_LED_ORANGE_SOLID = 3, // Orange is often used for Yellow
    NOTIF_LED_BLUE_BLINKING = 5,
    NOTIF_LED_RED_BLINKING = 6,
    NOTIF_LED_ORANGE_BLINKING = 7,
    NOTIF_LED_RED = 0xD, // Alias or specific direct control
    NOTIF_LED_ORANGE = 0xE, // Alias or specific direct control
} NotifLedType;

// Forward declaration for SetNotificationLED
// (Ideally this would be in a proper header)
extern void SetNotificationLED(uint8_t mask);

// --- Placeholder Raw IOS Device Functions ---
// These need to be replaced with actual IOS calls for raw device access.
// Signatures and behavior are assumed for now.
typedef int RawDeviceHandle; // Placeholder type for raw device handle
#define INVALID_RAW_DEVICE_HANDLE (-1)

static RawDeviceHandle IOS_RawOpen(const char* deviceName, int mode) {
    debug_printf("IOS_RawOpen (Placeholder) called for %s\n", deviceName);
    // Actual implementation needed. For now, return a dummy handle or error.
    // To allow some testing of the structure, let's simulate failure for now.
    return INVALID_RAW_DEVICE_HANDLE; // Simulate failure to force error paths
}

static int IOS_RawRead(RawDeviceHandle handle, void* buffer, size_t size) {
    debug_printf("IOS_RawRead (Placeholder) called for handle %d, size %zu\n", handle, size);
    // Actual implementation needed.
    return -1; // Simulate read error
}

static long long IOS_RawGetSize(RawDeviceHandle handle) {
    debug_printf("IOS_RawGetSize (Placeholder) called for handle %d\n", handle);
    // Actual implementation needed.
    return -1; // Simulate failure to get size
}

static int IOS_RawClose(RawDeviceHandle handle) {
    debug_printf("IOS_RawClose (Placeholder) called for handle %d\n", handle);
    // Actual implementation needed.
    return 0; // Simulate success
}
// --- End Placeholder Raw IOS Device Functions ---

// --- Placeholder IOS_Shutdown ---
// Placeholder for IOS_Shutdown function (usually from <wafel/ios/svc.h> or similar)
// If not found in includes, this signature is assumed.
// int IOS_Shutdown(bool restart);
// For now, to ensure it compiles, let's provide a dummy version if not found by includes.
// Actual SVC call would be needed if a library function isn't available.
#ifndef IOS_Shutdown
// This is a common pattern, but the actual function might be different or require direct SVC call
// For the purpose of this subtask, we'll define a dummy if no real one is picked up by includes.
// This dummy will allow compilation and testing of the call flow.
static int IOS_Shutdown(bool restart) {
    debug_printf("IOS_Shutdown (Placeholder) called with restart: %d\n", restart);
    // In a real scenario, this would trigger a power off or restart.
    // For testing, we might just loop here or print.
    // To simulate it not returning (like a real power off):
    // while(1) { usleep(1000000); }
    return 0; // Placeholder return
}
#define IOS_Shutdown_DEFINED_LOCALLY
#endif
// --- End Placeholder IOS_Shutdown ---

// Function to power off the console
static void power_off_console(void) {
    log_to_sd("Attempting console power off...\n");
    IOS_Shutdown(false); // Call with false for power off, not restart.
    // If IOS_Shutdown works, code below might not be reached.
    log_to_sd("Console did not power off (IOS_Shutdown might be a placeholder or failed).\n");
}

// Mount SD card
int mount_sd_card(void) {
    if (fsaHandle < 0) {
        fsaHandle = FSA_Open();
        if (fsaHandle < 0) {
            debug_printf("MLC Dumper: FSA_Open failed with error %d\n", fsaHandle);
            return -1;
        }
        debug_printf("MLC Dumper: FSA_Open succeeded, handle: %d\n", fsaHandle);
    }

    char mountPath[0x100];
    int ret = -1;
    for (int attempts = 0; attempts < 5; attempts++) {
        ret = FSA_Mount(fsaHandle, "/dev/sdcard01", "/vol/sdcard", 0, mountPath, sizeof(mountPath));
        if (ret == 0) {
            debug_printf("MLC Dumper: SD Card mounted successfully to /vol/sdcard\n");
            return 0;
        }
        debug_printf("MLC Dumper: FSA_Mount attempt %d failed with %d\n", attempts + 1, ret);
        usleep(1000 * 1000); // Wait 1 second
    }

    debug_printf("MLC Dumper: Failed to mount SD card after multiple attempts.\n");
    // We don't close FSA handle here, maybe it's needed for other volumes or retries later.
    return -1;
}

// Unmount SD card
void unmount_sd_card(void) {
    if (fsaHandle >= 0) {
        int ret = FSA_Unmount(fsaHandle, "/vol/sdcard", 0); // 0 for no force
        if (ret == 0) {
            debug_printf("MLC Dumper: SD Card /vol/sdcard unmounted successfully.\n");
        } else {
            debug_printf("MLC Dumper: FSA_Unmount /vol/sdcard failed with %d\n", ret);
        }
        // Not closing fsaHandle here, could be used for other operations.
        // FSA_Close(fsaHandle); fsaHandle = -1;
    } else {
        debug_printf("MLC Dumper: unmount_sd_card called but fsaHandle is not valid.\n");
    }
}

// Flush SD card volume
void flush_sd_card(void) {
    if (fsaHandle >= 0) {
        int ret = FSA_FlushVolume(fsaHandle, "/vol/sdcard");
        if (ret == 0) {
            debug_printf("MLC Dumper: Flushed /vol/sdcard successfully.\n");
        } else {
            debug_printf("MLC Dumper: FSA_FlushVolume /vol/sdcard failed with %d\n", ret);
        }
    } else {
        debug_printf("MLC Dumper: flush_sd_card called but fsaHandle is not valid.\n");
    }
}

// Log to SD card
void log_to_sd(const char* format, ...) {
    if (fsaHandle < 0) {
        debug_printf("MLC Dumper: log_to_sd - FSA handle not valid, cannot log to SD.\n");
        // Fallback to debug_printf if SD is not available
        va_list args;
        va_start(args, format);
        debug_vprintf(format, args); // Assuming debug_vprintf exists or use vsnprintf to a buffer then debug_printf
        va_end(args);
        debug_printf("\n"); // Newline if debug_vprintf doesn't add one
        return;
    }

    // Create directory, ignore if already exists
    int ret = FSA_MakeDir(fsaHandle, "/vol/sdcard/mlc_dump", 0);
    if (ret < 0 && ret != FSA_ERROR_ALREADY_EXISTS) { // FSA_ERROR_ALREADY_EXISTS is typically -17 (0xFFFCFFEA) or similar
        debug_printf("MLC Dumper: Failed to create /vol/sdcard/mlc_dump directory, error %d\n", ret);
        // Continue to attempt logging to file, maybe path is wrong or permissions issue
    }

    int logFileHandle = -1;
    // Mode "a" for append. If not supported, this might create new file or fail.
    ret = FSA_OpenFile(fsaHandle, "/vol/sdcard/mlc_dump.log", "a", &logFileHandle);
    if (ret < 0) {
        debug_printf("MLC Dumper: Failed to open/create /vol/sdcard/mlc_dump.log, error %d\n", ret);
        return;
    }

    static char buffer[256]; // Static buffer for log message
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len < 0) {
        debug_printf("MLC Dumper: vsnprintf encoding error for log_to_sd.\n");
        FSA_CloseFile(fsaHandle, logFileHandle);
        return;
    }

    // Ensure newline if not present, though typically logging formats include it.
    // For simplicity, assuming format string includes newline if desired.

    ret = FSA_WriteFile(fsaHandle, buffer, 1, len, logFileHandle, 0);
    if (ret < len) { // FSA_WriteFile returns number of bytes written
        debug_printf("MLC Dumper: Failed to write full message to log file, wrote %d of %d bytes, error %d (if <0)\n", ret < 0 ? 0 : ret, len, ret < 0 ? ret : 0);
        // Error handling might be more complex if ret is an error code vs bytes written
    }

    // Flush the file to ensure data is written to SD card
    // FSA_FlushFile might not be necessary if "a" mode auto-flushes or if system does it periodically
    // but for critical logs, it's safer.
    ret = FSA_FlushFile(fsaHandle, logFileHandle);
    if (ret < 0) {
        debug_printf("MLC Dumper: Failed to flush log file, error %d\n", ret);
    }

    ret = FSA_CloseFile(fsaHandle, logFileHandle);
    if (ret < 0) {
        debug_printf("MLC Dumper: Failed to close log file, error %d\n", ret);
    }
}

// MLC Dumper Plugin: Kernel Main
// This fn runs before everything else in kernel mode.
// It should be used to do extremely early patches
// (ie to BSP and kernel, which launches before MCP)
// It jumps to the real IOS kernel entry on exit.
__attribute__((target("arm")))
void kern_main()
{
    // Placeholder for MLC dumper kernel functionality
    debug_printf("MLC Dumper Plugin: kern_main entered\n");

    // Example: Print a message and a symbol address
    // debug_printf("init_linking symbol at: %08x\n", wafel_find_symbol("init_linking")); // May not be available this early

    // Hook installation moved to kern_main for earlier setup.
    // The target address 0x05027d54 is an EXAMPLE.
    uintptr_t target_hook_address = 0x05027d54;
    debug_printf("MLC Dumper Plugin: Attempting to install hook at 0x%08x in kern_main...\n", target_hook_address);
    trampoline_t_blreplace(target_hook_address, mlc_dump_trigger_hook);
    // Note: debug_printf might not be fully functional if called too early before essential kernel patches.
    // We assume it works here for basic logging. If not, this message might not appear.
    debug_printf("MLC Dumper Plugin: Hook installation attempt from kern_main complete.\n");

    debug_printf("MLC Dumper Plugin: kern_main exiting\n");
}

// Function to dump raw MLC device
static void dump_raw_mlc_device(void) {
    log_to_sd("Starting raw MLC device dump...\n");
    SetNotificationLED(NOTIF_LED_BLUE_BLINKING);

    RawDeviceHandle mlcFileHandle = INVALID_RAW_DEVICE_HANDLE; // Use new type and init
    int sdFileHandle = -1; // Stays as int for FSA SD card access
    char* dump_buffer = NULL;
    // int ret; // 'ret' may not be needed for IOS_RawOpen if it directly returns handle or error code

    // Attempt to open MLC using Raw IOS placeholder
    log_to_sd("Opening /dev/mlc01 via IOS_RawOpen (Placeholder)...\n");
    // Placeholder for actual raw device open. Mode '0' is a placeholder.
    mlcFileHandle = IOS_RawOpen("/dev/mlc01", 0 /* mode placeholder */);
    if (mlcFileHandle == INVALID_RAW_DEVICE_HANDLE) {
        log_to_sd("Failed to open /dev/mlc01 via IOS_RawOpen. Handle: %d\n", mlcFileHandle);
        SetNotificationLED(NOTIF_LED_RED);
        return;
    }
    log_to_sd("/dev/mlc01 opened successfully via IOS_RawOpen. Handle: %d\n", mlcFileHandle);

    // Attempt to get MLC size using Raw IOS placeholder
    log_to_sd("Getting MLC size via IOS_RawGetSize (Placeholder)...\n");
    long long retrieved_size = IOS_RawGetSize(mlcFileHandle);
    unsigned long long mlc_total_size = 0;

    if (retrieved_size < 0) { // Check if IOS_RawGetSize indicates error
        log_to_sd("Failed to get MLC size using IOS_RawGetSize. Error: %lld. Will attempt to dump up to a maximum assumed size or until error.\n", retrieved_size);
        SetNotificationLED(NOTIF_LED_ORANGE); // Warning state
        // mlc_total_size remains 0, indicating unknown size
    } else {
        mlc_total_size = (unsigned long long)retrieved_size;
        log_to_sd("MLC size from IOS_RawGetSize: %llu bytes (%llu MB)\n", mlc_total_size, mlc_total_size / (1024*1024));
        if (mlc_total_size == 0) {
            log_to_sd("Warning: MLC size reported as 0 by IOS_RawGetSize. Will attempt to dump up to a maximum assumed size or until error.\n");
            SetNotificationLED(NOTIF_LED_ORANGE); // Warning state
        }
    }

    // Allocate dump buffer
    log_to_sd("Allocating %lluMB dump buffer...\n", MLC_DUMP_CHUNK_SIZE_BYTES / (1024*1024ULL));
    dump_buffer = (char*)iosAlloc(0xCAFE, MLC_DUMP_CHUNK_SIZE_BYTES); // Using placeholder IOS heap ID
    if (!dump_buffer) {
        log_to_sd("Failed to allocate dump buffer.\n");
        SetNotificationLED(NOTIF_LED_RED);
        if (mlcFileHandle != INVALID_RAW_DEVICE_HANDLE) { // Check against new invalid handle
            IOS_RawClose(mlcFileHandle); // Use new close function
        }
        return;
    }
    log_to_sd("Dump buffer allocated at %p.\n", dump_buffer);

    // Initialize loop variables
    unsigned long long total_bytes_read = 0;
    unsigned long long current_part_bytes_written = 0;
    int part_number = 0;
    // sdFileHandle is already declared above, initialize to -1
    sdFileHandle = -1;
    char current_sd_path[128];
    bool dump_error_occurred = false;

    log_to_sd("Starting main dump loop...\n");

    // Main dumping loop
    for (;;) { // or while(true)
        // Part File Management
        if (sdFileHandle < 0) {
            sprintf(current_sd_path, "%s%s%02d.bin", MLC_DUMP_PATH, MLC_DUMP_PART_PREFIX, part_number); // Added .bin extension
            log_to_sd("Opening SD part file: %s\n", current_sd_path);
            ret = FSA_OpenFile(fsaHandle, current_sd_path, "wb", &sdFileHandle);
            if (ret < 0 || sdFileHandle < 0) {
                log_to_sd("Failed to open SD part file %s. Error: %d, Handle: %d\n", current_sd_path, ret, sdFileHandle);
                SetNotificationLED(NOTIF_LED_RED);
                dump_error_occurred = true;
                break;
            }
            log_to_sd("SD part file %s opened successfully. Handle: %d\n", current_sd_path, sdFileHandle);
            current_part_bytes_written = 0;
        }

        // Read from MLC using Raw IOS placeholder
        // log_to_sd("Reading %llu bytes from MLC via IOS_RawRead...\n", MLC_DUMP_CHUNK_SIZE_BYTES); // Too verbose
        int bytes_actually_read = IOS_RawRead(mlcFileHandle, dump_buffer, MLC_DUMP_CHUNK_SIZE_BYTES);

        // Handle Read Results (logic remains similar)
        if (bytes_actually_read < 0) { // Read error
            log_to_sd("Error reading from MLC. Error: %d. Approx offset: 0x%llx (%llu MB)\n", bytes_actually_read, total_bytes_read, total_bytes_read / (1024*1024));
            SetNotificationLED(NOTIF_LED_ORANGE);
            dump_error_occurred = true;
            // For a raw dump, typically stop on error.
            break;
        }
        if (bytes_actually_read == 0) { // End of File
            log_to_sd("End of MLC device reached (FSA_ReadFile returned 0 bytes).\n");
            break;
        }

        // Write to SD Part File
        if (bytes_actually_read > 0) {
            // log_to_sd("Writing %d bytes to %s...\n", bytes_actually_read, current_sd_path); // Too verbose
            int bytes_written = FSA_WriteFile(fsaHandle, dump_buffer, 1, bytes_actually_read, sdFileHandle, 0);
            if (bytes_written < bytes_actually_read) {
                log_to_sd("Error writing to SD part file %s. Wrote %d of %d bytes. Error: %d (if <0)\n", current_sd_path, bytes_written < 0 ? 0 : bytes_written, bytes_actually_read, bytes_written < 0 ? bytes_written : 0);
                SetNotificationLED(NOTIF_LED_RED);
                dump_error_occurred = true;
                break;
            }
            total_bytes_read += bytes_written;
            current_part_bytes_written += bytes_written;
        }

        // Progress Logging & LED Blinking
        if (total_bytes_read > 0 && (total_bytes_read % (MLC_DUMP_CHUNK_SIZE_BYTES * 10)) == 0) { // Log every 10 chunks (10MB)
            log_to_sd("Dump progress: %llu MB written to %s\n", total_bytes_read / (1024*1024), current_sd_path);
        }

        // The LED was set to LED_STATUS_BLUE_BLINKING at the start of the function.
        // If NOTIF_LED_BLUE_BLINKING handles hardware blinking, no further action is needed here in the loop
        // to maintain the blinking state. It will continue blinking until set_mlc_dump_led_status
        // is called again with a different state (e.g., on error or completion).

        // Part File Size Check / Rollover
        if (current_part_bytes_written >= MLC_DUMP_PART_SIZE_BYTES) {
            log_to_sd("Closing current part file %s (%llu bytes written).\n", current_sd_path, current_part_bytes_written);
            FSA_FlushFile(fsaHandle, sdFileHandle); // Flush before closing
            FSA_CloseFile(fsaHandle, sdFileHandle);
            sdFileHandle = -1;
            part_number++;
            log_to_sd("Rollover to part %d.\n", part_number);
        }

        // MLC Total Size Check (if mlc_total_size is known and > 0)
        if (mlc_total_size > 0 && total_bytes_read >= mlc_total_size) {
            log_to_sd("All expected MLC data read (%llu bytes of %llu).\n", total_bytes_read, mlc_total_size);
            break;
        }
    } // End of main dumping loop

    // After the loop cleanup and final status
    log_to_sd("Main dump loop finished. Cleaning up...\n");

    if (sdFileHandle >= 0) {
        log_to_sd("Closing final SD part file: %s (%llu bytes written)\n", current_sd_path, current_part_bytes_written);
        FSA_FlushFile(fsaHandle, sdFileHandle);
        FSA_CloseFile(fsaHandle, sdFileHandle);
        sdFileHandle = -1;
    }

    if (dump_buffer) {
        iosFree(0xCAFE, dump_buffer);
        log_to_sd("Dump buffer freed.\n");
    }
    if (mlcFileHandle != INVALID_RAW_DEVICE_HANDLE) { // Check against new invalid handle
        IOS_RawClose(mlcFileHandle); // Use new close function
        log_to_sd("/dev/mlc01 closed via IOS_RawClose.\n");
    }

    if (dump_error_occurred) {
        log_to_sd("MLC dump completed with errors. Total bytes read: %llu (%llu MB)\n", total_bytes_read, total_bytes_read / (1024*1024));
        // LED will already be NOTIF_LED_ORANGE or NOTIF_LED_RED, as set in the loop.
    } else {
        log_to_sd("MLC dump completed successfully. Total bytes read: %llu (%llu MB)\n", total_bytes_read, total_bytes_read / (1024*1024));
        SetNotificationLED(NOTIF_LED_OFF); // Or a success LED like solid green if available/desired
    }
}

// Hook function to trigger MLC dump
static void mlc_dump_trigger_hook(void) {
    // It's crucial that log_to_sd and other functions here are safe to call from a hook context.
    // If they rely on resources not available or safe in this context, this will crash.
    // Assuming for now that log_to_sd, set_mlc_dump_led_status, dump_raw_mlc_device,
    // flush_sd_card, and unmount_sd_card are safe.

    log_to_sd("MLC dump hook triggered!\n");

    // Re-check fsaHandle just in case, though it should be open if mcp_main succeeded.
    if (fsaHandle < 0) {
        log_to_sd("FSA handle not valid in hook! Attempting to open/mount SD again.\n");
        if (mount_sd_card() != 0) { // Try to mount again
             log_to_sd("Failed to mount SD card from hook. Aborting dump.\n");
             SetNotificationLED(NOTIF_LED_RED);
             return; // Cannot proceed
        }
    }

    SetNotificationLED(NOTIF_LED_BLUE_BLINKING); // Set initial LED for dump

    dump_raw_mlc_device(); // This function handles its own LED status updates during and after dump

    log_to_sd("Flushing SD card volumes post-dump...\n");
    flush_sd_card();

    // Unmounting SD is important to ensure data integrity before potential power off
    log_to_sd("Unmounting SD card post-dump...\n");
    unmount_sd_card();

    log_to_sd("Attempting to power off console...\n");
    power_off_console();

    // LED state is assumed to be correctly set by dump_raw_mlc_device() or earlier errors in this hook.
    // If power_off_console() is not called or fails, the current LED state will persist.
    // The message from power_off_console itself will indicate if it thinks it returned.
    log_to_sd("Hook function finished. Current LED state reflects dump outcome. Console power-off attempted.\n");
}


// MLC Dumper Plugin: MCP Main
// This fn runs before MCP's main thread, and can be used
// to perform late patches and spawn threads under MCP.
// It must return.
void mcp_main()
{
    // Set initial LED status
    SetNotificationLED(NOTIF_LED_OFF);

    debug_printf("MLC Dumper Plugin: mcp_main entered. Attempting to mount SD card...\n");

    if (mount_sd_card() != 0) {
        debug_printf("MLC Dumper: SD Card mount failed! SD logging/dumping unavailable.\n");
        SetNotificationLED(NOTIF_LED_RED); // Critical failure for core functionality
        // SD card is essential. If it fails, mcp_main's primary role (setup for SD logging for the hook) failed.
    } else {
        log_to_sd("MLC Dumper Plugin: mcp_main started. SD logging initialized.\n");
        // Hook installation is now done in kern_main.
        log_to_sd("MLC Dumper Plugin: mcp_main tasks complete. Hook was installed in kern_main.\n");
    }

    // The direct call to dump_raw_mlc_device(); was removed. Dumping is now triggered by the hook.

    debug_printf("MLC Dumper Plugin: mcp_main exiting. Check SD card for mlc_dump.log for status.\n");

    // Optionally unmount or flush here if NOT planning to use from hook,
    // but since the hook will use FSA, we leave fsaHandle open.
    // flush_sd_card();
    // unmount_sd_card();
    // If FSA_Close is to be called, it should be when all FSA ops are done.
    // if (fsaHandle >= 0) {
    //     FSA_Close(fsaHandle);
    //     fsaHandle = -1;
    // }
}
