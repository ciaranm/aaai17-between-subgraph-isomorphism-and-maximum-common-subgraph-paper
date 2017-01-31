all : tables graphs
	latexmk -pdf -pdflatex='pdflatex -interaction=nonstopmode %O %S' -shell-escape poster; cp poster.pdf ~/Downloads

TABLES =

GRAPHS = gen-graph-runtimes.tex gen-graph-which-k-by-family.tex #gen-graph-versus-cp.tex

tables : $(TABLES)

graphs : $(GRAPHS)

gen-graph-%.tex : graph-%.gnuplot
	gnuplot $<
	sed -i -e '19s/^\(\\path.*\)/\% \1/' gen-graph-$*.tex # epic haxx
	#latexmk -pdf gen-graph-$*

