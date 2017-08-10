echo "create $1"
shift
for f in "$@"; do
		echo ${f:-1}

		if [[ "${f:-1}" = ".a" ]]; then
				echo "addlib $f"
		else
				echo "addmod $f"
		fi
done
echo "save"
echo "list"
echo "end"


