# Model Radar Demonstration

The demonstration for this HAL/RTOS was the creation of a simple "model radar" system similar to those seen in classic (even cliche) war movies and shows. To give a gist of the idea, I have created a system that continuously moves a range-finding sensor back and forth and gives visual "pings" when an object is detected in a particular direction. If you can envision the old radar module with green dots that slowly fade out after a detection, that is the crux of what I have emulated. This project, in particular, works primarily with three personalized hardware drivers: an ultrasonic sensor, a stepper motor, and a Neopixel ring LED. Below you can find the details about these pieces of hardware and my wiring.

## Hardware References / Configuration
Here are the appropriate datasheets (which can also be found in the `ultrasonic.h`, `pix.h`, and `stepper.h` header files, respectively):

HC-SR04 (ultrasonic sensor): https://www.handsontec.com/dataspecs/HC-SR04-Ultrasonic.pdf

SK6812 (LED ring light driver): https://www.digikey.com/htmldatasheets/production/1876263/0/0/1/sk6812-datasheet.html?gad_source=1&gad_campaignid=120565755&gbraid=0AAAAADrbLlhSB6PwtzGYZOxIahRlOitZr&gclid=Cj0KCQiAosrJBhD0ARIsAHebCNrL5hliexnJs8HBbyLdp7V7NrYbmRgTa0UHymB8GQOTiCCL8xL1N9oaAlTjEALw_wcB&gclsrc=aw.ds

Arduino Stepper Library (Common driver for 28BYJ-48): https://github.com/arduino-libraries/Stepper

Similarly, as follows are my GPIO connections (for replication using the constants I currently have defined in the header files / initialization functions). All other ground / power connections are connected either to the common NRF52840 ground or the USB pin (providing 5V from plugged in microUSB).

Stepper Motor IN1-IN4: D5, D6, D7, D8

Data-In Neopixel Ring LED: D11

HC-SR04 Trigger / Echo: D12, D13
