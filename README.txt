1. Open config.json and change variables to represent current setup
(Structure is: List of Rooms, List of Doors, List of Events, recipient email address)
2. use 'python http.py' to start the http server
3. Attach sink mote to pc and use 'make login TARGET==*** | python cmdScript.py' to start the monitoring system
4. vistit localhost:8080 to access the web server


Receiver and Sender motes can send current battery status with single button press.
Pressing Receiver button twice resets normal RSSI.



Config explanation

Doors:
  open_close:
            1 == open door
            2 == closed door

Events:
      type:
                0 = Number of people in a room event. Event is triggered when a certain number is reached/exceeded
                1 = No one in the house event. Event is triggered when house is empty
                2 = Timed event. Event is triggered when defined time between entering and leaving a room has run out.
                Only exemplary. More event types can be added in future 

      people:
                number of people as condition for certain events

      time:
                time in minutes as condition for certain event
