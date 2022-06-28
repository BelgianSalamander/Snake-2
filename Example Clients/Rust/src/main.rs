use snake_client::SnakeBrain;

mod snake_client;

use snake_client::*;
use std::net::{SocketAddr, IpAddr, Ipv4Addr};
use core::str::FromStr;

struct MySnake {

}

impl SnakeBrain for MySnake {
    fn get_move(&mut self, state: &GameState) -> Move {
        Move::Up
    }
}

fn main() {
    SnakeClient::new(
        "Rust".to_string(),
        (0, 0, 255),
        Box::new(MySnake {}),
    ).run(
        SocketAddr::new(
            IpAddr::V4(Ipv4Addr::from_str("127.0.0.1").unwrap()),
            42069
        )
    );
}
