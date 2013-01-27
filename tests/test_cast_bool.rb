# Cet exemple illustre la conversion de nombres vers les booléens et les
# opérations sur les booléens.

b = 5.2

if b then
	puts 1
else
	puts 2
end

if (b && false) then
	puts 1
else
	puts 2
end

if (0 || true) then
	puts 1
else
	puts 2
end
