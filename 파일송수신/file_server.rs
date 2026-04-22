use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::fs::File;
use std::path::Path;

fn handle_client(mut stream: TcpStream) {
    println!("클라이언트 연결됨: {}", stream.peer_addr().unwrap());

    // 1. 파일 이름 길이 수신 (4바이트)
    let mut name_len_buf = [0u8; 4];
    stream.read_exact(&mut name_len_buf).unwrap();
    let name_len = u32::from_be_bytes(name_len_buf) as usize;

    // 2. 파일 이름 수신
    let mut name_buf = vec![0u8; name_len];
    stream.read_exact(&mut name_buf).unwrap();
    let file_name = String::from_utf8(name_buf).unwrap();
    println!("받을 파일: {}", file_name);

    // 3. 파일 데이터 수신 및 저장
    let path = Path::new(&file_name);
    let mut file = File::create(path).unwrap();
    let mut buffer = [0u8; 1024]; // 1KB 버퍼

    loop {
        let n = stream.read(&mut buffer).unwrap();
        if n == 0 { break; } // 연결 종료
        file.write_all(&buffer[..n]).unwrap();
    }
    println!("파일 수신 완료: {}", file_name);
}

fn main() {
    let ip_port = std::env::var("IP_PORT").unwrap_or_else(|_| "10.98.163.93:9222".to_string());
    let listener = TcpListener::bind(&ip_port).unwrap();
    println!("서버 대기 중... {}", ip_port);

    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                handle_client(stream);
            }
            Err(e) => println!("연결 실패: {}", e),
        }
    }
}
