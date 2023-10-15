#ifndef _DEVICE_UTILS_H
#define _DEVICE_UTILS_H

#define DEFINE_DEVICE_INIT(device) \
void device##_init ()

#define DECLARE_DEVICE_INIT(device) \
extern void device##_init ()

#define DEVICE_HANDLER_NAME(device) __intr_##device##_handler

#define DEFINE_DEVICE_HANDLER(device) \
void DEVICE_HANDLER_NAME(device) ()

#define DECLARE_DEVICE_HANDLER(device) \
extern void DEVICE_HANDLER_NAME(device) ()

#endif /* _DEVICE_UTILS_H */
