echo "create $1"
shift
for f in "$@"; do
		tail=${f:${#f}-2}
		if [[ "$tail" = ".a" ]]; then
				echo "addlib $f"
		else
				echo "addmod $f"
		fi
done
echo "save"
echo "end"


