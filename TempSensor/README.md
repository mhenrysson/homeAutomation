# homeAutomation tempsensor
ESP8266 NodeMCU based temperature sensor

In Arduino IDE, add the required libraries to compile and run. Set baud rate on serial monitor to 115200.

Button connected to D1<br />
DS18B20 temp sensor connected to D2

Press button to turn unit into wifi access point.<br />
Connect to wifi with ssid 'TempSensor', password 'thereisnospoon'.<br />
Open http://192.168.4.1 to configure wifi the unit should connect to and other sensor settings.<br />
After submitting the form, the unit will close down the access point and try to connect to the specified wifi.

