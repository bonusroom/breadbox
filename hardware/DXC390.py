import serial
from time import sleep


class DXC390(object):
    def __init__(self, port='/dev/tty.usbserial-FTXEKIR7D', iris=0, focus=0, zoom=0):
        self._iris = iris
        self._focus = focus
        self._zoom = zoom
        self._port = port
        self._ser = serial.Serial(self._port, timeout=1)
        self._connected = False

    def connect(self, attempts=3):
        while self._connected == False and attempts > 0:
            self._ser.flushInput()
            handshake = self._ser.read(5)
            if handshake == "OKAY\r":
                self._ser.flushOutput()
                self._ser.write("VERS\r")
                vers = self._ser.read(5)
                if vers == "V5.2\r":
                    self._connected = True
                    return
            else:
                self.disconnect()
                print("Could not connect."),
                if attempts > 0:
                    attempts -= 1
                    print(" -- Trying {} more times.\n".format(attempts))
                    sleep(2)
                else:
                    print(" -- Aborting.\n")

    def disconnect(self):
        self._ser.flushOutput()
        self._ser.write("STOP\r")
        sleep(1)
        self._connected = False

    connected = property(lambda self: self._connected)

    def command(self, prefix, value, low=0, high=255):
        # TODO: check if prefix is 2 chars and 0<value<255
        xmit = "{prefix}{value:02X}\r".format(prefix=prefix, value=value)
        self._ser.write(xmit)
        recv = self._ser.read(5)
        if recv != xmit:
            if recv == "CERR\r":
                print("ERROR: unknown command: {}".format(xmit))
            elif recv == "MERR\r":
                print("ERROR: unable to execute: {}".format(xmit))
            elif recv == "DERR\r":
                print("ERROR: input out of range: {}".format(xmit))
            else:
                print("ERROR: unexpected response: xmit: {} -- recv: {}".format(xmit, recv))

    def set_iris(self, value):
        self.command('AE', value)
        self._iris = value

    iris = property(lambda self: self._iris, set_iris)

    def set_focus(self, value):
        self.command('84', value)
        self._focus = value

    focus = property(lambda self: self._focus, set_focus)

    def set_zoom(self, value):
        self.command('85', value)
        self._zoom = value

    zoom = property(lambda self: self._zoom, set_zoom)
