import csv
import json
import signal
import sys
import smtplib
import thread
import os
from email.mime.text import MIMEText
from time import gmtime, strftime
import time

# load config
with open('config.json') as config:
    configList = json.load(config)

roomList = configList[0]
doorList = configList[1]
eventList = configList[2]
emailList = configList[3]

timerEventList = []

log = open(os.path.join(os.pardir,'http_server/log.txt' ),'a',0)
log.write("System started\n")

#sends mail to emails specified in the config
def sendMail(eventdescription):
    msg = MIMEText(eventdescription)
    msg['Subject'] = 'DMS Alert'
    msg['From'] = 'DoorMonitoringSystem@gmail.com'

    s = smtplib.SMTP_SSL('smtp.gmail.com',465)
    s.login("DoorMonitoringSystem@gmail.com","WSN-Project17")

    for to in emailList:
        msg['To'] = to
        s.sendmail("DoorMonitoringSystem@gmail.com",to,msg.as_string())

    s.quit()

#function used for threaded countdown of a timed event
def timedEventCounter(event, room, timestarted):
    inRoom = 1
    while inRoom:
        if(room["people"] < event["people"]):
                inRoom = 0

        elif (int(time.time()) - timestarted) >= event["time"]*60:
            print_event(event)
            sendMail(event["description"])
            inRoom = 0 #sending email once is enough
            timerEventList.remove(event["description"])

#goes through the eventList and checks whether an event occured
def checkForEvent():
    for event in eventList:

        #number of people in room
        if event["type"] == 0:
            room = lookup_room(event["room"])
            if(room["people"] >= event["people"]):
                print_event(event)
                sendMail(event["description"])

        #no one in the house
        if event["type"] == 1:
            houseEmpty = 1
            for room in roomList:
                if room["people"] != 0 and room["name"] != "OUT":
                    houseEmpty = 0

            if(houseEmpty):
                print_event(event)
                sendMail(event["description"])


        #timed event
        if event["type"] == 2:
            if not event["description"] in timerEventList:
                room = lookup_room(event["room"])
                if room["people"] == event["people"]:
                    thread.start_new_thread(timedEventCounter,(event,room,int(time.time())))
                    timerEventList.append(event["description"])

def copy_to_permalog():
    with open(os.path.join(os.pardir,'http_server/log.txt' ),'r') as log:
        lines = log.readlines()

    #open and close log in write mode to clear
    log = open(os.path.join(os.pardir,'http_server/log.txt' ),'w').close()

    with open(os.path.join(os.pardir,'http_server/permalog.txt' ),'a') as permaLog:
        for line in lines:
            permaLog.write(line)

# handles Ctrl+C termination
def signal_handler(signum,frame):
    print("exiting process")
    log.write(strftime("%Y-%m-%d %H:%M:%S", gmtime()) + ": " +
    "System Stopped\n\n")
    log.close()
    copy_to_permalog()
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
    with open(os.path.join(os.pardir,'http_server/rooms.json'), 'w') as roomfile:
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

def print_event(event):
    string = (strftime("%Y-%m-%d %H:%M:%S", gmtime()) + ": " +
    "Event happened: " + event["description"] + "\n sending Email")
    print(string)
    log.write(string+"\n")


def lookup_door(nodeId):
    for door in doorList:
        if door["frame_receiver"] == nodeId or door["door_receiver"] == nodeId:
            return door

def lookup_room(roomName):
    for room in roomList:
        if(room["name"] == roomName):
            return room

# lists of parameters from the previous received signal
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
while 1:
    row = raw_input()
    now = int(time.time())

    # when logging into the sink some strings are printed that are not signals from motes
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
        battery = (float(currentActionCode)/float(4095))*5
        print(strftime("%Y-%m-%d %H:%M:%S", gmtime()) + ": " +" Battery status of node ("+
        str(currentNodeId) + ") is "+ "%.2fV" % battery)
        if battery <= 2.3:
            sendMail("Battery of mote with nodeid %i is low and needs to be changed" % currentNodeId)

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

            checkForEvent()
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

            checkForEvent()
            #action finished, new action in next step
            lastNodeIDs[currentDoor["id"]] = -1
            lastActioncodes[currentDoor["id"]] = -1
