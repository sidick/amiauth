| chacha20_asm.s -- ChaCha20 block function (RFC 8439), hand-written m68k asm.
|
| void chacha20_block_asm(const uint32_t in[16], uint8_t out[64]);
|
| Same algorithm as chacha20_block_c() in chacha20.c. Restricted to
| instructions/addressing modes available on plain 68000 (no 68020-only
| tricks), so this is safe as the default on every CPU this project
| supports (#47); 68020+ still runs it faster purely by being a faster CPU
| on the same instructions.
|
| ROTL32(x,16) is done with SWAP (exact, one instruction). ROTL32(x,12) is
| done as two rotates (#8 then #4), since 68000's rotate-immediate only
| encodes 1-8 - rotate composition is associative, so this is exact, not an
| approximation. x[16] (the 16-word working state) lives on our own stack
| frame, since it doesn't fit in registers alongside the round counter and
| pointers - the same stack cost the C version already has as a local array.

        .macro qround off_a off_b off_c off_d
        move.l  \off_a(a2),d0
        move.l  \off_b(a2),d1
        move.l  \off_c(a2),d2
        move.l  \off_d(a2),d3

        add.l   d1,d0
        eor.l   d0,d3
        swap    d3                      | d = ROTL(d,16)

        add.l   d3,d2
        eor.l   d2,d1
        rol.l   #8,d1
        rol.l   #4,d1                   | b = ROTL(b,12) = ROTL(ROTL(b,8),4)

        add.l   d1,d0
        eor.l   d0,d3
        rol.l   #8,d3                   | d = ROTL(d,8)

        add.l   d3,d2
        eor.l   d2,d1
        rol.l   #7,d1                   | b = ROTL(b,7)

        move.l  d0,\off_a(a2)
        move.l  d1,\off_b(a2)
        move.l  d2,\off_c(a2)
        move.l  d3,\off_d(a2)
        .endm

        .text
        .even
        .globl  _chacha20_block_asm
_chacha20_block_asm:
        movem.l d2-d3/d7/a2,-(sp)
        sub.l   #64,sp                  | x[16] scratch buffer

| stack layout: 0(sp)=x[0..15] (64 bytes); 64(sp)=saved a2/d7/d3/d2 (16 bytes);
| 80(sp)=return address; 84(sp)=in (arg1); 88(sp)=out (arg2)
        move.l  84(sp),a0               | a0 = in
        move.l  88(sp),a1               | a1 = out
        lea     (sp),a2                 | a2 = &x[0]

| --- x[0..15] = in[0..15] ---
        moveq   #15,d7
xcopy_loop:
        move.l  (a0)+,d0
        move.l  d0,(a2)+
        dbra    d7,xcopy_loop
        lea     (sp),a2                 | reset a2 = &x[0] for the round loop
        move.l  84(sp),a0               | reset a0 = &in[0] for the final combine

| --- 10 iterations of 8 quarter-rounds (= 20 ChaCha rounds) ---
        moveq   #9,d7
round_loop:
        qround  0,16,32,48              | x[0],x[4],x[8],x[12]
        qround  4,20,36,52              | x[1],x[5],x[9],x[13]
        qround  8,24,40,56              | x[2],x[6],x[10],x[14]
        qround  12,28,44,60             | x[3],x[7],x[11],x[15]
        qround  0,20,40,60              | x[0],x[5],x[10],x[15]
        qround  4,24,44,48              | x[1],x[6],x[11],x[12]
        qround  8,28,32,52              | x[2],x[7],x[8],x[13]
        qround  12,16,36,56             | x[3],x[4],x[9],x[14]
        dbra    d7,round_loop

| --- out[i] = little-endian bytes of (x[i] + in[i]) ---
        moveq   #15,d7
combine_loop:
        move.l  (a2)+,d0
        add.l   (a0)+,d0
        move.b  d0,(a1)+                | byte 0 (low)
        lsr.l   #8,d0
        move.b  d0,(a1)+                | byte 1
        lsr.l   #8,d0
        move.b  d0,(a1)+                | byte 2
        lsr.l   #8,d0
        move.b  d0,(a1)+                | byte 3 (high)
        dbra    d7,combine_loop

        add.l   #64,sp
        movem.l (sp)+,d2-d3/d7/a2
        rts
