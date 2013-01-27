# Cet exemple illustre la conversion de nombres vers les booléens

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

if (b && true) then
	puts 1
else
	puts 2
end
