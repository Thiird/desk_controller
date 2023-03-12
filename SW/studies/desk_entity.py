import time
import datetime
import platform
import serial # pip install pyserial
import threading

sleepTime =  1.0 # seconds
baudRate = 57600
portName = ""
os = ""
biasLighting = False
msg = ""
cont = 0
event = threading.Event()

def computeBiasLightingValues():
    text = ""
    for x in range(60):
        text += "255,255,255"
    return text

def reader(serialPort):
    global sleepTime
    global event

    while(True):
        temp = serialPort.readline()
        if (temp != b''):
            string = ""
            try:
                string = temp.decode("utf-8")
            except Exception as e:
                print("Can't decode, possible transmission error")
                print(e)
                continue

            if("Bias_ON" in string):
                event.set()
                sleepTime = 0.01
            elif("Bias_OFF" in string):
                event.clear()
                sleepTime = 1.0
            print("Received " + str(len(string))  + ": " + string)

def writer(serialPort):
    global sleepTime
    global cont
    global msg
    global event

    while(True):
        a = datetime.datetime.now()
        msg = os + ': ' + str(cont)
        if(event.is_set()):
            msg += ' ' + computeBiasLightingValues()

        serialPort.write((msg + '\n').encode())
        cont = cont + 1
        print(datetime.datetime.now() - a)
        time.sleep(sleepTime)

if __name__ == "__main__":
    if (platform.system() == "Windows"):
        portName = "COM1"
        os = "Win"

    elif(platform.system() == "Linux"):
        portName = "/dev/ttyS0"
        os = "Lnx"
    else:
        print("Can't detect OS.")
        exit(1)

    threads = []
    with serial.Serial(portName, baudRate, timeout = 1) as serialPort:
        threads.append(threading.Thread(target = reader, args=[serialPort]))
        threads.append(threading.Thread(target = writer, args=[serialPort]))

        for t in threads:
            t.start()

        for t in threads:
            t.join()