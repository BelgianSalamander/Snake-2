from dataclasses import dataclass
from typing import Tuple
from abc import ABC, abstractmethod

import socket, struct

@dataclass
class Move:
    index: int
    shift: Tuple[int, int]

    def apply(self, row: int, col: int) -> Tuple[int, int]:
        return row + self.shift[0], col + self.shift[1]



UP = Move(0, (-1, 0))
RIGHT = Move(1, (0, 1))
DOWN = Move(2, (1, 0))
LEFT = Move(3, (0, -1))

MOVES = [
    UP,
    RIGHT,
    DOWN,
    LEFT
]

## Squares
EMPTY = 0
FOOD = 1
SNAKE = 2

@dataclass
class Square:
    square_type: int
    snake_id: int

    def __eq__(self, other) -> bool:
        if type(other) != type(self):
            return False

        if self.square_type != other.square_type:
            return False

        if self.square_type == SNAKE:
            return self.snake_id == other.snake_id

        return True

    def can_move_to(self) -> bool:
        return self.square_type == EMPTY or self.square_type == FOOD

    def is_empty(self):
        return self.square_type == EMPTY

    def is_food(self):
        return self.square_type == FOOD

    def is_snake(self):
        return self.square_type == SNAKE


## Inward bound packets
CONNECTION_ESTABLISHED = 0
MOVE_REQUEST = 1
GAME_CHANGES = 2
GAME_START = 3
WHOLE_GRID = 4
SNAKE_DEATH = 5
GAME_RESULTS = 6

INWARD = ["Connection Established", "Move Request", "Game Changes", "Game Start", "Whole Grid", "Snake Death", "Game Results"]

## Outward bound packets
NAME_AND_COLOR = 0
MOVE_RESPONSE = 1

def packet_header(type_: int, length: int) -> bytes:
    return bytes([
        length & 0xff,
        (length << 8) & 0xff,
        type_,
        0
    ])

class Client(ABC):
    def __init__(self, name: str, r: int, g: int, b:int):
        self.name = name
        self.r = r
        self.g = g
        self.b = b

        if len(name) != len(bytes(name, "utf-8")):
            raise ValueError("String must have only ascii characters")

    def run(self, ip: str, port: int):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((ip, port))

        #Send name and color
        packet = packet_header(NAME_AND_COLOR, 3 + len(self.name)) + \
                 bytes([self.r, self.g, self.b]) + \
                 bytes(self.name, "utf-8")

        self.sock.sendall(packet)

        #Await confirmation
        confirmation = self.wait_for_packet()

        if confirmation[0] != CONNECTION_ESTABLISHED:
            raise Exception("Connection failed")

        while True:
            self.handle(*self.wait_for_packet())



    def wait_for_packet(self):
        header = self.sock.recv(4)

        if len(header) != 4:
            raise ValueError("Didn't receive full header")

        packet_length: int = header[0] | (header[1] << 8)
        packet_type: int = header[2]

        data = b''

        while len(data) < packet_length:
            data += self.sock.recv(packet_length - len(data))

        #print(f"Received {INWARD[packet_type]}")

        return packet_type, data

    @abstractmethod
    def select_move(self) -> Move:
        pass

    def handle(self, packet_type, packet_data):
        if packet_type == MOVE_REQUEST:
            move = self.select_move()
            response = packet_header(MOVE_RESPONSE, 1) + bytes([move.index])
            self.sock.sendall(response)
        elif packet_type == GAME_START:
            self.num_rows, self.num_cols, self.ID = struct.unpack("<III", packet_data)

            self.board = [[Square(EMPTY, 0) for i in range(self.num_cols)] for j in range(self.num_rows)]

            self.on_game_start()
        elif packet_type == GAME_CHANGES:
            self.head_row, self.head_col, self.currTurn = struct.unpack("<III", packet_data[:12])
            packet_data = packet_data[12:]

            while len(packet_data) != 0:
                row, col, square_type = struct.unpack("<IIB", packet_data[:9])

                old = self.board[row][col]
                if square_type == SNAKE:
                    snake_ID, = struct.unpack("<I", packet_data[9: 13])
                    self.board[row][col] = Square(SNAKE, snake_ID)
                    packet_data = packet_data[13:]
                else:
                    self.board[row][col] = Square(square_type, 0)
                    packet_data = packet_data[9:]

                self.on_update_square(row, col, old, self.board[row][col])
        elif packet_type == WHOLE_GRID:
            for r in range(self.num_rows):
                for c in range(self.num_cols):
                    square_type, snake_ID = struct.unpack("<BI", packet_data[:5])
                    packet_data = packet_data[5:]
                    square = Square(square_type, snake_ID)
                    if square != self.board[r][c]:
                        print(f"Square mismatch at {r}, {c}")
        elif packet_type == SNAKE_DEATH:
            # The body of the packet is an ASCII string with a reason
            reason = packet_data.decode("ascii")
            self.on_death(reason)
        elif packet_type == GAME_RESULTS:
            self.process_game_results(*struct.unpack("<?IiIIIi", packet_data))

    def on_game_start(self):
        pass

    def on_update_square(self, row: int, col: int, old_square: Square, new_square: Square):
        pass

    def on_death(self, reason: str):
        print("Snake died!", reason)

    def process_game_results(self, died: bool, length: int, score: int, died_on: int, rank: int, num_ties: int, new_elo: int):
        print("Game over!")
        print(f"  Length: {length}")
        print(f"  Score: {score}")
        print(f"  Rank: {rank}")
        print(f"  Num ties: {num_ties}")
        print(f"  New ELO: {new_elo}")
        if died:
            print(f"  Died on: {died_on}")