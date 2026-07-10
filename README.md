# Custom Weather Station

## ESP-32

- Temperature data
- Humidity data
- Wind data (Speed and Direction)
- Move position of camera if requested
- Publish sensor data via MQTT
- Grab camera move requests

## Phone

- Ip webcam (send data over period of time)

## Server

- Display data on custom local website
- Take data and make future predictions
- Take user requests to move camera

### Software on Server

- Mosquitto broker (MQTT broker where esp32 publishes messages)
- Python script (reading esp32 data)
- Prediction/Forcastting
- Custom html page displaying all of it

---

## FUTURE PLANS

### Rasberry Pi

- Handle downlink to weather sats to get overhead images
- Put these into the website and prediction modeler

### Sattelites

| Satellite | Band | Orbit | Notes |
|---|---|---|---|
| Meteor-M | 137 MHz | Polar | current sat will work (only certain times a day) |
| GOES-16/18/19 | L-Band | Geostationary | LNA Required (Availible all day) |
| JPSS NOAA-20/21 | — | — | possibly restricted |
