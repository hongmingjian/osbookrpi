#.SUFFIXES: .svg .pdf
#.svg.pdf:
#	inkscape -D -z -o $@ $<
#	#convert -density 600 -font ArialUnicode $< $@  # cannot trim the border

FILES=3-5.pdf 4-1.pdf 4-6.pdf 4-7.pdf 4-9.pdf 5-1.pdf 5-2.pdf

all: 
	inkscape -D -z --export-type=pdf $(FILES:pdf=svg)

clean:
	$(RM) $(FILES)
