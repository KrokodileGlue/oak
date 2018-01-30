var l r m = I(day8.txt), {}, 0

for split /\n/, l {
	next when !/^(\w+) (\w+) (-?\d+) if (\w+) (\S+) (-?\d+)$/
	r[$1] = 0 when !r[$1]; r[$4] = 0 when !r[$4]
	next when !eval(r[$4] + ' ' + $5 + ' ' + $6)
	r[$1] += $2 == 'inc' ? int($3) : -int($3)
	m = r[$1] > m ? r[$1] : m
}

pl max(r), '\n', m
