// ============================================================================
// camera_manager.cpp — Runtime camera backend dispatcher
// ============================================================================

#include "camera_manager.h"
#include "dji_camera.h"
#include "gopro_camera.h"
#include <NimBLEDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <cstring>

// ──────────────────────────────────────────────────────────────────────────────
// Security & Concurrency Globals
// ──────────────────────────────────────────────────────────────────────────────
// NOTE: the firmware is single-threaded from loop()'s point of view — BLE
// scan callbacks only ever write a tiny pending slot that loop() drains.
// The two mutexes below are kept for the rare cross-task disconnect/scan
// path, but they use short timeouts (never portMAX_DELAY) so a wedged BLE
// host task can never deadlock loop().
// ──────────────────────────────────────────────────────────────────────────────

static SemaphoreHandle_t g_stateMutex = NULL;
static SemaphoreHandle_t g_scanMutex = NULL;

// Short lock timeout so loop() never blocks forever on the BLE host task.
static const uint32_t kCamMutexTimeoutMs = 50;

// Helper to safely sanitize device names (prevent XSS injection via BLE ads)
void sanitizeDeviceName(char* dest, const char* src, size_t maxSize) {
    if (!dest || !src || maxSize == 0) return;
    
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j < maxSize - 1; i++) {
        char c = src[i];
        // Allow only alphanumeric, space, dash, underscore, dot
        if ((c >= 'a' && c <= 'z') || 
            (c >= 'A' && c <= 'Z') || 
            (c >= '0' && c <= '9') || 
            c == ' ' || c == '-' || c == '_' || c == '.') {
            dest[j++] = c;
        } else {
            // Replace unsafe chars with '?'
            dest[j++] = '?';
        }
    }
    dest[j] = '\0';
}

// Thread-safe initialization of mutexes
static void initMutexes() {
    if (g_stateMutex == NULL) {
        g_stateMutex = xSemaphoreCreateMutex();
        configASSERT(g_stateMutex);
    }
    if (g_scanMutex == NULL) {
        g_scanMutex = xSemaphoreCreateMutex();
        configASSERT(g_scanMutex);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Internal state
// ──────────────────────────────────────────────────────────────────────────────

static bool _stackReady = false;

static void shutdownActiveBackend() {
    // Stop any scan and drop the current BLE connection before switching.
    NimBLEScan *pScan = NimBLEDevice::getScan();
    if (pScan && pScan->isScanning()) {
        pScan->stop();
    }
    std::list<NimBLEClient *> *clients = NimBLEDevice::getClientList();
    if (clients) {
        for (NimBLEClient *c : *clients) {
            if (c->isConnected()) c->disconnect();
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────────────────────

void camInit() {
    if (_stackReady) return;

    // Initialize thread-safety primitives first
    initMutexes();

    DBG("CAM: Initialising NimBLE stack...");
    NimBLEDevice::init("ESP32-ShutterLink");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    // Bonding enabled (needed for GoPro LE pairing); Just Works IO caps.
    NimBLEDevice::setSecurityAuth(true, false, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

    _stackReady = true;

    if (settingsGet().camera == CAMERA_GOPRO) {
        gpInit();
        DBG("CAM: Active backend — GoPro");
    } else {
        djiInit();
        DBG("CAM: Active backend — DJI Osmo");
    }
}

void camUpdate() {
    if (!_stackReady) return;
    if (settingsGet().camera == CAMERA_GOPRO) gpUpdate(); else djiUpdate();
}

bool camSendStartRecord() {
    if (!_stackReady) return false;
    return (settingsGet().camera == CAMERA_GOPRO) ? gpSendStartRecord()
                                                  : djiSendStartRecord();
}

bool camSendStopRecord() {
    if (!_stackReady) return false;
    return (settingsGet().camera == CAMERA_GOPRO) ? gpSendStopRecord()
                                                  : djiSendStopRecord();
}

BleConnectionState camGetState() {
    if (!_stackReady) return BLE_DISCONNECTED;
    return (settingsGet().camera == CAMERA_GOPRO) ? gpGetState() : djiGetState();
}

const CameraTelemetry& camGetTelemetry() {
    static CameraTelemetry empty;
    if (!_stackReady) return empty;
    return (settingsGet().camera == CAMERA_GOPRO) ? gpGetTelemetry()
                                                  : djiGetTelemetry();
}

bool camIsReady() {
    if (!_stackReady) return false;
    return (settingsGet().camera == CAMERA_GOPRO) ? gpIsReady() : djiIsReady();
}

const char* camGetName() {
    return cameraTypeName(settingsGet().camera);
}

void camSetCamera(CameraType type) {
    if (_stackReady && type != settingsGet().camera) {
        DBG("CAM: Switching backend to %s", cameraTypeName(type));
        shutdownActiveBackend();

        settingsGet().camera = type;
        settingsSave();

        if (type == CAMERA_GOPRO) gpInit(); else djiInit();
    }
}

void camKick() {
    if (!_stackReady) return;

    // Find the active saved camera (if any).
    ShutterSettings &s = settingsGet();
    int8_t active = -1;
    for (uint8_t i = 0; i < s.camCount; i++) {
        if (s.cams[i].active) { active = (int8_t)i; break; }
    }
    if (active < 0) {
        DBG("CAM: kick requested but no active saved camera");
        return;
    }

    SavedCamera &c = s.cams[active];
    if ((CameraType)c.type != s.camera) {
        camSetCamera((CameraType)c.type);   // Also persists + re-inits backend
    }

    DBG("CAM: kicking connection to %s (%s)", c.mac,
        cameraTypeName((CameraType)c.type));
    if ((CameraType)c.type == CAMERA_GOPRO) gpTargetMac(c.mac);
    else                                    djiTargetMac(c.mac);
}

// Disconnect current camera and stop BLE operations (for UI disconnect)
void camDisconnect() {
    // Short-timeout lock: never stall loop() on the BLE host task.
    bool locked = (g_stateMutex != NULL &&
                   xSemaphoreTake(g_stateMutex,
                                  pdMS_TO_TICKS(kCamMutexTimeoutMs)) == pdTRUE);
    
    shutdownActiveBackend();
    
    // Clear active camera flag in settings
    ShutterSettings &s = settingsGet();
    for (uint8_t i = 0; i < s.camCount; i++) {
        if (s.cams[i].active) {
            s.cams[i].active = false;
            break;
        }
    }
    settingsSave();
    
    DBG("CAM: disconnected active camera");
    
    if (locked) {
        xSemaphoreGive(g_stateMutex);
    }
}

// User-initiated one-shot discovery scan.  Called from the /api/camera
// {scan:true} endpoint — the ONLY code path that may start a discovery
// scan.  djiUpdate()/gpUpdate() will NOT auto-restart the scan after the
// 5 s window closes (acceptance criterion D).
void camStartUserScan() {
    if (!_stackReady) return;
    
    // Short-timeout lock: skip (don't queue) if the BLE task holds it.
    bool locked = (g_scanMutex != NULL &&
                   xSemaphoreTake(g_scanMutex,
                                  pdMS_TO_TICKS(kCamMutexTimeoutMs)) == pdTRUE);
    
    CameraType t = settingsGet().camera;
    DBG("CAM: user-initiated scan (backend=%s)", cameraTypeName(t));

    if (t == CAMERA_GOPRO) gpStartScan();
    else                    djiStartScan();
    
    if (locked) {
        xSemaphoreGive(g_scanMutex);
    }
}
