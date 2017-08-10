echo "create $1"
shift
for f in "$@"; do
		if [[ "${f:-2}" = ".a" ]]; then
				echo "addlib $f"
		else
				echo "addmod $f"
		fi
done
echo "save"
echo "list"
echo "end"


