var l = I(day2.txt)
l =~ s/\s+\Z//s

# KTRE bug?
# l =~ s/\n+$//

var s = 0
for split /\n/, l {
	var min = -1
	var max = 0

	for split /\s+/ {
		min = int when min < 0
		max = int > max ? int : max
		min = int < min ? int : min
	}

	s += max - min
}

pl s

s = 0
for split /\n/, l {
	var a = 0
	var b = 0

	for var i; split /\s+/ {
		for var j; split /\s+/ {
			# Shitty operator precedence issue with = and ,.
			if int i %% int j and i != j:
				(a = int i), (b = int j)

			last when a
		}

		last when a
	}

	s += a / b
}

pl s
