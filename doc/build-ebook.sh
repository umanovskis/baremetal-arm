#!/usr/bin/env bash

TEMPLATE=~/.pandoc/templates/eisvogel.tex
PANDOC_VERSION=$(pandoc -v | head -1 | cut -d ' ' -f 2)
if [ ! $(echo $PANDOC_VERSION | grep -q "^2") ]; then
    CHAPTERS_CMD="--top-level-division=chapter"
    LLANG="en-US"
else
    CHAPTERS_CMD="--chapters"
    LLANG="english"
fi

if [ ! -f "$TEMPLATE" ]; then
    TEMPLATE=~/pandoc-latex-template/eisvogel.tex
fi
if [ ! -f "$TEMPLATE" ]; then
    if [ $? -eq 1 ]; then
        TEMPLATE="$1"
    fi
fi
if [ ! -f "$TEMPLATE" ]; then
    echo "Cannot find eisvogel.tex anywhere. Install it, or point it out as an argument to this script"
    exit 1
fi

COMMIT=$(git rev-parse HEAD | cut -c -10)
pandoc *.md -o ebook.tex --from markdown --template "$TEMPLATE" -V book  -V classoption=oneside -V header-right="$COMMIT" -V lang="$LLANG" -V doublequote="" -V secnumdepth=0 -N --listings $CHAPTERS_CMD
pandoc *.md -o ebook.pdf --from markdown --template "$TEMPLATE" -V book  -V classoption=oneside -V header-right="$COMMIT" -V lang="$LLANG" -V doublequote="" -V secnumdepth=0 -N --listings $CHAPTERS_CMD
pandoc *.md -o ebook-s5.tex --from markdown --template "$TEMPLATE" -H s5.tex -V book  -V classoption=oneside -V header-right="$COMMIT" -V lang="$LLANG" -V doublequote="" -V secnumdepth=0 -N --listings $CHAPTERS_CMD
pandoc *.md -o ebook-s5.pdf --from markdown --template "$TEMPLATE" -H s5.tex -V book  -V classoption=oneside -V header-right="$COMMIT" -V lang="$LLANG" -V doublequote="" -V secnumdepth=0 -N --listings $CHAPTERS_CMD
if [ -f ebook.pdf ] && [ -f upload.sh ]; then
    sh -xe upload.sh &
fi
