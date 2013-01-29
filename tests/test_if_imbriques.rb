q = 10
while q
	puts q
	if q / 2 == 0 then
		puts 1
		if false then
			puts 11
		end
	else
		if q / 3 == 0 then
			puts 2
		else
			if q / 5 == 0 then
				puts 3
				for i in 1 .. 10
					puts 3 + i
				end
			else
				puts 4
			end
		end
	end
	q = q - 1
	puts 0.0
end
