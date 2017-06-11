package info.henrysson.homeautomation.mqtt.enrichers;

import com.google.gson.Gson;
import com.google.gson.JsonObject;
import org.eclipse.paho.client.mqttv3.*;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.concurrent.TimeUnit;

/**
 * Created by Mattias on 2017-06-10.
 */
public class AddTime implements MqttCallback {

  private static final String CLIENT_ID = "htester1";
  private static final String BROKER_URL = "tcp://test.mosquitto.org:1883";
  private static final String ENRICH_TOPIC = "/addtime";
  private static final SimpleDateFormat DATE_FORMAT = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss Z");

  private MqttClient client;

  public AddTime() {}

  public static void main(String... args) {
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

  public void connectionLost(Throwable throwable) {
    System.out.println("AddTime - Connection lost.");
    throwable.printStackTrace();
    try {
      TimeUnit.SECONDS.sleep(10);
    } catch (InterruptedException e) {
      e.printStackTrace();
    }
  }

  public void messageArrived(String s, MqttMessage mqttMessage) throws Exception {
    Gson gson = new Gson();
    String newTopic = s.substring(ENRICH_TOPIC.length());
    JsonObject message = gson.fromJson(new String(mqttMessage.getPayload()), JsonObject.class);
    message.addProperty("time", DATE_FORMAT.format(new Date()));
    System.out.println(message);
    mqttMessage.setPayload(message.toString().getBytes());
    client.publish(newTopic, mqttMessage);
  }

  public void deliveryComplete(IMqttDeliveryToken iMqttDeliveryToken) {

  }
}
