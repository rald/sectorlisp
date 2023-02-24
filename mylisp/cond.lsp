((LAMBDA (X Y)

	(COND 
		((EQ X Y) (QUOTE (EQUAL)))
		((QUOTE T) (QUOTE (NOT EQUAL)))
	)

) (QUOTE 1) (QUOTE 2) )
