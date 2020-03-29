.SUFFIXES: .svg .pdf
.svg.pdf:
	convert -density 600 -font ArialUnicode  $< $@

FILES=3-5.pdf 4-1.pdf 4-6.pdf 4-7.pdf 4-9.pdf

all: $(FILES)

clean:
	$(RM) $(FILES)
