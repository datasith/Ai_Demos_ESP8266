'''
  ------------------------------------------------------------------------------
  10/24/2018
  Author: Makerbro
  Platforms: OSX/Linux/Windows
  Language: Python (v2.7)
  File: rgb.py
  ------------------------------------------------------------------------------
  Description: 
  Code for YouTube video demonstrating how to deploy an IoT app. It involves
  running a web server on a cloud service (in this case Heroku), and using
  websockets to get the input from a user—the color of an RGB LED—via a webpage 
  served by the web server. The selected color is sent to the ESP8266, which 
  is setup to act as a client on the websocket.
  https://youtu.be/mrxgH1vH5NE

  Do you like my videos? You can support the channel:
  https://patreon.com/acrobotic
  https://paypal.me/acrobotic
  ------------------------------------------------------------------------------
  Please consider buying products from ACROBOTIC to help fund future
  Open-Source projects like this! We'll always put our best effort in every
  project, and release all our design files and code for you to use. 

  https://acrobotic.com/
  https://amazon.com/acrobotic
  ------------------------------------------------------------------------------
  License:
  Please see attached LICENSE.txt file for details.
  ------------------------------------------------------------------------------
'''
from flask import Flask, request, render_template
from flask_sockets import Sockets
import gevent
import time
import json

'''
Setting up the webserver
'''
app = Flask(__name__)
app.debug = True

'''
Setting up the websockets
'''
sockets = Sockets(app)

'''
Handling multiple clients
'''
clients = list()

'''
Store red, green, blue color data globally
'''
rgb = {'r':0, 'g':0, 'b':0}

'''
User-defined class for clients
'''
class Client:
	def __init__(self):
		self.queue = gevent.queue.Queue()
	def put(self, v):
		self.queue.put_nowait(v)
	def get(self):
		return self.queue.get()
'''
User-defined function for sending data on the 
websocket to all clients
'''
def send_all_json(msg):
	for client in clients:
		client.put(json.dumps(msg))
'''
Helper function for testing the validity of json format
'''
def is_json(text):
	try:
		json.loads(text)
	except ValueError, e:
		return False
	return True

'''
Greenlet function to read from websocket
'''
def read_ws(ws, client):
	while not ws.closed:
		gevent.sleep(0)
		try:
			msg = ws.receive() # This command blocks!
			print "WS RECEIVED: %s" % msg
			if(msg is None):
				client.put(msg)
			elif (is_json(msg)):
				send_all_json(json.loads(msg)) # Assuming the data is properly formatted :)
			else:
				raise ValueError('Not a JSON string')
		except:
			print "WS ERROR: read_ws exception"

'''
Adding a route to the websockets server
to "subscribe" clients
'''
@sockets.route('/subscribe')
def subscribe_socket(ws):
	client = Client() # User-defined object to store client info
	clients.append(client) 
	print '# Clients: {}'.format(len(clients))
	g = gevent.spawn( read_ws, ws, client)
	# If data is received by the queue, send it to the websocket
	try:
		while g:
			msg = client.get() # This command blocks!
			if msg is not None:
				ws.send(msg)
			else:
				break
	except Exception as e:
		print "WS ERROR: %s" % e
	finally:
		ws.close()
		clients.remove(client)
		gevent.kill(g)

'''
Adding a route to the webserver
'''
@app.route('/')
def index():
	return render_template("index.html")
