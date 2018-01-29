var l r m = I(day8.txt), {}, 0
l =~ s/\s+\Z//s

for split /\n/, l {
	if /^(\w+) (\w+) (-?\d+) if (\w+) (\S+) (-?\d+)$/ {
		r[$1] = 0 when !r[$1]
		r[$4] = 0 when !r[$4]
		next when !eval(r[$4] + ' ' + $5 + ' ' + $6)
		r[$1] += match $2 {
			'inc' => int($3),
			'dec' => -(int $3)
		}
		m = r[$1] > m ? r[$1] : m
	} else die 'invalid instruction: ' + _
}

pl (reverse sort values r)[0], '\n', m
