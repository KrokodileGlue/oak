var l s step dist m a = map { int } I(day6.txt) =~ /-?\d+/gs, [], 0, 0, 0, 0

while true {
	step++, push s, l
	for 0 -> l: m = iff l[] > l[m], a = l[m] - 1; l[m] = 0
	for 0 -> a: l[(m + 1 +) % length l]++
	for 0 -> s: if l == s[]: dist = step -
	last when dist
}

pl step
pl dist
