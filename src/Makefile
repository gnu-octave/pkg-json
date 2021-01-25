SRCS = jsondecode.cc jsonencode.cc
OCTS = $(SRCS:.cc=.oct)
HEADS = octave7.h

WGET_OPTS = --quiet --no-check-certificate

all: $(OCTS)

%.oct: %.cc $(HEADS) rapidjson
	$(MKOCTFILE) -Irapidjson/include $< -o $@

master.tar.gz:
	wget $(WGET_OPTS) https://github.com/Tencent/rapidjson/archive/master.tar.gz

rapidjson: master.tar.gz
	tar -xf master.tar.gz
	mv rapidjson-master rapidjson
