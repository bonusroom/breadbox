import DXC390
from time import sleep
from random import randint

if __name__ == '__main__':
    cam = DXC390.DXC390('/dev/tty.usbserial-FTXEKIR7D')

    cam.connect()

    for i in range(10):
    #while True:
        cam.iris = randint(100, 200)
        cam.zoom = randint(0, 80)
        cam.focus = randint(0, 255)
        cam.pedestal = randint(80, 120)
        cam.rpedestal = randint(1, 255)
        cam.bpedestal = randint(1, 255)
        cam.detail = randint(100, 255)
        sleep(1)

    cam.disconnect()
