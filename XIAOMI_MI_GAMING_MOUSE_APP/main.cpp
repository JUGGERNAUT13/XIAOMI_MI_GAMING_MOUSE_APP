#include <iostream>
#include <QByteArray>
#include <libusb-1.0/libusb.h>

#define VENDOR_ID               0x2717
#define PRODUCT_ID_WIRE         0x5009
//#define PRODUCT_ID_WIRELESS     0x500b        //NOT WORKING IN WIRELESS MODE, BUT COMMANDS EXEC SUCCESSFULLY
#define PACKET_SIZE             32

typedef enum interface_protocol_type {
    KEYBOARD        = 1,
    MOUSE           = 2
} interface_protocol_type;

typedef enum devices {
    TAIL            = 0,
    WHEEL           = 1
} devices;

typedef enum modes {        //modes 2..6 not allowed for WHEEL
    DISABLE         = 0,
    STATIC          = 1,
    BREATH          = 2,
    TIC_TAC         = 3,
    TEST_1          = 4,    //has no effect
    COLORS_CHANGING = 5,
    RGB             = 6
} modes;

typedef enum speed {
    SPEED_WRONG     = 0,    //the fastest blink for WHEEL, but has no effect for TAIL, maybe a bug(not usable value)
    SPEED_1         = 1,
    SPEED_2         = 2,
    SPEED_3         = 3,
    SPEED_4         = 4,
    SPEED_5         = 5,
    SPEED_6         = 6,
    SPEED_7         = 7,
    SPEED_8         = 8
} speed;

using namespace std;

int write_to_mouse(QByteArray &data) {
    struct libusb_device_handle * device_handle = NULL;
    struct libusb_config_descriptor *cfg_desc = NULL;
    libusb_context *context = NULL;
    int result;
    result = libusb_init(&context);             // Initialize libusb
    if(result < 0) {
//        cout << "Error initializing libusb: " << libusb_error_name(result);
        return -1;
    }
//    libusb_set_option(context, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);      // Set debugging output to max level
    libusb_device **list;
    libusb_device *found = NULL;
    ssize_t cnt = libusb_get_device_list(context, &list);
    libusb_device *device;
    struct libusb_device_descriptor desc;
    for(int i = 0; i < cnt; i++) {
        device = list[i];
        result = libusb_get_device_descriptor(device, &desc);
//        cout << "Vendor:Device = " << hex << desc.idVendor << " " << hex << desc.idProduct << dec << i << endl;
        if((result == LIBUSB_SUCCESS) && (desc.idVendor == VENDOR_ID) && ((desc.idProduct == PRODUCT_ID_WIRE)/* || (desc.idProduct == PRODUCT_ID_WIRELESS)*/)) {
            found = device;
            break;
        }
    }
    if(!found) {
        return -2;
    }
    libusb_open(found, &device_handle);
    if(!device_handle) {
//        cout << "Error finding USB device" << endl;
        libusb_exit(context);
        return -3;
    }
    // Enable auto-detaching of the kernel driver. If a kernel driver currently has an interface claimed, it will be automatically be detached when we claim that interface.
    // When the interface is restored, the kernel driver is allowed to be re-attached. This can alternatively be manually done via libusb_detach_kernel_driver().
//    libusb_set_auto_detach_kernel_driver(device_handle, 1);
    libusb_get_active_config_descriptor(found, &cfg_desc);
    int interface_number = -1;
    for(int i = 0; i < static_cast<int>(cfg_desc->bNumInterfaces); i++) {
        for(int j = 0; j < static_cast<int>(cfg_desc->interface[i].num_altsetting); j++) {
            if(static_cast<int>(cfg_desc->interface[i].altsetting[j].bInterfaceProtocol) == KEYBOARD) {
                interface_number = static_cast<int>(cfg_desc->interface[i].altsetting[j].bInterfaceNumber);
            }
        }
    }
    libusb_detach_kernel_driver(device_handle, interface_number);
//    cout << "Interface number: " << interface_number << endl;
    result = libusb_claim_interface(device_handle, interface_number);
    if(result < 0) {
//        cout << "Error claiming interface: " << libusb_error_name(result) << endl;
        if(device_handle) {
            libusb_close(device_handle);
        }
        libusb_exit(context);
        return -4;
    }
    result = libusb_control_transfer(device_handle, 0x0021, 0x0009, 0x024d, 0x0001, reinterpret_cast<unsigned char *>(data.data()), data.count(), 50);
    if((result == LIBUSB_SUCCESS) || (result == PACKET_SIZE)) {
        result = 0;
//        cout << "Sucess" << endl;
    } else {
        result = -5;
//        cout << "Failure " << libusb_error_name(result) << ";  code: " << result << endl;
    }
    libusb_release_interface(device_handle, interface_number);          // We are done with our device and will now release the interface we previously claimed as well as the device
    libusb_attach_kernel_driver(device_handle, interface_number);
    libusb_close(device_handle);
    libusb_exit(context);                                               // Shutdown libusb
    return result;
}

int mouse_set_color_for_device(devices dev, modes mod, speed spd, uint8_t r, uint8_t g, uint8_t b) {
    QByteArray clr_mod_spd_arr = "\x4d\xa1";
    clr_mod_spd_arr.append(dev);
    clr_mod_spd_arr.append(mod);
    clr_mod_spd_arr.append(spd * static_cast<int>(mod > 1));
    clr_mod_spd_arr.append("\x08\x08");
    clr_mod_spd_arr.append(r);
    clr_mod_spd_arr.append(g);
    clr_mod_spd_arr.append(b);
    clr_mod_spd_arr.append("\x00", (PACKET_SIZE - clr_mod_spd_arr.count()));
    return write_to_mouse(clr_mod_spd_arr);
}

int mouse_non_sleep() {
    QByteArray non_sleep_arr = "\x4d\x90\xde\x30\xfe\xff\xff\xff\xda\x98\x20\x76\xd5\xd1\xae\x68\xa0\xe6\xe9\x03\xbc\xd8\x28\x00\xf3\xe0\x81\x77\xb8\xee\x37\x06";
    return write_to_mouse(non_sleep_arr);
}

int main() {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 255;
    mouse_set_color_for_device(TAIL, STATIC, SPEED_8, r, g, b);
//    mouse_non_sleep();
    return 0;
}
