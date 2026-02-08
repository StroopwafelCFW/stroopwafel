# Stroopwafel IPC Interface

Stroopwafel provides an IPC interface through the `/dev/stroopwafel` device. This interface allows for low-level interaction with the IOS kernel from user-land applications.

## Device Name
`/dev/stroopwafel`

## Commands

### `IOCTLV_WRITE_MEMORY` (0x3)
Writes data to IOS memory.

- **Input Vectors:**
  - `vector[0]`: `ptr` contains the destination **virtual** address in IOS memory. `len` contains the total length of data to be written.
  - `vector[1...num_in-1]`: Contain the source data to be written. The sum of lengths of these vectors must match `vector[0].len`.
- **Notes:** After writing, the implementation ensures cache coherency by flushing the D-cache and invalidating the I-cache.

### `IOCTLV_EXECUTE` (0x4)
Executes a function in IOS.

- **Input Vectors:**
  - `vector[0]`: `ptr` contains the **virtual** address of the function to be called.
  - `vector[1]`: (Optional) Config buffer passed to the function.
- **Output Vectors:**
  - `vector[0]`: (Optional) Output buffer passed to the function.
- **Function Signature:** `void (*func)(void *config, void *output)`
- **Notes:** The function is called with the config buffer and output buffer as arguments.

### `IOCTL_MAP_MEMORY` (0x5)
Maps memory pages in IOS.

- **Input Buffer:** `ios_map_shared_info_t` structure.

### `IOCTL_GET_MINUTE_PATH` (0x6)
Retrieves the path to the loaded minute image.

- **Output Buffer:** `minute_path_t` structure.

### `IOCTL_GET_PLUGIN_PATH` (0x7)
Retrieves the path to the loaded plugin.

- **Output Buffer:** `minute_path_t` structure.

## Structures and Enums

### `ios_map_shared_info_t`
```c
typedef struct {
    uint32_t paddr;
    uint32_t vaddr;
    uint32_t size;
    uint32_t domain;
    uint32_t type;
    uint32_t cached;
} ios_map_shared_info_t;
```

### `minute_path_t`
```c
typedef struct {
    uint32_t device;
    char path[256];
} minute_path_t;
```

### `minute_device_t`
```c
typedef enum {
    MINUTE_DEVICE_UNKNOWN = 0,
    MINUTE_DEVICE_SLC     = 1,
    MINUTE_DEVICE_SD      = 2,
} minute_device_t;
```
