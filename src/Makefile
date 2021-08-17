SRCS = jsondecode.cc jsonencode.cc
OCTS = $(SRCS:.cc=.oct)
HEADS = octave7.h

RAPID_JSON_URL = https://github.com/Tencent/rapidjson/archive/master.tar.gz
RAPID_JSON_TAR = master.tar.gz

MKOCTFILE ?= mkoctfile

CURL_OPTS = --fail --location --silent --show-error --output

all: $(OCTS)

%.oct: %.cc $(HEADS) rapidjson
	$(MKOCTFILE) -Irapidjson/include $< -o $@

$(RAPID_JSON_TAR):
	curl $(CURL_OPTS) $(RAPID_JSON_TAR) $(RAPID_JSON_URL)

rapidjson: $(RAPID_JSON_TAR)
	tar -xf $(RAPID_JSON_TAR)
	mv rapidjson-master rapidjson
