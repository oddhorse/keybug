#include "ble.h"

#include <Arduino.h>

#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

#include "hid/hid.h"

// BLE Service
BLEDfu bledfu;	 // OTA DFU service
BLEDis bledis;	 // device information
BLEUart bleuart; // uart over ble
BLEBas blebas;	 // battery

static void startAdv(void);
static void connect_callback(uint16_t conn_handle);
static void disconnect_callback(uint16_t conn_handle, uint8_t reason);

void ble_init()
{
	// Setup the BLE LED to be enabled on CONNECT
	// Note: This is actually the default behavior, but provided
	// here in case you want to control this LED manually via PIN 19
	Bluefruit.autoConnLed(true);

	// Config the peripheral connection with maximum bandwidth
	// more SRAM required by SoftDevice
	// Note: All config***() function must be called before begin()
	Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

	Bluefruit.begin();
	Bluefruit.Periph.setConnInterval(6, 12); // units of 1.25ms → 7.5–15ms range
	Bluefruit.setTxPower(4);				 // Check bluefruit.h for supported values
	// Bluefruit.setName(getMcuUniqueID()); // useful testing with multiple central connections
	Bluefruit.setName("keybug!");
	Bluefruit.Periph.setConnectCallback(connect_callback);
	Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

	// To be consistent OTA DFU should be added first if it exists
	bledfu.begin();

	// Configure and Start Device Information Service
	bledis.setManufacturer("oddhorse");
	bledis.setModel("keybug");
	bledis.begin();

	// Configure and Start BLE Uart Service
	bleuart.begin();

	// Start BLE Battery Service
	blebas.begin();
	blebas.write(100);

	// Set up and start advertising
	startAdv();

	Serial.println("connect via python script!");
	Serial.println("Once connected start typing");
}

static void startAdv(void)
{
	// Advertising packet
	Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
	Bluefruit.Advertising.addTxPower();

	// Include bleuart 128-bit uuid
	Bluefruit.Advertising.addService(bleuart);

	// Secondary Scan Response packet (optional)
	// Since there is no room for 'Name' in Advertising packet
	Bluefruit.ScanResponse.addName();

	/* Start Advertising
	 * - Enable auto advertising if disconnected
	 * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
	 * - Timeout for fast mode is 30 seconds
	 * - Start(timeout) with timeout = 0 will advertise forever (until connected)
	 *
	 * For recommended advertising interval
	 * https://developer.apple.com/library/content/qa/qa1931/_index.html
	 */
	Bluefruit.Advertising.restartOnDisconnect(true);
	Bluefruit.Advertising.setInterval(32, 244); // in unit of 0.625 ms
	Bluefruit.Advertising.setFastTimeout(30);	// number of seconds in fast mode
	Bluefruit.Advertising.start(0);				// 0 = Don't stop advertising after n seconds
}

static uint8_t frame_buf[7];
static uint8_t frame_len = 0;

void ble_task()
{

	// Capture from BLEUART and dispatch as keystrokes
	// frame structure: [event_type: u8][code: u8][value: i16][value2: i16][modifiers: u8]
	while (bleuart.available())
	{
		frame_buf[frame_len++] = bleuart.read();
		if (frame_len == 7)
		{
#ifdef DEV_BUILD
			int16_t value = (int16_t)((uint16_t)frame_buf[2] | ((uint16_t)frame_buf[3] << 8));
			int16_t value2 = (int16_t)((uint16_t)frame_buf[4] | ((uint16_t)frame_buf[5] << 8));
			Serial.print("event_type (u8): ");
			Serial.print(frame_buf[0], HEX);
			Serial.print("; code (u8): ");
			Serial.print(frame_buf[1], HEX);
			Serial.print("; value (i16): ");
			Serial.print(value);
			Serial.print("; value2 (i16): ");
			Serial.print(value2);
			Serial.print("; modifiers (u8): ");
			Serial.print(frame_buf[6], HEX);
			Serial.print("\n");
#endif

			enqueue_frame(frame_buf); // decode + act
			frame_len = 0;
		}
	}
}

// callback invoked when central connects
static void connect_callback(uint16_t conn_handle)
{
	// Get the reference to current connection
	BLEConnection *connection = Bluefruit.Connection(conn_handle);

	char central_name[32] = {0};
	connection->getPeerName(central_name, sizeof(central_name));

	Serial.print("Connected to ");
	Serial.println(central_name);
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
static void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
	(void)conn_handle;
	(void)reason;

	Serial.println();
	Serial.print("Disconnected, reason = 0x");
	Serial.println(reason, HEX);
}