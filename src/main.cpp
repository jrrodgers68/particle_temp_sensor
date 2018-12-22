
#include "application.h"
#include "Arduino_KeyPadLCD_Shield.h"
#include "Adafruit_DHT_Particle.h"



DHT dht(D2, DHT22);

LiquidCrystal lcd(A5, A4, D6, D0, D1, D7); // Shield shield v3.0.1


void setup()
{
    Serial.begin(9600);
    dht.begin();

    pinMode(A0, INPUT);  // pin for button array (read analog value)
                         // make sure KeyPad LCD shield never exceedes 3.3V
                         // on this pin (measure with no button pressed)

    lcd.begin(16,2);


    Time.zone(-4);
    Particle.syncTime();

    Particle.connect();
    waitUntil(Particle.connected);
    delay(2000);
}

int lastReadTime = 0;

void loop()
{
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
        Particle.publish("SITRMHUMID", String(humidity), 60, PRIVATE);
    }
}
