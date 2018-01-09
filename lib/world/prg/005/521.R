>rand_prog 6~
if inroom($r) == here
  if hitprcnt($r) < 100
    mpcallmagic 'cure critical' $r
  endif
endif
~
|
