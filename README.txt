1. Open config.json and change variables to represent current setup
(Structure is: List of Rooms, List of Doors, List of Events, recipient email address)
2. use 'python http.py' to start the http server
3. Attach sink mote to pc and use 'make login TARGET==*** | python cmdScript.py' to start the monitoring system
4. vistit localhost:8080 to access the web server


Receiver and Sender motes can send current battery status with single button press.
Pressing Receiver button twice resets normal RSSI.



Code Mappings

Doors:
  open_close:
            1 == open door
            2 == closed door

Events:
      Condition:
                0 = Timed event. Event is triggered when time defined in "Time" has passed with someone being in the room
                1 = No one in the room event. Event is triggered when room is empty
                2 = Someone in the room event. Event is triggered when room isn't empty

                10 = No one in the house event.
                20 = Someone in the House event
