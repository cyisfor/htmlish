(
    echo hmph
    echo "this <b>is</b> a test"
    echo 'derp <pre>'
    echo '  whee    wheeee'
    echo '</pre> erpd'
    echo 'derp2 '
    echo
    echo
    echo
    echo '<blockquote>beep</blockquote>'
) | ./parse > test.html

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