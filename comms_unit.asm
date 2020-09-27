.comment
    This file is part of PadSwitcher64.
	
    PadSwitcher64 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
	
    PadSwitcher64 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
	
    You should have received a copy of the GNU General Public License
    along with PadSwitcher64.  If not, see <https://www.gnu.org/licenses/>.
	
	
	Communications portion of the PadSwitcher64 interface application
	To be assemebled without load address header and converted to BASIC DATA statements
	...
	Because the PadSwitcher64 software is written in compiled BASIC and I'm too lazy to
	re-write the entire thing in assembly...
	
	Written for 64tass.

.endc

	OP=56320
	IP=56321

*       = $c000
input	.fill 1
result	.fill 1
jmp communicate
handshake
	sei
	lda #239
	sta 56322
	lda #0
	sta 56323
	lda #240
	sta OP
handshakeloop
	lda OP
	and #16
	bne notfire
	sta result
	cli
	rts
notfire	
	lda IP
	and #31
	bne handshakeloop
	jsr nopalicious
	lda #255
	sta 56322
	lda #1
	sta OP
	ldx #255
	ldy #255
hs1
	lda IP
	and #31
	bne ontohs2
	dex
	bne hs1
	dex
	dey
	bne hs1
	lda #128
	sta result
	cli
	lda #1
	sta 53280
	rts
ontohs2
	jsr nopalicious
	lda #4
	sta op
	ldx #255
	ldy #255
hs2
	lda IP
	and #31
	beq hs3
	dex
	bne hs2
	dex
	dey
	bne hs2
	lda #128
	sta result
	cli
	lda #2
	sta 53280
	rts
hs3
	lda #0
	sta 53280
	jsr nopalicious
	sta op
	lda #1
	sta result
	cli
	rts
	
communicate
	sei
	jsr nopalicious
	lda input
	sta op
	ldx #255
	ldy #255
commloop
	lda IP
	;and #31
	sta result
	and #16
	beq commdone
	dex
	bne commloop
	dex
	dey
	bne commloop
	lda #128
	sta result
	cli
	lda #3
	sta 53280
	rts
commdone
	lda result
	eor #255
	and #15
	sta result
	;jsr nopalicious
	ldx #255
mininop
	dex
	bne mininop
	lda #0
	sta op
	ldx #255
	ldy #255
ackloop
	lda IP
	and #16
	bne ackdone
	dex
	bne ackloop
	dex
	dey
	bne ackloop
	lda #128
	sta result
	lda #4
	sta 53280
	cli
	rts
ackdone

	cli
	rts
nopalicious
	ldx #255
	ldy #5
noploop
	dex
	bne noploop
	dex
	dey
	bne noploop
	rts




