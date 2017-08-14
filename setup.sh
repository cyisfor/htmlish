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
						git remote add local $source
						cd ..
				fi
		fi
}

function uplink {
		source=$1/$2
		[[ -L $source ]] && return;
		ln -rs $source $2
}

dir=/home/code/html_when
remote=https://github.com/cyisfor/html_when.git
dest=html_when
sync

uplink ./html_when libxml2