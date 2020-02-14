
#include "application.h"
#include "Arduino_KeyPadLCD_Shield.h"
#include "Adafruit_DHT_Particle.h"
#include <MQTT.h>


DHT dht(D2, DHT22);

LiquidCrystal lcd(A5, A4, D6, D0, D1, D7); // Shield shield v3.0.1

int lastDSTCheckDay = 0;

void callback(char* topic, byte* payload, unsigned int length);
MQTT client("192.168.2.226", 1883, callback);
static const char* deviceTopic = "home1/masterbedroom/device/tempsensor";

static const char* tempTopic  = "home1/masterbedroom/temp";
static const char* humidTopic = "home1/masterbedroom/humidity";

// recieve message
void callback(char* topic, byte* payload, unsigned int length)
{
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = NULL;

    String reboot("reboot");

    String ps(p);
    if(reboot.equalsIgnoreCase(ps))
    {
        Particle.publish("INFO", "Reboot request received", 60, PRIVATE);
        System.reset();
    }
}

bool isDST(int day, int month, int dow)
{
    // inputs are assumed to be 1 based for month & day.  DOW is ZERO based!

    // jan, feb, and dec are out
    if(month < 3 || month > 11)
    {
        return false;
    }

    // april - oct is DST
    if(month > 3 && month < 11)
    {
        return true;
    }

    // in March, we are DST if our previous Sunday was on or after the 8th
    int previousSunday = day - dow;
    if(month == 3)
    {
        return previousSunday >= 8;
    }

    // for November, must be before Sunday to be DST ::  so previous Sunday must be before the 1st
    return previousSunday <= 0;
}

void handle_dst()
{
    int offset = -5;
    Time.zone(offset);
    if(isDST(Time.day(), Time.month(), Time.weekday()-1))
    {
        Time.beginDST();
    }
    else
    {
        Time.endDST();
    }

    lastDSTCheckDay = Time.day();
}

void connect()
{
    if(Particle.connected() == false)
    {
        Particle.connect();               //Connect to the Particle Cloud
        waitUntil(Particle.connected);    //Wait until connected to the Particle Cloud.
    }

    if(lastDSTCheckDay != Time.day())
    {
        handle_dst();
    }

    Particle.syncTime();

    // connect to the server

    if(!client.isConnected())
    {
        Particle.publish("MQTT", "Connecting", 60 , PRIVATE);
        client.connect("sittingrmtemp");
        Particle.publish("MQTT", "Connected", 60, PRIVATE);

        client.subscribe(deviceTopic);
    }
}

void mqtt_publish(const char* topic, int value)
{
    String v(value);
    client.publish(topic, v);
}

void setup()
{
    Serial.begin(9600);
    dht.begin();

    pinMode(A0, INPUT);  // pin for button array (read analog value)
                         // make sure KeyPad LCD shield never exceedes 3.3V
                         // on this pin (measure with no button pressed)

    lcd.begin(16,2);

    connect();
}

int lastReadTime = 0;

void loop()
{
    if (client.isConnected())
    {
        client.loop();
    }
    else
    {
        connect();
    }

    // read temp & humidity every 5 seconds, then publish it
    int now = Time.now();

    lcd.setCursor(0, 0);
    lcd.print(Time.format(Time.now(), "%m/%d/%y %I:%M%p."));

    if(now - lastReadTime >= 120)
    {
        lastReadTime = now;

        float temp = dht.getTempFarenheit();
        float humidity = dht.getHumidity();

    	if (isnan(humidity) || isnan(temp))
    	{
		    Serial.println("Failed to read from DHT sensor!");
		    return;
	    }

        int t = (int)(temp+0.5);
        int h = (int)(humidity+0.5);

        lcd.setCursor(0,1);
        String line2 = "Temp:" + String(t)+ "F Hum:" + String(h)+ "%";
        lcd.print(line2);

        if(Particle.connected() == false)
        {
            Particle.connect();
            waitUntil(Particle.connected);
        }

        Particle.publish("SITRMTEMP", String(temp), 60, PRIVATE);
        mqtt_publish(tempTopic, temp);

        Particle.publish("SITRMHUMID", String(humidity), 60, PRIVATE);
        mqtt_publish(humidTopic, humidity);
    }
}
