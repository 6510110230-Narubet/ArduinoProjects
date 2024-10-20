import serial
import requests
import time

# UART configuration
SERIAL_PORT = '/dev/ttyS1'
BAUD_RATE = 9600

# ThingSpeak configuration
THINGSPEAK_URL = f'https://api.thingspeak.com/update?api_key=PP37ZKIOJ1BI08J4&field1=0'

# Line Notify configuration
LINE_NOTIFY_URL = 'https://notify-api.line.me/api/notify'
LINE_NOTIFY_TOKEN = 'Z8WJXV36iBhLiTqzcLRsIkgUNqWg0PYC0K9i4z6GWRY'

def send_to_thingspeak(pm25_value):
    payload = {
        'field1': pm25_value
    }
    try:
        response = requests.post(THINGSPEAK_URL, params=payload)
        if response.status_code == 200:
            print(f"Data sent to ThingSpeak: PM2.5 = {pm25_value}")
        else:
            print(f"Failed to send data to ThingSpeak. Status code: {response.status_code}")
    except Exception as e:
        print(f"Error sending data to ThingSpeak: {e}")

def send_to_line_notify(pm25_value):
    headers = {
        'Authorization': f'Bearer {LINE_NOTIFY_TOKEN}'
    }
    payload = {
        'message': f'PM2.5 Level: {pm25_value} ug/m3'
    }
    try:
        response = requests.post(LINE_NOTIFY_URL, headers=headers, data=payload)
        if response.status_code == 200:
            print(f"PM2.5 notification sent to Line: {pm25_value} ug/m3")
        else:
            print(f"Failed to send notification to Line. Status code: {response.status_code}")
    except Exception as e:
        print(f"Error sending notification to Line: {e}")

def main():
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Connected to {SERIAL_PORT} at {BAUD_RATE} baud.")
    while True:
        try:
            line = ser.readline().decode('utf-8').strip()
            if line.startswith("PM25:"):
                pm25_value = int(line.split(":")[1])
                print(f"Received PM2.5: {pm25_value} ug/m3")
                send_to_thingspeak(pm25_value)
                send_to_line_notify(pm25_value)
        except Exception as e:
            print(f"Error: {e}")
        time.sleep(15) 

if __name__ == "__main__":
    main()