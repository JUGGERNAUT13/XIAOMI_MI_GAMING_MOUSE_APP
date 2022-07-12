#include <iostream>
#include <QString>
#include <libusb-1.0/libusb.h>

#define VENDOR_ID       0x2717
#define PRODUCT_ID      0x5009
#define PACKET_SIZE     32

typedef enum devices {
    TAIL    = 0,
    WHEEL   = 1
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
    SPEED_WRONG = 0,        //the fastest blink for WHEEL, but has no effect for TAIL, maybe a bug(not usable value)
    SPEED_1     = 1,
    SPEED_2     = 2,
    SPEED_3     = 3,
    SPEED_4     = 4,
    SPEED_5     = 5,
    SPEED_6     = 6,
    SPEED_7     = 7,
    SPEED_8     = 8
} speed;

using namespace std;

int main() {
    struct libusb_device_handle * device_handle = NULL;
    struct libusb_config_descriptor *cfg_desc = NULL;
    libusb_context *context = NULL;
    int result;
    result = libusb_init(&context);             // Initialize libusb
    if(result < 0) {
        cout << "Error initializing libusb: " << libusb_error_name(result);
        exit(-1);
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
        if((result == LIBUSB_SUCCESS) && (desc.idVendor == VENDOR_ID) && (desc.idProduct == PRODUCT_ID)) {
            found = device;
            break;
        }
    }
    if(!found) {
        return 0;
    }
    libusb_open(found, &device_handle);
    if(!device_handle) {
        cout << "Error finding USB device" << endl;
        libusb_exit(context);
        exit(-2);
    }
    // Enable auto-detaching of the kernel driver.
    // If a kernel driver currently has an interface claimed, it will be automatically be detached
    // when we claim that interface. When the interface is restored, the kernel driver is allowed
    // to be re-attached. This can alternatively be manually done via libusb_detach_kernel_driver().
    libusb_set_auto_detach_kernel_driver(device_handle, 1);
    libusb_get_active_config_descriptor(found, &cfg_desc);
    int interface_number = cfg_desc->interface[0].altsetting[0].bInterfaceNumber;
    cout << "Interface number: " << interface_number << endl;
    result = libusb_claim_interface(device_handle, /*interface_number*/ 1);          //need solve the problem, why after 'interface_number' not working properly
    if(result < 0) {
        cout << "Error claiming interface: " << libusb_error_name(result) << endl;
        if(device_handle) {
            libusb_close(device_handle);
        }
        libusb_exit(context);
        exit(-3);
    }
    int curr_mode = STATIC;
    int curr_dev = TAIL;
    int curr_speed = SPEED_8 * static_cast<int>(curr_mode > 1);
    QByteArray arr = "\x4d\xa1";
    arr.append(curr_dev);
    arr.append(curr_mode);
    arr.append(curr_speed);
    arr.append("\x08\x08");
    int r = 0;
    int g = 0;
    int b = 255;
    arr.append(r);
    arr.append(g);
    arr.append(b);
    arr.append("\x00", (PACKET_SIZE - arr.count()));
    result = libusb_control_transfer(device_handle, 0x0021, 0x0009, 0x024d, 0x0001, reinterpret_cast<unsigned char *>(arr.data()), arr.count(), 100);
    if((result == LIBUSB_SUCCESS) || (result == PACKET_SIZE)) {
        cout << "Sucess" << endl;
    } else {
        cout << "Failure " << libusb_error_name(result) << endl;
    }
    // We are done with our device and will now release the interface we previously claimed as well as the device
    libusb_release_interface(device_handle, interface_number);
    libusb_close(device_handle);
    // Shutdown libusb
    libusb_exit(context);
    return 0;
}
