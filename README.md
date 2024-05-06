# SnoozeProof

This is the source code for my custom-built alarm clock. The code is intended to run on an RP2040 microcontroller connected to an HC05 Bluetooth IC, piezo buzzer, and force-sensitive resistor. The HC05 allows alarms to be set remotely over Bluetooth Serial. The force-sensitive resistor is used to detect whether anyone is in the bed, and the buzzer is used to sound the alarm.

Alarms are set as constant time windows with a defined start and end time. The alarms can be set using custom commands over the Bluetooth serial connection and whenever the current time is not within an alarm window. When inside the window, it is impossible to disable the alarms. If extra weight is detected above a settable threshold during an alarm window, then the alarm will sound.
