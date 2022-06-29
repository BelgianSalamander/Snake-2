use std::io::{Write, Read};
use std::net::*;
use std::boxed::Box;

pub type SnakeId = u32;

#[derive(Copy, Debug, Clone, PartialEq)]
pub enum Square {
    Empty,
    Food,
    Snake(u32)
}

impl Square {
    pub fn is_food(&self) -> bool {
        match *self {
            Square::Food => true,
            _ => false
        }
    }

    pub fn is_snake(&self) -> bool {
        match *self {
            Square::Snake(_) => true,
            _ => false
        }
    }

    pub fn is_empty(&self) -> bool {
        match *self {
            Square::Empty => true,
            _ => false
        }
    }

    pub fn can_move_to(&self) -> bool {
        match *self {
            Square::Empty => true,
            _ => false
        }
    }
}

#[derive(Copy, Debug, Clone, PartialEq)]
pub enum Move {
    Up, Right, Down, Left
}

impl Move {
    fn get_ordinal(&self) -> u32 {
        match self {
            &Move::Up => 0,
            &Move::Right => 1,
            &Move::Down => 2,
            &Move::Left => 3
        }
    }

    pub fn get_shift(&self) -> (i32, i32) {
        match self {
            &Move::Up => (0, -1),
            &Move::Right => (1, 0),
            &Move::Down => (0, 1),
            &Move::Left => (-1, 0)
        }
    }
}

#[derive(Copy, Debug, Clone, PartialEq)]
pub struct Pos {
    pub r: u32,
    pub c: u32,
}

impl Pos {
    pub fn new(r: u32, c: u32) -> Pos {
        Pos { r: r, c: c }
    }

    pub fn shift(&self, move_: &Move) -> Pos {
        let (r_shift, c_shift) = move_.get_shift();
        Pos { r: (self.r as i32 + r_shift) as u32, c: (self.c as i32 + c_shift) as u32 }
    }
}

pub struct GameState {
    pub id: SnakeId,
    pub board: Vec<Vec<Square>>,
    pub num_rows: u32,
    pub num_cols: u32,
    pub current_turn: u32,
    pub head: Pos
}

impl GameState {
    fn new() -> GameState {
        GameState {
            id: 0,
            board: vec![],
            num_rows: 0,
            num_cols: 0,
            current_turn: 0,
            head: Pos::new(0, 0)
        }
    }

    pub fn get_square(&self, pos: Pos) -> Square {
        self.board[pos.r as usize][pos.c as usize]
    }

    fn set_square(&mut self, pos: Pos, square: Square) -> Square{
        let old_square = self.get_square(pos);
        self.board[pos.r as usize][pos.c as usize] = square;
        old_square
    }
}

pub trait SnakeBrain {
    fn get_move(&mut self, state: &GameState) -> Move;

    fn on_game_start(&mut self, state: &GameState) {

    }

    fn on_square_change(&mut self, state: &GameState, pos: Pos, old_square: Square, new_square: Square) {
        
    }

    fn on_death(&mut self, reason: String) {
        println!("You died: {}", reason);
    }

    fn process_game_results(&mut self, died: bool, length: u32, score: i32, died_on: u32, rank: u32, num_ties: u32, new_elo: i32) {
        if died {
            println!("You died on turn {}", died_on);
        }
        println!("Your length was {}", length);
        println!("Your score was {}", score);
        println!("Your rank was {}", rank);
        println!("You tied with {} other snakes", num_ties);
        println!("Your new ELO rating is {}", new_elo);
    
    }
}

enum ClientBoundPacketType {
    ConnectionEstablished = 0,
    MoveRequest = 1,
    GameChanges = 2,
    GameStart = 3,
    WholeGrid = 4,
    SnakeDeath = 5,
    GameResults = 6
}

enum ServerBoundPacketType {
    NameAndColor = 0,
    MoveResponse = 1
}

