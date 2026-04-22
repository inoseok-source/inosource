use std::io::{Read, Write};
use std::net::TcpStream;
use std::fs::File;
use std::path::Path;
use std::env;

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        println!("사용법: cargo run --bin client <파일경로>");
        return;
    }
    let file_path = &args[1];
    let ip_port = std::env::var("IP_PORT").unwrap_or_else(|_| "10.98.163.93:9222".to_string());

    // 1. 파일 열기
    let mut file = File::open(file_path).unwrap();
    let file_name = Path::new(file_path).file_name().unwrap().to_str().unwrap();

    // 2. 서버 연결
    let mut stream = TcpStream::connect(&ip_port).unwrap();
    println!("서버에 연결됨. 파일 전송 시작: {}", file_name);

    // 3. 파일 이름 전송 (길이 + 이름)
    let name_bytes = file_name.as_bytes();
    let name_len = name_bytes.len() as u32;
    stream.write_all(&name_len.to_be_bytes()).unwrap();
    stream.write_all(name_bytes).unwrap();

    // 4. 파일 데이터 전송
    let mut buffer = [0u8; 1024];
    loop {
        let n = file.read(&mut buffer).unwrap();
        if n == 0 { break; }
        stream.write_all(&buffer[..n]).unwrap();
    }
    println!("파일 전송 완료.");
}
