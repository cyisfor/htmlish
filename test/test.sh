PARSE=../build/parse

$PARSE <test.txt > test.html
# note: xmlwf fails on html5
# b/c libxml doesn't use self-closing tags on <meta> in html5

xmlwf test.html > wf.log
if [ -z $(dd if=wf.log count=1 bs=1 2>/dev/null) ]; then
    echo XML valid
else
    echo Invalid XML!
    cat wf.log
fi

cleanup() {
    rm -f wf.log
}

trap cleanup 'EXIT'
