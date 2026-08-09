# Copyright (c) 2026 Rally Tactical Systems, Inc.
# All rights reserved.
#
# CoT sample generator (C++11)
CXX ?= c++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -O2

TARGET = generate_cot_payload
SRC = generate_cot_payload.cpp

# PDF: pandoc (HTML) + Google Chrome headless (Linux: CHROME=/usr/bin/google-chrome or chromium)
CHROME ?= /Applications/Google Chrome.app/Contents/MacOS/Google Chrome

.PHONY: all clean readme-pdf

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

readme-pdf: README.md
	pandoc README.md -s --metadata title="Cursor on Target (CoT) — cot" -o README.html
	"$(CHROME)" --headless --disable-gpu --no-pdf-header-footer \
		--print-to-pdf="$(CURDIR)/README.pdf" "file://$(CURDIR)/README.html"
	rm -f README.html

clean:
	rm -f $(TARGET) README.html
