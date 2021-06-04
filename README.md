# Emulating the Intel 8080

[Emulator 101](http://emulator101.com/)

## Build

This depends on SDL2 for rendering graphics. So, on macOS with Homebrew:

```bash
brew install sdl2
```

Then, to build:

```bash
make
```

Or with another C compiler:

```bash
CC=clang make
```

With debug symbols:

```bash
make clean && make debug
```

## Run

```bash
./intel8080 invaders/invaders
```

## Notes

### Parity

Need to check what the Parity flag is actually for. It seems that [this reference JavaScript implementation](https://bluishcoder.co.nz/js8080/) sets the flag based on the _value_ of the result rather than the number of bits set.

At line 287 of `js8080.js`:

```js
if (x % 2)
  this.f &= ~PARITY & 0xFF;
else
  this.f |= PARITY;
```

From the manual:

> If the modulo 2 sum of the bits of the result of the operation is 0, (ie., if the result has even parity), this flag is set; otherwise it is reset (ie., if the result has odd parity).

## References

* [Computer Archeology](http://computerarcheology.com/Arcade/SpaceInvaders/Hardware.html)
* [8080 opcodes](http://www.emulator101.com/reference/8080-by-opcode.html)
* [8080 assembly programming manual](http://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf)
