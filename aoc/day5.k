var l = I(day5.txt) =~ /-?\d+/gs
var f i s = map { int } l, 0, 0
while i < length f { f[i] += 1; i += f[i] - 1; s++ }
pl s

f = map { int } l
i = 0, s = 0

while i < length f and i >= 0 {
	i += f[i] > 2 ? f[i] -= 1 : f[i] += 1
	s++
}

pl s
