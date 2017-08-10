echo "create libhtmlish.a"
for f in "$@"; do
		if [[ "${f:-2}" -eq ".a" ]]; then
				echo "addlib $f"
		else
				echo "addmod $f"
		fi
		echo "save"
		echo "list"
		echo "end"
done

