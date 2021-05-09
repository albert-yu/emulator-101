# Emulating the Intel 8080

[Emulator 101](http://emulator101.com/)

## Build

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

### Memory Map

```plain
Space Invaders, (C) Taito 1978, Midway 1979    

CPU: Intel 8080 @ 2MHz (CPU similar to the (newer) Zilog Z80)    

Interrupts: $cf (RST 8) at the start of vblank, $d7 (RST $10) at the end of vblank.    

Video: 256(x)*224(y) @ 60Hz, vertical monitor. Colours are simulated with a    
plastic transparent overlay and a background picture.    
Video hardware is very simple: 7168 bytes 1bpp bitmap (32 bytes per scanline).    

Sound: SN76477 and samples.    

Memory map:    
 ROM    
 $0000-$07ff:    invaders.h    
 $0800-$0fff:    invaders.g    
 $1000-$17ff:    invaders.f    
 $1800-$1fff:    invaders.e    

 RAM    
 $2000-$23ff:    work RAM    
 $2400-$3fff:    video RAM    

 $4000-:     RAM mirror   
```

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

### Interrupts

Another difference found is at instruction 42434, the js emulator processes an interrupt. I have not yet emulated this.

## References

* [8080 opcodes](http://www.emulator101.com/reference/8080-by-opcode.html)
* [8080 assembly programming manual](http://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf)
