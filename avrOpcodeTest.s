        ldi R19, 95
        ldi R20, 2
        out 62, R20
        out 61, R19
start:  rjmp begin
fail:   break
begin:  ldi R16, 255 ; Test add
        ldi R17, 1
        ldi R18, 8
        add R16, R17
        rcall status
        adc R18, R17
        subi R18, 10
        breq end
        rjmp fail
end:    rjmp end
status: breq zcorr
        rjmp fail
zcorr:  brcs ccorr
        rjmp fail
ccorr:  brhs hcorr
        rjmp fail
hcorr:  brge scorr
        rjmp fail
scorr:  brpl ncorr
        rjmp fail
ncorr:  brvc vcorr
        rjmp fail
vcorr:  ret
