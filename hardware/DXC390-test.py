import DXC390
from time import sleep
from random import randint

if __name__ == '__main__':
    cam = DXC390.DXC390('/dev/tty.usbserial-FTXEKIR7D')

    cam.connect()

    for i in range(3):
        cam.iris = randint(0, 255)
        cam.zoom = randint(0, 255)
        cam.focus = randint(0, 255)
        sleep(1)

    cam.disconnect()
