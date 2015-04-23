import serial
import io
from time import sleep


class DXC390(object):
    def __init__(self, port='/dev/tty.usbserial-FTXEKIR7D',
                 iris=0, focus=0, zoom=0,
                 pedestal=128, rpedestal=128, bpedestal=128,
                 detail = 128):
        self._port = port
        self._ser = serial.Serial(self._port, timeout=1)
        self._connected = False
        self._iris = iris
        self._init_iris = iris
        self._focus = focus
        self._init_focus = focus
        self._zoom = zoom
        self._init_zoom = zoom
        self._pedestal = pedestal
        self._init_pedestal = pedestal
        self._rpedestal = rpedestal
        self._init_rpedestal = rpedestal
        self._bpedestal = bpedestal
        self._init_bpedestal = bpedestal
        self._detail = detail
        self._init_detail = detail

    def connect(self, attempts=3):
        while self._connected is False and attempts > 0:
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
                print("ERROR: unknown command: {}".format(xmit.strip('\r')))
            elif recv == "MERR\r":
                print("ERROR: unable to execute: {}".format(xmit.strip('\r')))
            elif recv == "DERR\r":
                print("ERROR: input out of range: {}".format(xmit.strip('\r')))
            else:
                print("ERROR: unexpected response: xmit: {} -- recv: {}".format(xmit.strip('\r'), recv.strip('\r')))

    def get_iris(self):
        return self._iris

    def set_iris(self, value):
        self.command('AE', value)
        self._iris = value

    iris = property(get_iris, set_iris)

    def get_focus(self):
        return self._focus

    def set_focus(self, value):
        self.command('84', value)
        self._focus = value

    focus = property(get_focus, set_focus)

    def get_zoom(self):
        return self._zoom

    def set_zoom(self, value):
        self.command('85', value)
        self._zoom = value

    zoom = property(get_zoom, set_zoom)

    def get_pedestal(self):
        return self._pedestal

    def set_pedestal(self, value):
        self.command('AD', value, low=1)
        self._pedestal = value

    pedestal = property(get_pedestal, set_pedestal)

    def get_rpedestal(self):
        return self._rpedestal

    def set_rpedestal(self, value):
        self.command('A8', value, low=1)
        self._rpedestal = value

    rpedestal = property(get_rpedestal, set_rpedestal)

    def get_bpedestal(self):
        return self._bpedestal

    def set_bpedestal(self, value):
        self.command('A9', value, low=1)
        self._bpedestal = value

    bpedestal = property(get_bpedestal, set_bpedestal)

    def get_detail(self):
        return self._detail

    def set_detail(self, value):
        self.command('A5', value)
        self._detail = value

    detail = property(get_detail, set_detail)
