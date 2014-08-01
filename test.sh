./parse <test.txt > test.html

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
