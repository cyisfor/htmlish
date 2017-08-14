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
	
dir=/extra/home/packages/git/libxml2
remote=git://git.gnome.org/libxml2
dest=libxml2
sync

dir=/home/code/note
remote=https://github.com/cyisfor/note.git
dest=note
sync
