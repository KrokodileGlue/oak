var l = I(day5.txt) =~ /-?\d+/g
var f i s = map { int } l, 0, 0

for i = 0; i < length f; s++: i += f[i]++
pl s

f = map { int } l, i = 0

for s = 0; i < length f and i >= 0; s++:
	i += f[i] > 2 ? f[i]-- : f[i]++
pl s
