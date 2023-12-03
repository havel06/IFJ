
.PHONY: clean

out/dokumentace.pdf: dokumentace.tex $(wildcard sections/*) $(wildcard graphics/*)
	latexmk -pdf -output-directory=out $<

clean:
	rm out/dokumentace.*
