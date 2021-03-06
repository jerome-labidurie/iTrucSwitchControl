/** iOS Switch Control
 * BLE HID Keyboard
 */

#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#include "HIDKeyboardTypes.h"
#include <string>

#define SW_PIN 19
#define KEY "J"


static BLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;
volatile uint8_t connected = 0;

/*
 * This callback is connect with output report. In keyboard output report report special keys changes, like CAPSLOCK, NUMLOCK
 * We can add digital pins with LED to show status
 * bit 1 - NUM LOCK
 * bit 2 - CAPS LOCK
 * bit 3 - SCROLL LOCK
 */
class MyOutputCallbacks : public BLECharacteristicCallbacks {
	void onWrite(BLECharacteristic* me){
		uint8_t* value = (uint8_t*)(me->getValue().c_str());
		Serial.printf ("special keys: %d", *value);
	}
};

class MyCallbacks : public BLEServerCallbacks {
	void onConnect(BLEServer* pServer){
		Serial.println ("connected");
		connected = 1;
	}

	void onDisconnect(BLEServer* pServer){
		Serial.println ("disconnected");
		connected = 0;
	}
};

void setup() {
	Serial.begin(115200);
	Serial.println("iOS Switch Control");

	/* init hardware for button */
	pinMode (SW_PIN, INPUT_PULLUP);

	BLEDevice::init("iOS_SC");
	BLEServer *pServer = BLEDevice::createServer();
	pServer->setCallbacks(new MyCallbacks());

	hid = new BLEHIDDevice(pServer);
	input = hid->inputReport(1); // <-- input REPORTID from report map
	output = hid->outputReport(1); // <-- output REPORTID from report map

	/**
	 * Set manufacturer name (OPTIONAL)
	 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.manufacturer_name_string.xml
	 */
	std::string name = "Fablab-Lannion";
	hid->manufacturer()->setValue(name);

	/*
	 * Set pnp parameters (MANDATORY)
	 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.pnp_id.xml
	 */

	hid->pnp(0x02, 0xe502, 0xa111, 0x0210);

	/*
	 * Set hid informations (MANDATORY)
	 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.hid_information.xml
	 */
	hid->hidInfo(0x00,0x01);

	/*
	 * Keyboard
	 */
	const uint8_t reportMap[] = {
		USAGE_PAGE(1),      0x01,       // Generic Desktop Ctrls
		USAGE(1),           0x06,       // Keyboard
		COLLECTION(1),      0x01,       // Application
		REPORT_ID(1),       0x01,       // REPORTID
		USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
		USAGE_MINIMUM(1),   0xE0,
		USAGE_MAXIMUM(1),   0xE7,
		LOGICAL_MINIMUM(1), 0x00,
		LOGICAL_MAXIMUM(1), 0x01,
		REPORT_SIZE(1),     0x01,       //   1 byte (Modifier)
		REPORT_COUNT(1),    0x08,
		HIDINPUT(1),        0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position
		REPORT_COUNT(1),    0x01,       //   1 byte (Reserved)
		REPORT_SIZE(1),     0x08,
		HIDINPUT(1),        0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
		REPORT_COUNT(1),    0x05,       //   5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
		REPORT_SIZE(1),     0x01,
		USAGE_PAGE(1),      0x08,       //   LEDs
		USAGE_MINIMUM(1),   0x01,       //   Num Lock
		USAGE_MAXIMUM(1),   0x05,       //   Kana
		HIDOUTPUT(1),       0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
		REPORT_COUNT(1),    0x01,       //   3 bits (Padding)
		REPORT_SIZE(1),     0x03,
		HIDOUTPUT(1),       0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
		REPORT_COUNT(1),    0x06,       //   6 bytes (Keys)
		REPORT_SIZE(1),     0x08,
		LOGICAL_MINIMUM(1), 0x00,
		LOGICAL_MAXIMUM(1), 0x65,       //   101 keys
		USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
		USAGE_MINIMUM(1),   0x00,
		USAGE_MAXIMUM(1),   0x65,
		HIDINPUT(1),        0x00,       //   Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
		END_COLLECTION(0)
	};

	/*
	 * Set report map (here is initialized device driver on client side) (MANDATORY)
	 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.report_map.xml
	 */
	hid->reportMap((uint8_t*)reportMap, sizeof(reportMap));

	/*
	 * We are prepared to start hid device services. Before this point we can change all values and/or set parameters we need.
	 * Also before we start, if we want to provide battery info, we need to prepare battery service.
	 * We can setup characteristics authorization
	 */
	hid->startServices();

	/*
	 * Its good to setup advertising by providing appearance and advertised service. This will let clients find our device by type
	 */
	BLEAdvertising *pAdvertising = pServer->getAdvertising();
	pAdvertising->setAppearance(HID_KEYBOARD);
	pAdvertising->addServiceUUID(hid->hidService()->getUUID());
	pAdvertising->start();


	BLESecurity *pSecurity = new BLESecurity();
	pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

	Serial.println ("Advertising started!");

}

/** send a string as keycode events
 * @param[in] str the string (null terminated)
 */
void sendStr (const char* str) {
// 	Serial.printf("sendStr(%s)\n", str);
	Serial.println("sendStr");
	while(*str) {
		KEYMAP map = keymap[(uint8_t)*str];
		/*
		 * simulate keydown, we can send up to 6 keys
		 */
		uint8_t a[] = {map.modifier, 0x0, map.usage, 0x0,0x0,0x0,0x0,0x0};
		input->setValue(a,sizeof(a));
		input->notify();

		/*
		 * simulate keyup
		 */
		uint8_t v[] = {0x0, 0x0, 0x0, 0x0,0x0,0x0,0x0,0x0};
		input->setValue(v, sizeof(v));
		input->notify();
		str++;
	}
}

void loop() {
	uint8_t pressed = HIGH;

	pressed = digitalRead (SW_PIN);
	if (pressed == LOW && connected == 1) {
		sendStr (KEY);
	}
	delay(150);
}
