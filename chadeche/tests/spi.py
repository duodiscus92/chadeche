 #--------------------------------------------------------------


import time
from threading import Timer
import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)
#pin definition
#CLK= 2
CLK= 24
#DOUT=3
DOUT=4
CS=25

GPIO.setup(CLK,GPIO.OUT)
GPIO.setup(DOUT,GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(CS,GPIO.OUT)
while True:
    ADCdata=0
    GPIO.output(CS, True) 
    time.sleep(0.0050)
    time.sleep(0.0050)
    GPIO.output(CS, False)
    time.sleep(0.050)#250ms delay or 0.25s delay
    for x in range (0,1):
      if (GPIO.input(CLK)== True):
          GPIO.output(CLK, False) 
      else:
        GPIO.output(CLK, True)
        time.sleep(0.0050)
        time.sleep(0.0050)
    for i in range (0,16):
      if GPIO.input(CLK)== True:
        time.sleep(0.0050)
        time.sleep(0.0050)
        if GPIO.input(DOUT)== True:
           ADCdata+=1
        ADCdata<<=1
        GPIO.output(CLK, False)
        time.sleep(0.0050)
        time.sleep(0.0050)
      else:
        GPIO.output(CLK, True)
        time.sleep(0.0050)
        time.sleep(0.0050)
    ADCdata >>=1;
    ADCFloat=float(ADCdata)
    Voltage=ADCFloat*5.0/256
    print("Raw data is: ", ADCdata, "Voltage is: ", Voltage, " V")
GPIO.cleanup()#to release any resources that script is using

#-----------------END-----------------------------------------------
