## AddTime

Maven build

Argument list (all optional but read in order, i.e. first argument is always broker URL, second topic etc)
* MQTT broker URL, default: tcp://test.mosquitto.org:1883
* MQTT topic prefix to enrich, default: /addtime
* MQTT client ID, default: htester1
* SimpleDateFormat to use for message time stamp, default: yyyy-MM-dd HH:mm:ss Z
