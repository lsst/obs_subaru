proc bitset {val} {
  if ($val&1) {return 1}
  if ($val&2) {return 2}
  if ($val&4) {return 3}
  if ($val&8) {return 4}
  if ($val&16) {return 5}
  if ($val&32) {return 6}
  if ($val&64) {return 7}
  if ($val&128) {return 8}
  return 0
}

proc specialbitset {val} {
  if ($val&1) {
    if ($val&2) {
      if ($val&4) { return 7} \
      else {return 2}
    } else {
      if ($val&4) {return 6} \
      else {return 1}
    }
  } else {
    if ($val&2) {
      if ($val&4) {return 4} \
      else {return 3}
    } else {
      if ($val&4) {return 5} \
      else {return 0}
    }
  }
}


proc mybitset { val } {

  if ($val&128) {
    return {1}
  }
  return { 0 }
}
