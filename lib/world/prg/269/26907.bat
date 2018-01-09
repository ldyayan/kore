>death_prog 100~
  mpecho The bat desperately clings at the ceiling to try to avoid your final blow but is unsuccessful!
  mpecho As you strike the bat there's a horrible sound as its huge wing rips out from its side and hangs there, still clinging from the ceiling.
  mpecho The wing swings back and forth and falls to the ground with a heavy thud as the claw comes loose.
  mpasound Your blood freezes as you hear a loud, screeching death cry from somewhere nearby.
  if rand(10)
    mpoload 26918
  else
    mpoload 26917
  endif
  mppose $i stand
  mpsilent drop wing
  mppose $i dead
~
|
