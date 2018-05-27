## AtmoScan

Instructable can be found [HERE](https://www.instructables.com/id/AtmoScan/).  
See it in action in this [VIDEO](https://www.youtube.com/watch?v=iyFuKU8ZcuA).

**NEWS - UPCOMING CHANGES**
* A small hardware change will allow Atmoscan to **turn off itself from software**, remediating one of the biggest drawbacks of the design - how to handle low battery. The change can be easily applied to boards V1.0. I will publish changes to HW and SW together with a PCB V2.0. Incidentally, Atmoscan might be (to my knowledge at least) the first ESP8266 application that **uses all GPIOs**, including those that allegedly cannot be used - stay tuned
* I am also working on addressing the **instability** that in some cases arise in low battery situations. After extensive oscilloscope play i know what the problem is. Mod not complete yet though.
* Shortly I will post the CAD files for the **complete enclosure**, not only for the inner supports

**AtmoScan** is a multisensor device that measures:

* Temperature
* Humidity
* Pressure
* CO2
* CO
* NO2
* VOC (Air Quality indicator)
* PM 01
* PM25
* PM10
* PM 01
* Radiation

It has an LCD color display, it is gesture controlled and it posts to ThingSpeak (or others) via MQTT, but can properly handle disconnected operations. With a rechargeable battery it lasts a full day when disconnected from power.

It uses a a multitasking cooperative framework and is very responsive to user input while sampling sensors, handling UI, posting to MQTT. I believe it squeezes quite a bit out of the tiny ESP8266...

It integrates a number of open source libraries and leverages internet web services. I started buiding the software with a proper OO application framework in mind and you can find artifacts of it... but never had the time to properly finish it the way i started, so i quickly completed it with way too many hacks. Therefore code could be way better (not properly commented, test code, residuals...).  If anyone is interested in contributing, it's very welcome.

I still have a few PCBs that i am happy to give away to those who want to build it and contribute to it.

### MAIN FEATURES
```
* Color screen
* Gesture control
* Location awareness
* ThingSpeak logging        
* Gracefully handles connected and disconnected operation
* 24h Rechargeable battery
* OTA updates
* Bonus features:
	* Weather Station
	* Plane Spotter

```


### CREDITS


**INCLUDES CODE & LIBRARIES FROM**

```
Adafruit
Arcao
Bblanchon
Bodmer
ClosedCube
Gmag11
Knolleary	
Lucadentella
Seeed
Squix78
Tzapu
Wizard97	
```

**INTEGRATES WEB SERVICES FROM**

```
Adsbexchange.com
GeoNames.org
Google.com
Mylnikov.org
Timezonedb.com
Wunderground.com
```

