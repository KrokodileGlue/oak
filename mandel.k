var width = 1024
var height = 780

var offset = 150

pl 'P3'
pl width, ' ', height
pl 80

fn abs (x) = x < 0 ? x : -x

for var y = 0; y < height; y++ {
	for var x = -offset; x < width - offset; x++ {
		var a = x - width / 2
		var b = y - height / 2

		var res

		var oa = a
		var ob = b

		for var n = 0; n < 32; n++ {
			res = n
			var aa = a * a / 256
			var bb = b * b / 256

			var temp = aa + bb

			temp = abs(temp)
			last when temp > 2048

			b = a * b * 2 / 256 + ob
			a = aa - bb + oa
		}

		pl res / 2, ' ', res, ' ', res * 4
	}
}
