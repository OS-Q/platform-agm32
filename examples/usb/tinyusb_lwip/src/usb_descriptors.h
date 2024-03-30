#ifndef _TUSB_DESCRIPTORS_H_
#define _TUSB_DESCRIPTORS_H_

extern uint8_t const * tud_descriptor_device_cb(void);
extern uint8_t const * tud_descriptor_configuration_cb(uint8_t index);
extern uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid);

#endif /* _TUSB_DESCRIPTORS_H_ */
