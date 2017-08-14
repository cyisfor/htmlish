function sync {
		if [[ -d $dir ]]; then
				source=$dir
				adjremote=1
		else
				source=$remote
		fi
		if [[ -d $dest ]]; then
				cd $dest
				git pull $source
				cd ..
		else
				git clone $source $dest
				if [[ -n "$adjremote" ]]; then
						cd $dest
						git remote origin set-url $remote
						cd ..
				fi
		fi
}
	
dir=/extra/home/code/html_when
remote=https://github.com/cyisfor/html_when.git
dest=html_when
sync
