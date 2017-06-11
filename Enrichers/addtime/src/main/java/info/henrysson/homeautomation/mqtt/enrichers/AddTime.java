package info.henrysson.homeautomation.mqtt.enrichers;

import com.google.gson.Gson;
import com.google.gson.JsonObject;
import org.eclipse.paho.client.mqttv3.*;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.concurrent.TimeUnit;

/**
 * Created by Mattias on 2017-06-10.
 *
 * Subsribes to mqtt topic ENRICH_TOPIC/#
 * When a message is published, AddTime grabs the message,
 * adds a time stamp and publishes on remaining topic,
 * i.e. messages published to <ENRICH_TOPIC>/<suffix> will
 * be republished to topic <suffix>.
 */
public class AddTime implements MqttCallback {

  private static String CLIENT_ID = "htester1";
  private static String BROKER_URL = "tcp://test.mosquitto.org:1883";
  private static String ENRICH_TOPIC = "/addtime";
  private static SimpleDateFormat DATE_FORMAT = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss Z");

  private MqttClient client;

  public AddTime() {}

  /***
   * @param args = [MQTT_BROKER_URL [TOPIC_PATTERN_TO_ENRICH [MQTT_CLIENT_ID [DATE_FORMAT]]]]
   */
  public static void main(String... args) {
    if(args.length > 0)
      BROKER_URL = args[0];
    if(args.length > 1)
      ENRICH_TOPIC = args[1];
    if(args.length > 2)
      CLIENT_ID = args[2];
    if(args.length > 3)
      DATE_FORMAT = new SimpleDateFormat(args[3]);
    new AddTime().connect();
  }

  private void connect() {
    MqttConnectOptions connOpts = new MqttConnectOptions();
    connOpts.setCleanSession(true);
    connOpts.setKeepAliveInterval(30);
    connOpts.setAutomaticReconnect(true);
    try {
      client = new MqttClient(BROKER_URL, CLIENT_ID);
      client.setCallback(this);
      client.connect(connOpts);
      System.out.println("AddTime Enricher connected to " + BROKER_URL);
      client.subscribe(ENRICH_TOPIC + "/#");
    } catch (MqttException e) {
      e.printStackTrace();
    }
  }

  /**
   * Automatic reconnect enabled so no need for reconnect in method.
   * Sleep to avoid hammering broker.
   */
  public void connectionLost(Throwable throwable) {
    System.out.println(DATE_FORMAT.format(new Date()) + " AddTime - Connection lost.");
    throwable.printStackTrace();
    try {
      TimeUnit.SECONDS.sleep(10);
    } catch (InterruptedException e) {
      e.printStackTrace();
    }
  }

  /**
   * Callback function for new messages on subscribed topics.
   * Add time stamp to published message and republish
   */
  public void messageArrived(String s, MqttMessage mqttMessage) throws Exception {
    Gson gson = new Gson();
    String newTopic = s.substring(ENRICH_TOPIC.length());
    JsonObject message = gson.fromJson(new String(mqttMessage.getPayload()), JsonObject.class);
    message.addProperty("time", DATE_FORMAT.format(new Date()));
    mqttMessage.setPayload(message.toString().getBytes());
    client.publish(newTopic, mqttMessage);
  }

  public void deliveryComplete(IMqttDeliveryToken iMqttDeliveryToken) {

  }
}