pub struct SnakeClient {
    state: GameState,
    name: String,
    color: (u8, u8, u8),
    brain: Box<dyn SnakeBrain>,
    connected: bool
}

fn push_packet_header(v: &mut Vec<u8>, packet_type: ServerBoundPacketType, length: u16) {
    v.push((length & 0xff) as u8);
    v.push(((length << 8) & 0xff) as u8);
    v.push(packet_type as u8);
    v.push(0);
}

struct TcpHelper {
    data: Vec<u8>,
    idx: usize
}

impl TcpHelper {
    fn new(data: Vec<u8>) -> TcpHelper {
        TcpHelper { data: data, idx: 0 }
    }

    fn read_u8(&mut self) -> u8 {
        let ret = self.data[self.idx];
        self.idx += 1;
        ret
    }

    fn read_u16(&mut self) -> u16 {
        let ret = (self.read_u8() as u16) | ((self.read_u8() as u16) << 8);
        ret
    }

    fn read_u32(&mut self) -> u32 {
        let ret = (self.read_u16() as u32) | ((self.read_u16() as u32) << 16);
        ret
    }

    fn read_i8(&mut self) -> i8 {
        let ret = self.read_u8() as i8;
        ret
    }

    fn read_i16(&mut self) -> i16 {
        let ret = self.read_u16() as i16;
        ret
    }

    fn read_i32(&mut self) -> i32 {
        let ret = self.read_u32() as i32;
        ret
    }

    fn read_bool(&mut self) -> bool {
        let ret = self.read_u8() != 0;
        ret
    }

    fn read_pos(&mut self) -> Pos {
        Pos::new(self.read_u32(), self.read_u32())
    }

    fn read_square(&mut self) -> Square {
        let square_type = self.read_u8();
        match square_type {
            0 => Square::Empty,
            1 => Square::Food,
            2 => Square::Snake(self.read_u32()),
            _ => panic!("Invalid square type {}", square_type)
        }
    }

    fn read_square_uncompact(&mut self) -> Square {
        let square_type = self.read_u8();
        let snake_id = self.read_u32();
        match square_type {
            0 => Square::Empty,
            1 => Square::Food,
            2 => Square::Snake(snake_id),
            _ => panic!("Invalid square type {}", square_type)
        }
    }

    fn skip(&mut self, n: usize) {
        self.idx += n;
    }

    fn is_end(&self) -> bool {
        self.idx >= self.data.len()
    }
}

impl SnakeClient {
    pub fn new(name: String, color: (u8, u8, u8), brain: Box<dyn SnakeBrain>) -> SnakeClient {
        if name.len() != name.bytes().len() {
            panic!("Name must be a valid ASCII string");
        }

        SnakeClient {
            state: GameState::new(),
            name: name,
            color: color,
            brain: brain,
            connected: false
        }
    }

