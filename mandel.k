fn iter (depth a b) {
	var res

	var oa = a
	var ob = b

	for var n = 0; n < 32; n++ {
		res = n
		var aa = a * a / 256
		var bb = b * b / 256

		last when aa + bb > 2048

		b = a * b * 2 / 256 + ob
		a = aa - bb + oa
	}

	return res
}

fn mandel (width height offset) {
	pl 'P3'
	pl width, ' ', height
	pl 80

	for var y = 0; y < height; y++ {
		for var x = -offset; x < width - offset; x++ {
			var res = iter(32, x - width / 2, y - height / 2)
			pl res / 2, ' ', res, ' ', res * 4
		}
	}
}

mandel(1024, 780, 150)
