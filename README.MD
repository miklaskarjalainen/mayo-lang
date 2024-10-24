## Mayo!
Mayo is a statically typed and compiled programming language. Mayo is very much WIP, and written as a hobby project.

### Examples

Mayo in its current state looks like this.
```rust
extern fn printf(fmt: char*, ...) -> i32;
fn main() -> i32 {
  let x: i32 = 0;
  while x < 100 {
    x = x + 1;
    
    let print_num: bool = true;
    if x % 3 == 0 {
      printf("Fizz");
      print_num = false;
    }
    if x % 5 == 0 {
      printf("Buzz");
      print_num = false;
    }
    if print_num == true {
      printf("%i", x);
    }
    printf("\n");
  }
  return 0;
}

```

### Technologies
Written in plain old C. Mayo uses [QBE](https://c9x.me/compile/) as its backend. 