    pub fn run(&mut self, address: SocketAddr) {
        let mut socket = TcpStream::connect(address).unwrap();
        self.send_name_and_color(&mut socket);

        loop {
            let mut header_buffer = [0; 4];
            socket.read_exact(&mut header_buffer).unwrap();

            let length = ((header_buffer[1] as u16) << 8) | (header_buffer[0] as u16);
            let packet_type = header_buffer[2] as u8;

            let mut packet_buffer = vec![0; length as usize];

            socket.read_exact(&mut packet_buffer).unwrap();

            let packet_type: ClientBoundPacketType = match packet_type {
                0 => ClientBoundPacketType::ConnectionEstablished,
                1 => ClientBoundPacketType::MoveRequest,
                2 => ClientBoundPacketType::GameChanges,
                3 => ClientBoundPacketType::GameStart,
                4 => ClientBoundPacketType::WholeGrid,
                5 => ClientBoundPacketType::SnakeDeath,
                6 => ClientBoundPacketType::GameResults,
                _ => {
                    println!("Unknown packet type: {}", packet_type);
                    break;
                }
            };

            match packet_type {
                ClientBoundPacketType::ConnectionEstablished => {
                    if length != 0 {
                        panic!("Invalid packet length");
                    } else if self.connected {
                        panic!("Already connected");
                    } else {
                        self.connected = true;
                        println!("Connected!");
                    }
                }

                ClientBoundPacketType::MoveRequest => {
                    if !self.connected {
                        panic!("Not connected!");
                    }

                    if length != 0 {
                        panic!("Invalid packet length");
                    } else {
                        let selected_move = self.brain.get_move(&mut self.state);
                        self.send_move(&mut socket, selected_move);
                    }
                }

                ClientBoundPacketType::GameChanges => {
                    if !self.connected {
                        panic!("Not connected!");
                    }

                    if length < 12 {
                        panic!("Invalid packet length");
                    } else {
                        let mut helper = TcpHelper::new(packet_buffer);

                        self.state.head = helper.read_pos();
                        self.state.current_turn = helper.read_u32();

                        while !helper.is_end() {
                            let pos = helper.read_pos();
                            let square = helper.read_square();

                            let old = self.state.set_square(pos, square);
                            self.brain.on_square_change(&self.state, pos, old, square);
                        }
                    }
                }

                ClientBoundPacketType::GameStart => {
                    if !self.connected {
                        panic!("Not connected!");
                    }

                    if length != 12 {
                        panic!("Invalid packet length");
                    } else {
                        let mut helper = TcpHelper::new(packet_buffer);

                        self.state.num_rows = helper.read_u32();
                        self.state.num_cols = helper.read_u32();
                        self.state.id = helper.read_u32();

                        self.state.board = vec![];

                        for _ in 0..self.state.num_rows {
                            self.state.board.push(vec![Square::Empty; self.state.num_cols as usize]);
                        }

                        self.brain.on_game_start(&self.state);
                    }
                }

                ClientBoundPacketType::WholeGrid => {
                    //This packet is used for debugging to check wether your grid is in sync. 
                    if !self.connected {
                        panic!("Not connected!");
                    }

                    let mut helper = TcpHelper::new(packet_buffer);

                    for row in 0..self.state.num_rows {
                        for col in 0..self.state.num_cols {
                            let pos = Pos::new(row, col);
                            let square = helper.read_square_uncompact();

                            if square != self.state.get_square(pos) {
                                panic!("Grid out of sync");
                            }
                        }
                    }
                }

                ClientBoundPacketType::SnakeDeath => {
                    if !self.connected {
                        panic!("Not connected!");
                    }

                    //Body is a string with a reason for death
                    self.brain.on_death(String::from_utf8(packet_buffer).unwrap());
                }

                ClientBoundPacketType::GameResults => {
                    if !self.connected {
                        panic!("Not connected!");
                    }

                    let mut helper = TcpHelper::new(packet_buffer);

                    let died = helper.read_bool();
                    let length = helper.read_u32();
                    let score = helper.read_i32();
                    let died_on = helper.read_u32();
                    let rank = helper.read_u32();
                    let num_ties = helper.read_u32();
                    let new_elo = helper.read_i32();

                    self.brain.process_game_results(died, length, score, died_on, rank, num_ties, new_elo)
                }
            }
        }
    }

    fn send_name_and_color(&mut self, socket: &mut TcpStream) {
        let mut packet = vec![];
        push_packet_header(&mut packet, ServerBoundPacketType::NameAndColor, self.name.len() as u16 + 3);
        packet.push(self.color.0);
        packet.push(self.color.1);
        packet.push(self.color.2);
        packet.extend(self.name.bytes());
        socket.write(&packet).unwrap();
    }

    fn send_move(&mut self, socket: &mut TcpStream, move_: Move) {
        let mut packet = vec![];
        push_packet_header(&mut packet, ServerBoundPacketType::MoveResponse, 1);
        packet.push(move_.get_ordinal() as u8);
        socket.write(&packet).unwrap();
    }
}