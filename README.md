# KH station
Autosampling titrator for measuring of dKH level in saltwater aquarium

[comment]: <> (![Travis CI status]&#40;https://api.travis-ci.org/witnessmenow/igorlab/KH_station.svg?branch=master&#41;)

[comment]: <> (![Travis CI status]&#40;https://api.travis-ci.org/witnessmenow/igorlab/KH_station.svg?branch=master&#41;)
![License](https://img.shields.io/badge/license-GPL3.0-green)
![Release stable](https://badgen.net/github/release/igorlab/KH_station/stable)

![All parts](Assembling/img/front1.jpg)
![All parts](Assembling/img/back1.jpg)

[![](https://markdown-videos.vercel.app/youtube/T8ol2PM2Kjg)](https://youtu.be/T8ol2PM2Kjg)

Join the [Arduino Aquarium titrator Group Chat](https://t.me/+Ad4m-7L7tV1lNGNi) if you have any questions/feedback or
would just like to be kept up to date with the project progress.


## Getting Started

1) Register on https://labaqua.net/ get token on email

2) To get Telegram notification you should generate new Bot and get an Access Token. Talk to [BotFather](https://telegram.me/botfather) and follow a few simple steps described [here](https://core.telegram.org/bots#botfather).

3) Connect device to PC throughout USB, open COM-port terminal (monitor_speed: 115200) and do next steps

4) Set the PASSWORD for your WiFi, e.g. wifipasw_YourWiFi#YourPassword

5) Set the token from email, **ukey_1Egu-78jt7NpQ7gau5d778a299e0a43,67773777**

6) Set the telegram bot access token, e.g. **bottoken_123789548:RRRRRGseAZ1ikRIhpjGa1-asdfghGVWqwers**

7) Get your telegram ID and set up it to the device so only you have access, **settlgrmid_123456789**
Restart the device, if all is done fine you see the notification "KH station started" on your chatbot.

## Calibration

1) Open chatbot, click "Calibrations"
2) Select "Calibrate pumps"
3) Setup the desired hose-tip over the weighed vessel and click "Run <desired> pump test" 
4) After ending pumping (beeping three times) measure the mass of the pumped liquid.
If the mass is ***out of the admissible¹ interval*** the real pumped mass should be sent to the device for correction.
Click "Set real <PUMP> mass", after that enter the mass, e.g. ***24.87***
Repeat calibrations until getting admissible¹ results.

¹ reagent 10.00±0.05 g, water 25.00±0.05 g

  
