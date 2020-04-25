# Emulating the Intel 8080
http://emulator101.com/

## Build

```bash
make
```

## Run

```bash
./intel8080 invaders/invaders
```

## Notes

Need to check what the Parity flag is actually for. It seems that [this reference JavaScript implementation](https://bluishcoder.co.nz/js8080/) sets the flag based on the _value_ of the result rather than the number of bits set.

At line 287 of `js8080.js`:

```js
if (x % 2)
  this.f &= ~PARITY & 0xFF;
else
  this.f |= PARITY;
```

## References
* [8080 opcodes](http://www.emulator101.com/reference/8080-by-opcode.html)
* [8080 assembly programming manual](http://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf)

