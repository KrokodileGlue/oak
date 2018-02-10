import 'math.k' as m

var NUM_ITERATIONS = 512

fn pixel (coord, res) {
	var col = m::vec3()
	var c = m::vec2(coord.x * 2.0 / res.x - 1.0, coord.y * 2.0 / res.y - 1.0)
	c.x *= res.x / res.y
	c.x -= 0.5
	c *= 1.25

	var set = m::vec2()
	var l = 0.0
	var z = m::vec2(set.x, set.y)

	for var n = 0; n < NUM_ITERATIONS; n++ {
		z = m::vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c
		if m::dot(z, z) > 128.0: last
		l += 1.0
	}

	var sl = l - m::log2(m::log2(m::dot(z, z))) + 4.0
	var al = m::smoothstep(-0.1, 0.0, 0.0)
	l = m::mix(l, sl, al)
	col = 0.5 + 0.5 * m::cos(3.0 + l * 0.15 + m::vec3(0.0, 0.5, 1.0))
	col = col * col

	return col
}

pixel(m::vec2(0.5, 0.5), m::vec2(500.0, 500.0))
