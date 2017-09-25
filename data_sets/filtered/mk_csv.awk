#!/usr/bin/awk -f
BEGIN { sum = 0 }
{
	split($1, a, ":")
	split($1, b, "_")
	split($1, c, ".")
	curr = b[1]  "_"  b[2]
	curr_prefix = c[1]
	if (match(c[2], /res_/) == 0)
	{
		curr_prefix = c[1] "." c[2]
	}
	if (NR == 1)
	{
		prev = curr
		prev_prefix = curr_prefix
		printf "%s,%s,", curr_prefix, $3
	}
	if (prev != curr)
	{
		printf "%09.6f,", (sum/8)
		if (prev_prefix != curr_prefix)
		{
			printf "\n%s,%s,", curr_prefix, $3
		}
		sum = 0
	}
	sum += $4
	prev = curr
	prev_prefix = curr_prefix
}
END { printf "\n" }
