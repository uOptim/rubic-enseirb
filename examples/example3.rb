# Cet exemple illustre la conversion de nombres vers les booléens, la
# declaration de variables dans les structures de controle et le calcul
# de 5!

b = 5
a = 1

while b
	tmp = 0
	for i in 1 .. b
		# tmp = b
		tmp = tmp + 1
	end
	a = tmp * a
	b = b - 1
end

puts a
