import tornado.web
import tornado.autoreload
import tornado.ioloop
import os
import json

settings = {
    'template_path' :os.path.join(os.path.dirname(__file__), "templates")
}

class MainHandler(tornado.web.RequestHandler):
    def get(self):
        with open('log.txt','r') as logfile:
            logList = reversed(list(logfile))
            self.render("log.html", param=logList)

class RoomHandler(tornado.web.RequestHandler):
    def get(self):
        with open('rooms.json','r') as roomfile:
            roomList = json.load(roomfile)
            self.render("room.html",param = roomList)


handlers = [
    (r'/', MainHandler),
    (r'/rooms', RoomHandler)
]

def signal_handler(signum, frame):
    tornado.ioloop.IOLoop.current().stop()

if __name__ == "__main__":
    app = tornado.web.Application(handlers, **settings)
    app.listen(8080)
    tornado.autoreload.start()
    print("Server loaded")

    #reload server everytime a file changes (debug)
    for dir,_,files in os.walk("."):
        [tornado.autoreload.watch(os.path.join(dir, f)) for f in files if not f.startswith('.')]

    tornado.ioloop.IOLoop.current().start()
