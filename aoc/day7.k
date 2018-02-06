var a b c d = I(day7.txt), [], 0, 0
a =~ s/\s+\Z//
c = split /\n/, a

for c: if /\w+ \(\d+\) -> (.*)/: for $1 =~ /\w+/: push b

for c {
	var e = false

	if /^(\w+) / {
		for b: if _ == $1: e = true
		d = $1 when !e
	}

	last when !e
}

pl d

# fn f (x) for c: return int $2 when /^(\w+) \((\d+)\)/ and x == $1
# fn g (x) for c: return $2 =~ /\w+/g when /^(\w+) \(\d+\) -> (.*)/ and x == $1
