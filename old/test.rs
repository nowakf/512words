use std::io::{stdin, Read};

fn main() -> std::io::Result<()> {
    let mut buffer = vec![];
    stdin().read_to_end(&mut buffer)?;
    let mut ptr : &[u8] = &buffer;
    loop {
        match std::str::from_utf8(ptr) {
            Err(error) => {
                let (_, after_valid) = ptr.split_at(error.valid_up_to());
                if let Some(invalid_len) = error.error_len() {
                    println!("{} {}", error.valid_up_to(), invalid_len);
                    ptr = &after_valid[invalid_len..];
                } else {
                    break;
                }
            },
           Ok(_) => break,
        }
    }
    Ok(())
}
