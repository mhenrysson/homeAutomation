# homeAutomation tempsensor
nodeMCU based temperature sensor

Created using Arduino IDE

Button connected to D1
DS18B20 temp sensor connected to D2

Press button to turn unit into wifi access point.
Connect to wifi with ssid 'TempSensor', password 'thereisnospoon'.
Open http://192.168.4.1 to configure wifi the unit should connect to and other sensor settings.
After submitting the form, the unit will close down the access point and try to connect to the specified wifi.

