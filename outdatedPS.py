import csv
import json
import signal
import sys
import smtplib
from email.mime.text import MIMEText
from time import gmtime, strftime
import time

# load config
with open('src/config.json') as config:
    configList = json.load(config)

roomList = configList[0]
doorList = configList[1]
eventList = configList[2]
to = configList[3]

log = open('/home/user/Workspace/Project/http_server/log.txt','a')

def sendMail(eventcode):
    msg = MIMEText("something alarming happened")
    msg['Subject'] = 'DMS Alert'
    msg['From'] = 'DoorMonitoringSystem@gmail.com'
    msg['To'] = to

    # Send the message via google smtp server.
    s = smtplib.SMTP_SSL('smtp.gmail.com',465)
    s.login("DoorMonitoringSystem@gmail.com","WSN-Project17")
    s.sendmail("DoorMonitoringSystem@gmail.com",to,msg.as_string())
    s.quit()

def checkForEvent():
    return "Nothing for now"

def signal_handler(signum,frame):
    print("exiting process")
    log.write(strftime("%Y-%m-%d %H:%M:%S", gmtime()) + ": " +
    "System Stopped\n")
    log.close()
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

# define print functions
def print_enter(door):
    string = (strftime("%Y-%m-%d %H:%M:%S", gmtime()) + ": " +
    "Someone walked out of the " + door["room_out"] +
    " and into the " + door["room_in"])
    print(string)
    log.write(string+"\n")

def print_leave(door):
    string = (strftime("%Y-%m-%d %H:%M:%S", gmtime()) + ": " +
    "Someone walked out of the " + door["room_in"] +
    " and into the " + door["room_out"])
    print(string)
    log.write(string+"\n")

def print_room(room):
    string = (strftime("%Y-%m-%d %H:%M:%S", gmtime()) + ": " +
    "People in the " + room["name"] + " : " + str(room["people"]))
    print(string)
    log.write(string+"\n")
    with open('/home/user/Workspace/Project/http_server/rooms.json', 'w') as roomfile:
        json.dump(roomList,roomfile)

def print_door_opened(door):
    string = (strftime("%Y-%m-%d %H:%M:%S", gmtime()) + ": " +
    "The " + door["room_out"] + "'s door to the " + door["room_in"] + " has been opened")
    print(string)
    log.write(string+"\n")

def print_door_closed(door):
    string = (strftime("%Y-%m-%d %H:%M:%S", gmtime()) + ": " +
    "The " + door["room_out"] + "'s door to the " + door["room_in"] + " has been closed")
    print(string)
    log.write(string+"\n")

def lookup_door(nodeId):
    for door in doorList:
        if door["frame_receiver"] == nodeId or door["door_receiver"] == nodeId:
            return door

lastMsgTimes = {}
for door in doorList:
    lastMsgTimes[door["id"]] = 0

lastNodeIDs = {}
for door in doorList:
    lastNodeIDs[door["id"]] = -1

lastActioncodes = {}
for door in doorList:
    lastActioncodes[door["id"]] = -1

#mainloop
with open('src/log.csv', 'r') as logfile:
    sinklog = csv.reader(logfile)
    for row in sinklog:
        row = raw_input()
        now = int(time.time())

        if(not row[0].isdigit()):
            continue

        row = row.split(',')
        currentNodeId = int(row[0])
        currentActionCode = int(row[1])
        currentDoor = lookup_door(currentNodeId)
        lastMsgTimes[currentDoor["id"]] = now

        #if nothing has happened at this door for 5 seconds, assume a new action
        if lastMsgTimes[currentDoor["id"]] != 0 and (now - lastMsgTimes[currentDoor["id"]]) >= 5:
            print("resetting door after 8 secs")
            lastNodeIDs[currentDoor["id"]] = -1
            lastActioncodes[currentDoor["id"]] = -1

        # if this is the first input of an action
        if lastNodeIDs[currentDoor["id"]] == -1:
            lastNodeIDs[currentDoor["id"]] = currentNodeId
            lastActioncodes[currentDoor["id"]] = currentActionCode

        #if the door is closed everything coming from the door receiver except a door open is ignored
        if(currentDoor["door_receiver"] == currentNodeId and currentDoor["open_close"] == 2 and currentActionCode != 1):
            continue

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
            str(currentNodeId) + ") is "+ "%0.2fV" % battery)

        #closing doors can be done from either side without entering/leaving a room
        if(currentActionCode == 2):
            continue

        # if last Node Id does not equal current one -> walking through door (if open)
        if lastNodeIDs[currentDoor["id"]] != currentNodeId and currentDoor["open_close"] == 1:
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

                #action finished, new action in next step
                lastNodeIDs[currentDoor["id"]] = -1
                lastActioncodes[currentDoor["id"]] = -1

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

                #action finished, new action in next step
                lastNodeIDs[currentDoor["id"]] = -1
                lastActioncodes[currentDoor["id"]] = -1

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

            #if the door is closed everything coming from the door receiver but a door open is ignored
            if(currentDoor["door_receiver"] == currentNodeId and currentDoor["open_close"] == 2 and currentActionCode != 1):
                continue

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
                str(currentNodeId) + ") is "+ "%0.2fV" % battery)

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
