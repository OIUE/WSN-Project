1. Open config.json and change variables to represent current setup
(Structure is: list of Rooms, list of Doors, list of Events, list of recipient email addresses)
2. Attach sink mote to pc and use 'make login TARGET==*** | python cmdScript.py' to start the monitoring system
3. navigate to http_server and use 'python http.py' to start the server
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

      people:
                number of people as condition for certain events

      time:
                time in minutes as condition for certain event
