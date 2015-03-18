import serial


class DXC390(object):
    def __init__(self, port='/dev/tty.usbserial', iris=0, focus=0, zoom=0):
        self._iris = iris
        self._focus = focus
        self._zoom = zoom
        self._port = port
        self._ser = serial.Serial(self._port, timeout=1)

    def command(self, prefix, value, low=0, high=255):
        # TODO: check if prefix is 2 chars and 0<value<255
        xmit = "{prefix}{value:02X}\r".format(prefix=prefix, value=value)
        self._ser.write(xmit)
        recv = self._ser.read(5)
        if recv != xmit:
            raise Exception("return code ({}) doesn't match command({})".format(recv, xmit))

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
