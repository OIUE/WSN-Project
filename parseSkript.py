import csv
import json
import signal
import sys
import smtplib
from email.mime.text import MIMEText
from time import gmtime, strftime

# load config
with open('config.json') as config:
    configList = json.load(config)

roomList = configList[0]
doorList = configList[1]
eventList = configList[2]
to = configList[3]

serverLog = open('http_server/log.txt','a')

def signal_handler(signum,frame):
    print("exiting process")
    log.write(strftime("%Y-%m-%d %H:%M:%S", gmtime()) + ": " +
    "System Stopped")
    serverLog.close()
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

# define print functions
def print_enter(door):
    string = (strftime("%Y-%m-%d %H:%M:%S", gmtime()) + ": " +
    "Someone walked out of the " + door["room_out"] +
    " and into the " + door["room_in"])
    print(string)
    serverLog.write(string+"\n")

def print_leave(door):
    string = (strftime("%Y-%m-%d %H:%M:%S", gmtime()) + ": " +
    "Someone walked out of the " + door["room_in"] +
    " and into the " + door["room_out"])
    print(string)
    serverLog.write(string+"\n")

def print_room(room):
    string = (strftime("%Y-%m-%d %H:%M:%S", gmtime()) + ": " +
    "People in the " + room["name"] + " : " + str(room["people"]))
    print(string)
    serverLog.write(string+"\n")
    with open('http_server/rooms.json', 'w') as roomfile:
        json.dump(roomList,roomfile)

def print_door_opened(door):
    string = (strftime("%Y-%m-%d %H:%M:%S", gmtime()) + ": " +
    "The " + door["room_out"] + "'s door to the " + door["room_in"] + " has been opened")
    print(string)
    serverLog.write(string+"\n")

def print_door_closed(door):
    string = (strftime("%Y-%m-%d %H:%M:%S", gmtime()) + ": " +
    "The " + door["room_out"] + "'s door to the " + door["room_in"] + " has been closed")
    print(string)
    serverLog.write(string+"\n")

def lookup_door(nodeId):
    for door in doorList:
        if door["frame_receiver"] == nodeId or door["door_receiver"] == nodeId:
            return door


lastNodeIDs = {}
for door in doorList:
    lastNodeIDs[door["id"]] = -1

lastActioncodes = {}
for door in doorList:
    lastActioncodes[door["id"]] = -1

#load log
with open('firstlog.csv','r') as log:
    log_reader = csv.reader(log)

    for row in log_reader:
        currentNodeId = int(row[0])
        currentActionCode = int(row[1])
        currentDoor = lookup_door(currentNodeId)

        # if this is the first input
        if lastNodeIDs[currentDoor["id"]] == -1:
            lastNodeIDs[currentDoor["id"]] = currentNodeId
            lastActioncodes[currentDoor["id"]] = currentActionCode

        #if door is currently closed and being opened
        if currentActionCode == 1 and currentDoor["open_close"] == 2:
            print_door_opened(currentDoor)
            currentDoor["open_close"] = 1
        #if door is currently open and being closed
        elif currentActionCode == 2 and currentDoor["open_close"] == 1:
            print_door_closed(currentDoor)
            currentDoor["open_close"] = 2
        elif currentActionCode > 2: #battery
            # 2.1V is the minimum power to operate the radio, so send warning before that
            # battery value of 1883.7 is equal to 2.3V
            battery = (currentActionCode/4095)*5
            print(strftime("%Y-%m-%d %H:%M:%S", gmtime()) + ": " +" Battery status of node ("+
            str(currentNodeId) + ") is "+ "%0.2f" % battery)

        #closing doors can be done from either side without entering/leaving a room
        if(currentActionCode == 2):
            continue

        # if last Node Id does not equal current one
        if lastNodeIDs[currentDoor["id"]] != currentNodeId:
            # if the siganl is of a door receiver, it means that the previous
            # one had to be of a frame receiver -> someone walked into the room
            if currentNodeId == currentDoor["door_receiver"]:
                print_enter(currentDoor)

                for room in roomList:
                    if room["name"] == currentDoor["room_out"]:
                        room["people"] -= 1
                        print_room(room)

                for room in roomList:
                    if room["name"] == currentDoor["room_in"]:
                        room["people"] += 1
                        print_room(room)

                lastNodeIDs[currentDoor["id"]] = currentNodeId
                lastActioncodes[currentDoor["id"]] = currentActionCode

            #previously door_receiver pass or open, now frame_receiver -> someone walked out of the room
            else:
                print_leave(currentDoor)

                for room in roomList:
                    if room["name"] == currentDoor["room_out"]:
                        room["people"] += 1
                        print_room(room)

                for room in roomList:
                    if room["name"] == currentDoor["room_in"]:
                        room["people"] -= 1
                        print_room(room)

                lastNodeIDs[currentDoor["id"]] = currentNodeId
                lastActioncodes[currentDoor["id"]] = currentActionCode
