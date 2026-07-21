| sha1_asm.s -- SHA-1 block compression (FIPS 180-1), hand-written m68k asm.
|
| void sha1_compress_asm(uint32_t state[5], const uint8_t block[64]);
|
| Same algorithm as sha1_compress_c() in sha1.c - kept structurally close to
| the C reference for reviewability. Restricted to instructions/addressing
| modes available on plain 68000 (no 68020-only tricks), so this is safe as
| the default on every CPU this project supports (#47); 68020+ still runs it
| faster purely by being a faster CPU on the same instructions.
|
| Register use: d0-d4 = a,b,c,d,e (the working state); d5 = f; d6 = tmp/k;
| d7 = round counter; a2 = &w[0], walked with (a2)+ during both the W
| expansion and the round loop, since both consume w[] strictly in order;
| a3/a4 = scratch. w[80] (320 bytes) lives on our own stack frame - the same
| stack cost the C version already has as a local array, not a regression.

        .text
        .even
        .globl  _sha1_compress_asm
_sha1_compress_asm:
        movem.l d2-d7/a2-a4,-(sp)
        sub.l   #320,sp

| stack layout from here:
|   0(sp)   = w[0..79] (320 bytes)
|   320(sp) = saved a4/a3/a2/d7/d6/d5/d4/d3/d2 (36 bytes, movem order reversed on stack)
|   356(sp) = return address
|   360(sp) = state (arg1)
|   364(sp) = block (arg2)
        move.l  360(sp),a0              | a0 = state
        move.l  364(sp),a1              | a1 = block
        lea     (sp),a2                 | a2 = &w[0]

| --- w[0..15]: big-endian 32-bit words straight from the block ---
        moveq   #15,d7
wload_loop:
        clr.l   d0
        move.b  (a1)+,d0
        lsl.l   #8,d0
        move.b  (a1)+,d0
        lsl.l   #8,d0
        move.b  (a1)+,d0
        lsl.l   #8,d0
        move.b  (a1)+,d0
        move.l  d0,(a2)+
        dbra    d7,wload_loop

| --- w[16..79] = ROTL(w[i-3]^w[i-8]^w[i-14]^w[i-16], 1) ---
| a2 currently points one past w[15], i.e. at w[16]'s slot.
        moveq   #(80-16-1),d7
wexp_loop:
        move.l  -12(a2),d0              | d0 = w[i-3]
        move.l  -32(a2),d1
        eor.l   d1,d0                   | ^ w[i-8]
        move.l  -56(a2),d1
        eor.l   d1,d0                   | ^ w[i-14]
        move.l  -64(a2),d1
        eor.l   d1,d0                   | ^ w[i-16]
        rol.l   #1,d0
        move.l  d0,(a2)+
        dbra    d7,wexp_loop

| --- the 80-round compression, in the same 4 ranges as the C reference ---
| d0=a d1=b d2=c d3=d d4=e (the working state, register-resident throughout);
| d5/d6 = scratch (f, then tmp); a2 = &w[i], walked via (a2)+ in round order.
        lea     (sp),a2                 | reset to &w[0] for the round loop
        move.l  (a0),d0                 | a = state[0]
        move.l  4(a0),d1                | b = state[1]
        move.l  8(a0),d2                | c = state[2]
        move.l  12(a0),d3               | d = state[3]
        move.l  16(a0),d4               | e = state[4]

| rounds 0-19: f = (b&c)|(~b&d), k = 0x5A827999
        moveq   #19,d7
round1_loop:
        move.l  d1,d6
        and.l   d2,d6                   | d6 = b&c
        move.l  d1,d5
        not.l   d5                      | d5 = ~b
        and.l   d3,d5                   | d5 = ~b&d
        or.l    d5,d6                   | d6 = f
        move.l  d0,d5
        rol.l   #5,d5                   | d5 = ROTL(a,5)
        add.l   d6,d5
        add.l   d4,d5
        add.l   #0x5A827999,d5
        add.l   (a2)+,d5                | d5 = tmp
        move.l  d3,d4                   | e = d
        move.l  d2,d3                   | d = c
        move.l  d1,d6
        ror.l   #2,d6
        move.l  d6,d2                   | c = ROTL(b,30)
        move.l  d0,d1                   | b = a
        move.l  d5,d0                   | a = tmp
        dbra    d7,round1_loop

| rounds 20-39: f = b^c^d, k = 0x6ED9EBA1
        moveq   #19,d7
round2_loop:
        move.l  d1,d6
        eor.l   d2,d6
        eor.l   d3,d6                   | d6 = f
        move.l  d0,d5
        rol.l   #5,d5
        add.l   d6,d5
        add.l   d4,d5
        add.l   #0x6ED9EBA1,d5
        add.l   (a2)+,d5
        move.l  d3,d4
        move.l  d2,d3
        move.l  d1,d6
        ror.l   #2,d6
        move.l  d6,d2
        move.l  d0,d1
        move.l  d5,d0
        dbra    d7,round2_loop

| rounds 40-59: f = (b&c)|(b&d)|(c&d), k = 0x8F1BBCDC
        moveq   #19,d7
round3_loop:
        move.l  d1,d6
        and.l   d2,d6                   | d6 = b&c
        move.l  d1,d5
        and.l   d3,d5                   | d5 = b&d
        or.l    d5,d6
        move.l  d2,d5
        and.l   d3,d5                   | d5 = c&d
        or.l    d5,d6                   | d6 = f
        move.l  d0,d5
        rol.l   #5,d5
        add.l   d6,d5
        add.l   d4,d5
        add.l   #0x8F1BBCDC,d5
        add.l   (a2)+,d5
        move.l  d3,d4
        move.l  d2,d3
        move.l  d1,d6
        ror.l   #2,d6
        move.l  d6,d2
        move.l  d0,d1
        move.l  d5,d0
        dbra    d7,round3_loop

| rounds 60-79: f = b^c^d, k = 0xCA62C1D6
        moveq   #19,d7
round4_loop:
        move.l  d1,d6
        eor.l   d2,d6
        eor.l   d3,d6
        move.l  d0,d5
        rol.l   #5,d5
        add.l   d6,d5
        add.l   d4,d5
        add.l   #0xCA62C1D6,d5
        add.l   (a2)+,d5
        move.l  d3,d4
        move.l  d2,d3
        move.l  d1,d6
        ror.l   #2,d6
        move.l  d6,d2
        move.l  d0,d1
        move.l  d5,d0
        dbra    d7,round4_loop

| state[i] += (a,b,c,d,e)
        add.l   d0,(a0)
        add.l   d1,4(a0)
        add.l   d2,8(a0)
        add.l   d3,12(a0)
        add.l   d4,16(a0)

        add.l   #320,sp
        movem.l (sp)+,d2-d7/a2-a4
        rts
