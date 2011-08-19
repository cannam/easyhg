#!/bin/bash

css='<link rel="stylesheet" type="text/css" href="help.css"/>'
backlink='<p><a href="topics.html">Back to Topics</a></p>'

echo "$css" > topics.html
cat intro.html >> topics.html

pcat=""

for x in topics/*.txt ; do

    b=`basename "$x" .txt`
    out="a-$b.html"

    echo "$css" > "$out"
    echo "$backlink<hr>" >> "$out"

    cat "$x" | perl -e '
$_ = join "", <>;
s/^{[\w\s]+}//s;
s/^(\s*)([A-Za-z][^\n]*)/$1<h2>$2<\/h2>/s;
s/^\s+\*\s+(.*)$/<ul><li>$1<\/li><\/ul>/gm;
s/\*([\w"][^\*]+)\*/<b>$1<\/b>/gs;
s/"([\w])/&ldquo;$1/gs;
s/([\w])"/$1&rdquo;/gs;
s/^\#([^\s]+)$/<center><img src="images\/$1.png"><\/center>/gm;
s/\n-+\n/\n/gs;
s/\n\n([^\n])/\n\n<p>$1/gs;
s/^\n*([^<\n])/\n<p>$1/gs;
s/^\n*(<[^p])/\n<p>$1/gs;
s/([^\n])\n\n/$1<\/p>\n\n/gs;
s/([^>\n])\n*$/$1<\/p>\n\n/gs;
s/\[\[([^\|]*)\|([^\]]*)\]\]/<a href="a-$1.html">$2<\/a>/gs;
s/\[\[([^\|\]]*)\]\]/<a href="$1">$1<\/a>/gs;
s/\b_([^_]+)_\b/<i>$1<\/i>/gs;
s/@(\w[^@]+)@/<code>$1<\/code>/gs;
s/---/&mdash;/gs;
s/--/&ndash;/gs;
s/<p><h2>/<h2>/gs;
s/<\/h2><\/p>/<\/h2>/gs;
print;
' >> "$out"

    echo "<hr>$backlink" >> "$out"

    category=`grep '^{.*}$' "$x" | sed 's/[{}]//g'`

    if [ "$category" != "$pcat" ]; then
	echo "<h3>$category</h3>" >> topics.html
	pcat="$category"
    fi

    grep '<h2>' "$out" | sed "s|<h2>|<p><a href=\"$out\">|" | sed 's/<\/h2>/<\/a><\/p>/' >> topics.html

done

