var l = int I(day3.txt)
fn abs (x) = x < 0 ? -x : x
var i c = 1, 0
for var i = 1; i * i <= l; i += 2: c++
pl abs(c - ((l - ((i - 2)  * (i - 2))) % (c * 2))) + c
