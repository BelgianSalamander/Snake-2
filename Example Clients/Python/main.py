from snakeclient import *

class MyClient(Client):
    def select_move(self) -> Move:
        return UP


if __name__ == '__main__':
    MyClient("Python", 0, 255, 0).run("localhost", 42069)
