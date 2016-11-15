#!/bin/bash
RED='\033[0;31m'
GRN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'
DIR=.nemoexplorer_thumb

if [[ -z $1 ]]; then
    echo -e "${RED}Usage: ./thumb.sh [directory]${NC}" 
    exit
fi

if [[ -z $(which ffmpeg 2> /dev/null) ]]; then
    echo -e "${RED}Install ffmpeg, plz${NC}"
    exit
fi

CONVERT=convert
if [[ -z $(which convert 2> /dev/null) ]]; then
    CONVERT=convert.im6
    if [[ -z $(which convert.im6 2> /dev/null) ]]; then
        echo -e "${RED}Install image-magick, plz${NC}"
        exit
    fi
fi

if [ ! -e "/usr/lib64/libbz2.so.1.0" ]; then
    echo "Make symbolic link for libbz2"
    ln -s /usr/lib64/libbz2.so.0.0.0 /usr/lib64/libbz2.so.1.0
fi

MVSIZE=384
IMGSIZE=384

function make_thumb
{
    for i in "$1"/*.avi "$1"/*.mp4 "$1"/*.mkv "$1"/*.wmv "$1"/*.mov "$1"/*.webm "$1"/*.mpg "$1"/*.flv "$1"/*.swf "$1"/*.divx
    do
        if [ ! -e "$i" ]; then
            continue
        fi
        echo -e "${BLUE}creating a thumbnail animation for $i${NC}"
        export OUT="$1/${DIR}/${i##*/}.%3d.jpg"
        < /dev/null ffmpeg -i "$i" -ss 00:00:10 -vf scale="-1:${MVSIZE}" -vframes 120 "${OUT}"
# Too slow, seems to be not needed.
#        for j in {1..9}
#        do
#            echo -e "${BLUE}creating thumbnail for ${OUT}"
#            export OUT="$1/${DIR}/${i##*/}.00${j}.jpg"
#            $CONVERT "${OUT}" -thumbnail ${IMGSIZE}^ "${OUT}"
#        done
#        for (( j=10 ; j <= 99 ; j++ ))
#        do
#            echo -e "${BLUE}creating thumbnail for ${OUT}"
#            export OUT="$1/${DIR}/${i##*/}.0${j}.jpg"
#            $CONVERT "${OUT}" -thumbnail ${IMGSIZE}^ "${OUT}"
#        done
#        for j in {100..120}
#        do
#            echo -e "${BLUE}creating thumbnail for ${OUT}"
#            export OUT="$1/${DIR}/${i##*/}.${j}.jpg"
#            $CONVERT "${OUT}" -thumbnail ${IMGSIZE}^ "${OUT}"
#        done
    done

    for i in "$1"/*.jpg "$1"/*.png
    do
        if [ ! -e "$i" ]; then
            continue
        fi
        echo -e "${BLUE}creating a thumbnail for $i${NC}"
        $CONVERT "$i" -thumbnail ${IMGSIZE}^ "$1/${DIR}/${i##*/}"
    done
}


find "${1}" -type d -print0 | while  IFS= read -r -d '' i
do 
    if [[ "${i##*/}" != .* ]]; then
        echo -e "${GRN}Searching media files in "$i"${NC}"
        rm -rf "$i/${DIR}"
        mkdir  "$i/${DIR}"
        make_thumb "$i"
    fi
done
