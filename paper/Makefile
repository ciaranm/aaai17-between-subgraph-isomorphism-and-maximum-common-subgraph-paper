all : tables graphs
	latexmk -pdf -pdflatex='pdflatex -interaction=nonstopmode %O %S' -shell-escape paper

TABLES =

GRAPHS = gen-graph-ids.pdf gen-graph-constraints.pdf \
	 gen-graph-constraints-induced-0.pdf \
	 gen-graph-constraints-induced-1.pdf \
	 gen-graph-constraints-induced-2.pdf \
	 gen-graph-constraints-induced-3.pdf \
	 gen-graph-constraints-induced-4.pdf \
	 gen-graph-constraints-induced-5.pdf \
	 gen-graph-runtimes.pdf gen-graph-which-k-by-family.pdf gen-graph-versus-cp.pdf

tables : $(TABLES)

graphs : $(GRAPHS)

gen-graph-%.pdf : graph-%.gnuplot
	gnuplot $<
	sed -i -e '19s/^\(\\path.*\)/\% \1/' gen-graph-$*.tex # epic haxx
	latexmk -pdf gen-graph-$*

